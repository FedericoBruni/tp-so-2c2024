#include "utilidades.h"

t_log *logger;
t_config *config;
char *ip_memoria;
char *puerto_memoria;
char *ip_cpu;
char *puerto_cpu_dispatch;
char *puerto_cpu_interrupt;
char *algoritmo_planificacion;
int quantum;
char *log_level;
t_queue *cola_new;
t_queue *cola_ready;
t_queue *cola_blocked;
t_queue *cola_exit;
t_queue *cola_finalizacion;
t_queue *cola_new_hilo;
TCB *tcb_en_ejecucion;
int autoincremental_pcb = 0;
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_blocked;
pthread_mutex_t mutex_fd_memoria;
sem_t sem_hay_memoria;
sem_t sem_crear_hilo;
sem_t sem_finalizar_proceso;
sem_t sem_finalizar_hilo;
sem_t sem_hay_ready;
sem_t sem_hay_new;
sem_t sem_cpu_ejecutando;
sem_t memoria_libre;
PCB *pcb_en_ejecucion;
TCB *tcb_a_crear = NULL;
t_list* colas_prioridades;
t_list* mutex_sistema;




void iniciar_kernel(void)
{
    config = iniciar_config("kernel.config");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    ip_cpu = config_get_string_value(config, "IP_CPU");
    puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    quantum = config_get_int_value(config, "QUANTUM");
    log_level = config_get_string_value(config, "LOG_LEVEL");
    logger = iniciar_logger("logKernel.log", "Kernel", log_level_from_string(log_level));
    cola_new = queue_create();
    cola_new_hilo = queue_create();
    cola_ready = queue_create();
    cola_blocked = queue_create();
    cola_exit = queue_create();
    cola_finalizacion = queue_create();
    colas_prioridades = list_create();
    mutex_sistema = list_create();
    iniciar_semaforos();
}

void iniciar_semaforos(void){
    inicializar_mutex(&mutex_new, "New");
    inicializar_mutex(&mutex_ready, "Ready");
    inicializar_mutex(&mutex_blocked, "Blocked");
    inicializar_mutex(&mutex_exec, "Execute");
    inicializar_mutex(&mutex_exit, "Exit");
    inicializar_mutex(&mutex_fd_memoria, "Mutex FD Memoria");

    inicializar_semaforo(&sem_hay_memoria, "Hay memoria", 0);
    inicializar_semaforo(&sem_crear_hilo, "Crear hilo", 0);
    inicializar_semaforo(&sem_finalizar_proceso, "Finalizar proceso", 0);
    inicializar_semaforo(&sem_finalizar_hilo, "Finalizar hilo", 0);
    inicializar_semaforo(&sem_hay_ready, "Hay hilo en ready", 0);
    inicializar_semaforo(&sem_hay_new, "Hay proceso en new", 0);
    inicializar_semaforo(&sem_cpu_ejecutando, "Hay un hilo ejecutando en cpu", 0);
    inicializar_semaforo(&memoria_libre, "Memoria liberado", 1);
    


}

void inicializar_mutex(pthread_mutex_t* mutex, char* nombre){
    if (pthread_mutex_init(mutex, NULL) != 0) {
        log_error(logger, "No se pudo inicializar el MUTEX: %s", nombre);
        exit(-1);
    }
}

void inicializar_semaforo(sem_t* semaforo, char* nombre, int valor){
    if (sem_init(semaforo, 1, valor) != 0) {
        log_error(logger, "No se pudo inicializar el SEMAFORO: %s", nombre);
        exit(-1);
    }
}

int conectarse_a_cpu_interrupt(void)
{
    return crear_conexion(ip_cpu, puerto_cpu_interrupt, logger);
}

int conectarse_a_cpu_dispatch(void)
{
    return crear_conexion(ip_cpu, puerto_cpu_dispatch, logger);
}

int conectarse_a_memoria(void)
{
    return crear_conexion(ip_memoria, puerto_memoria, logger);
}

void terminar_ejecucion(int dispatch, int memoria, int interrupt)
{
    close(dispatch);
    close(memoria);
    close(interrupt);
    queue_destroy_and_destroy_elements(cola_new, liberar_pcb);
    // if(cola_new != NULL){
    //     log_error(logger,"Cola NEW no se borro correctamente");
    // }
    queue_destroy_and_destroy_elements(cola_ready, liberar_tcb);
    // if(cola_ready != NULL){
    //     log_error(logger,"Cola READY no se borro correctamente");
    // }
    queue_destroy_and_destroy_elements(cola_blocked, liberar_tcb);
    // if(cola_blocked != NULL){
    //     log_error(logger,"Cola BLOCKED no se borro correctamente");
    // }
    queue_destroy_and_destroy_elements(cola_exit, liberar_tcb);
    // if(cola_exit != NULL){
    //     log_error(logger,"Cola EXIT no se borro correctamente");
    // } HAY Q ARREGLAR LAS FUNCIONES DE DESTRUCCION
    log_info(logger, "Finalizando ejecucion de KERNEL");
    log_destroy(logger);
    config_destroy(config);
    exit(EXIT_SUCCESS);
}

/* Ejemplo
void person_destroy(void *ptr)
{
    t_person *person = (t_person *)ptr;
    free(person->name);
    free(person);
} */



void mover_tcbs_exit(PCB *pcb)
{
    int size = list_size(pcb->threads);
    for (unsigned int i = 0; i < size; i++)
    {
        TCB *tcb = list_get(pcb->threads, i);
        cambiar_estado_hilo(tcb, EXIT);
        queue_push(cola_exit, tcb);
    }
}
	// * t_person* find_by_name_contains(t_list* people, char* substring) {
	// *     bool _name_contains(void* ptr) {
	// *         t_person* person = (t_person*) ptr;
	// *         return string_contains(person->name, substring);
	// *     }
	// *     return list_find(people, _name_contains);
	// * }



void encolar(t_queue* cola, void* elemento, pthread_mutex_t mutex){
    pthread_mutex_lock(&mutex);
    queue_push(cola, elemento);
    pthread_mutex_unlock(&mutex);
}

void* desencolar(t_queue* cola, pthread_mutex_t mutex){
    pthread_mutex_lock(&mutex);
    void* elemento = queue_pop(cola);
    pthread_mutex_unlock(&mutex);
    return elemento;
}

bool comparar_prioridad(void *a, void *b){
    COLA_PRIORIDAD* cola1 = (COLA_PRIORIDAD*)a;
    COLA_PRIORIDAD* cola2 = (COLA_PRIORIDAD*)b;
    return cola1->prioridad <= cola2->prioridad;
}

COLA_PRIORIDAD* crear_multinivel(TCB* tcb){
    COLA_PRIORIDAD* cola_nueva = malloc(sizeof(COLA_PRIORIDAD));
    cola_nueva->cola_prioridad = queue_create();
    cola_nueva->prioridad = tcb->prioridad;
    cola_nueva->quantum = quantum;
    inicializar_mutex(&cola_nueva->mutex, "Mutex cola prioridad");
    list_add(colas_prioridades, cola_nueva);
    list_sort(colas_prioridades, (void*)comparar_prioridad);
    return cola_nueva;
}

COLA_PRIORIDAD *existe_cola_con_prioridad(int prioridad) {
    bool _tiene_prioridad(void *ptr){
        COLA_PRIORIDAD* cola = (COLA_PRIORIDAD*) ptr;
        return cola->prioridad == prioridad;
    }
    list_find(colas_prioridades, _tiene_prioridad);
}

void encolar_multinivel(COLA_PRIORIDAD *cola, TCB *tcb){
    encolar(cola->cola_prioridad, tcb, cola->mutex);

}

void* desencolar_multinivel(COLA_PRIORIDAD *cola){
    return desencolar(cola->cola_prioridad,cola->mutex);
}

COLA_PRIORIDAD* obtener_cola_con_mayor_prioridad() { // y q tenga elementos
    for (unsigned int i = 0; i < list_size(colas_prioridades) ; i++){
        COLA_PRIORIDAD*  cola_prioridad = list_get(colas_prioridades,i);
        if(!queue_is_empty(cola_prioridad->cola_prioridad))
            return cola_prioridad; 
    }
    return NULL;
}

void replanificar(TCB *tcb){
    if(string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
        COLA_PRIORIDAD * cola = existe_cola_con_prioridad(tcb->prioridad);
        if(cola != NULL){
            encolar_multinivel(cola, tcb);
        } else{
            COLA_PRIORIDAD *cola_nueva = crear_multinivel(tcb);
            log_info(logger,"Se creo la cola multinivel de prioridad: %i",cola_nueva->prioridad);
            encolar_multinivel(cola_nueva, tcb);
        }         
    }else{
        encolar(cola_ready, tcb,mutex_ready);     // los hilos van en la cola de ready o los procesos? o ambos por separado
        //imprimir_pcb(pcb); 
    }
    sem_post(&sem_hay_ready);
}

MUTEX *existe_mutex(char* recurso){
    bool _hay_recurso(void *ptr){
        MUTEX *mutex = (MUTEX *)ptr;
        return string_equals_ignore_case(mutex->recurso, recurso);
    }
    list_find(mutex_sistema,_hay_recurso);
}

void bloquear_hilo_mutex(t_list* cola_bloqueados, TCB* tcb){
    queue_push(cola_bloqueados,tcb);
    cambiar_estado_hilo(tcb, BLOCKED);
    sem_post(&sem_hay_ready);
}

void desbloquear_hilo_mutex(MUTEX *mutex){
    TCB *tcb = queue_pop(mutex->cola_bloqueados);
    if(tcb != NULL){
        mutex->asignadoA = tcb->tid;
        cambiar_estado_hilo(tcb, READY);
        replanificar(tcb);
    }else{
        mutex->asignadoA = NULL;
        mutex->binario = 1;
    }
}
void asignar_a_hilo_mutex(MUTEX *mutex, TCB *tcb){
    mutex->asignadoA=tcb->tid;
    mutex->binario = 0;
}

void bloquear_hilo_syscall(TCB *tcb,int tid){
    cambiar_estado_hilo(tcb,BLOCKED);
    tcb->bloqueadoPor = tid;
    encolar(cola_blocked,tcb,mutex_blocked);
    sem_post(&sem_hay_ready);
}



void desbloquear_bloqueados_por_hilo(tidBloqueante){
    t_list* hilos_bloqueados = list_create();
    bool _esBloqueado(void *ptr){
        TCB *tcb = (TCB *)ptr;
        return tcb->bloqueadoPor == tidBloqueante;
    }
    hilos_bloqueados = list_filter(cola_blocked,_esBloqueado);
    for(int i = 0; i< list_size(hilos_bloqueados); i++){
        desbloquear_hilo(list_get(hilos_bloqueados,i));
    }
    
    list_destroy(hilos_bloqueados);
}

void desbloquear_hilo(TCB *tcb){
    cambiar_estado_hilo(tcb,READY);
    replanificar(tcb);
}
