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
t_queue *cola_fin_pcb;
t_queue *cola_io;
t_queue *cola_new_hilo;
TCB *tcb_en_ejecucion;
int autoincremental_pcb = 0;
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_blocked;
pthread_mutex_t mutex_fd_memoria;
pthread_mutex_t mutex_io;
sem_t sem_hay_memoria;
sem_t sem_crear_hilo;
sem_t sem_finalizar_proceso;
sem_t sem_finalizar_hilo;
sem_t sem_hay_ready;
sem_t sem_hay_new;
sem_t sem_cpu_ejecutando;
sem_t memoria_libre;
sem_t sem_puede_ejecutar;
sem_t sem_syscall_fin;
sem_t sem_io;
sem_t sem_exec_recibido;
sem_t sem_io_iniciado;
PCB *pcb_en_ejecucion;
t_list* colas_prioridades;
t_list* mutex_sistema;
TCB *tcb_anterior;
extern char* estado_lock;
extern int fd_cpu_dispatch;
//extern int fd_memoria;
extern int fd_cpu_interrupt;
extern pthread_t hilo_io;
bool fin_ciclo=false;


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
    cola_ready = queue_create();
    cola_blocked = queue_create();
    cola_exit = queue_create();
    cola_fin_pcb=queue_create();
    cola_io = queue_create();
    cola_finalizacion = queue_create();
    colas_prioridades = list_create();
    cola_new_hilo = queue_create();
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
    inicializar_mutex(&mutex_io, "Mutex IO");

    inicializar_semaforo(&sem_hay_memoria, "Hay memoria", 1);
    inicializar_semaforo(&sem_crear_hilo, "Crear hilo", 0);
    inicializar_semaforo(&sem_finalizar_proceso, "Finalizar proceso", 0);
    inicializar_semaforo(&sem_finalizar_hilo, "Finalizar hilo", 0);
    inicializar_semaforo(&sem_hay_ready, "Hay hilo en ready", 0);
    inicializar_semaforo(&sem_hay_new, "Hay proceso en new", 0);
    inicializar_semaforo(&sem_cpu_ejecutando, "Hay un hilo ejecutando en cpu", 0);
    inicializar_semaforo(&memoria_libre, "Memoria liberado", 1);
    inicializar_semaforo(&sem_puede_ejecutar, "Puede ejecutar", 1);
    inicializar_semaforo(&sem_syscall_fin, "sem_syscall_fin", 0);
    inicializar_semaforo(&sem_io, "sem_io", 0);
    inicializar_semaforo(&sem_exec_recibido,"sem exec",0);
    inicializar_semaforo(&sem_io_iniciado, "sem_io_iniciado", 0);
    


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

void terminar_ejecucion()
{
    close(fd_cpu_dispatch);
    //close(fd_memoria);
    close(fd_cpu_interrupt);

    queue_destroy_and_destroy_elements(cola_new, liberar_pcb);
    queue_destroy_and_destroy_elements(cola_ready, liberar_tcb);
    queue_destroy_and_destroy_elements(cola_blocked, liberar_tcb);
    queue_destroy_and_destroy_elements(cola_exit, liberar_tcb);
    queue_destroy_and_destroy_elements(cola_finalizacion, liberar_tcb);
    queue_destroy_and_destroy_elements(cola_fin_pcb, liberar_pcb);
    queue_destroy_and_destroy_elements(cola_new_hilo,liberar_tcb);

    if (string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
        for(int i = 0; i < list_size(colas_prioridades);i++){
            COLA_PRIORIDAD *cola_prioridad = list_get(colas_prioridades, i);
            if (!queue_is_empty(cola_prioridad->cola_prioridad)) queue_destroy_and_destroy_elements(cola_prioridad->cola_prioridad,liberar_tcb);
            else queue_destroy(cola_prioridad->cola_prioridad);
            free(cola_prioridad);
            
        }
        
    }
    list_destroy(colas_prioridades);
    for (int i = 0; i < list_size(mutex_sistema); i++) {
        MUTEX *mutex = list_get(mutex_sistema, i);
        queue_destroy_and_destroy_elements(mutex->cola_bloqueados, liberar_tcb);
        free(mutex->recurso);
        free(mutex);
    }
    list_destroy(mutex_sistema);

    while (!queue_is_empty(cola_io)) {
        IOStruct *structIo = queue_pop(cola_io);
        liberar_tcb(structIo->tcb);
        free(structIo);
    }
    queue_destroy(cola_io);
    if (tcb_en_ejecucion) liberar_tcb(tcb_en_ejecucion);
    if (pcb_en_ejecucion) liberar_pcb(pcb_en_ejecucion);
    
    log_info(logger, "Finalizando ejecucion de KERNEL");
    

    fin_ciclo=true;
    sem_post(&sem_hay_new);
    sem_post(&sem_finalizar_proceso);
    sem_post(&sem_crear_hilo);
    sem_post(&sem_finalizar_hilo); 
    sem_post(&sem_io);
    sem_post(&sem_hay_ready);
    sleep(1);
    config_destroy(config);
    log_destroy(logger);
    //exit(EXIT_SUCCESS);
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
        //queue_push(cola_exit, tcb);
        encolar(cola_finalizacion, tcb, mutex_exit);
        sem_post(&sem_finalizar_hilo);
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
    //log_trace(logger, "Encolando multinivel Prioridad: %i, Cola prioridad size: %i", cola->prioridad, queue_size(cola->cola_prioridad));

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
    tcb_anterior = tcb;
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
    sem_post(&sem_puede_ejecutar);
}

MUTEX *existe_mutex(char* recurso){
    bool _hay_recurso(void *ptr){
        MUTEX *mutex = (MUTEX *)ptr;
        return string_equals_ignore_case(mutex->recurso, recurso);
    }
    return list_find(mutex_sistema,_hay_recurso);
}

void bloquear_hilo_mutex(t_list* cola_bloqueados, TCB* tcb){
    queue_push(cola_bloqueados,tcb);
    cambiar_estado_hilo(tcb, BLOCKED);
    if(!queue_is_empty(cola_ready)) sem_post(&sem_hay_ready);
}

void desbloquear_hilo_mutex(MUTEX *mutex){
    if (queue_is_empty(mutex->cola_bloqueados)){
        log_error(logger,"TCB es NULL en desbloquear");
        mutex->asignadoA = NULL;
        mutex->binario = 1;
        estado_lock = "LIBRE";
    }else{
        TCB *tcb = queue_pop(mutex->cola_bloqueados);
        if(tcb != NULL){
            log_error(logger,"TCB NO ES NULL EN DESBLOQUEAR MUTEX");
            if(string_equals_ignore_case(algoritmo_planificacion, "MULTINIVEL")){
            COLA_PRIORIDAD *cola = existe_cola_con_prioridad(tcb->prioridad);
                if(cola != NULL){
                    encolar_multinivel(cola, tcb);
                    log_error(logger, "Encolar en desbloquear_hilo_mutex()");
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
            mutex->asignadoA = tcb->tid;
            mutex->binario = 0;
        }
    }
}
void asignar_a_hilo_mutex(MUTEX *mutex, TCB *tcb){
    mutex->asignadoA=tcb->tid;
    mutex->binario = 0;
}

int bloquear_hilo_syscall(TCB *tcb,int tid){
    bool hayHilo= false;
    log_warning(logger, "SIZE de tids: %i", list_size(pcb_en_ejecucion->tids));
    for (int i = 0; i < list_size(pcb_en_ejecucion->tids); i++) {
        int tid_actual = list_get(pcb_en_ejecucion->tids, i);
        if (tid_actual == tid) hayHilo = true;
            
    }
    
    if(!hayHilo){
        log_error(logger,"EL TID PASADO NO EXISTE O YA FINALIZÓ, NO SE BLOQUEA");
        return 0;
    }
    
    cambiar_estado_hilo(tcb,BLOCKED);
    tcb->bloqueadoPor = tid;
    encolar(cola_blocked,tcb,mutex_blocked);
    //sem_post(&sem_hay_ready);
    return 1;
}



void desbloquear_bloqueados_por_hilo(int tidBloqueante,int pidhilo){
    t_queue* cola_aux = queue_create();
    while(!queue_is_empty(cola_blocked)){
        void* elemento = desencolar(cola_blocked,mutex_blocked);
        TCB* tcb = (TCB*) elemento;
        if(tcb->bloqueadoPor==tidBloqueante && tcb->pcb_pid == pidhilo){
            desbloquear_hilo(tcb);
        }else{
            queue_push(cola_aux,elemento);
        }
    }
    while(!queue_is_empty(cola_aux)){
        queue_push(cola_blocked,queue_pop(cola_aux));
    }
    
    queue_destroy(cola_aux);
}

void desbloquear_hilo(TCB *tcb){
    cambiar_estado_hilo(tcb,READY);
    replanificar(tcb);
}

void imprimir_cola(t_queue *cola, pthread_mutex_t mutex){
    t_queue* cola_aux = queue_create();
    printf("Cola: ");
    while(!queue_is_empty(cola)){
        void* elemento = desencolar(cola, mutex);

        TCB* tcb = (TCB*) elemento;
        log_trace(logger,"COLA NEW TID: %i; ", tcb->tid);
        queue_push(cola_aux,elemento);
    }
    while(!queue_is_empty(cola_aux)){
        queue_push(cola,queue_pop(cola_aux));
    }
    printf("\n");
    queue_destroy(cola_aux);
}

void imprimir_cola_new(t_queue *cola, pthread_mutex_t mutex){
    t_queue* cola_aux = queue_create();
    printf("Cola: ");
    while(!queue_is_empty(cola)){
        void* elemento = desencolar(cola, mutex);

        PCB* pcb = (PCB*) elemento;
        log_trace(logger,"COLA NEW PID: %i; ", pcb->pid);
        queue_push(cola_aux,elemento);
    }
    while(!queue_is_empty(cola_aux)){
        queue_push(cola,queue_pop(cola_aux));
    }
    printf("\n");
    queue_destroy(cola_aux);
}

// void encolar_multinivel(COLA_PRIORIDAD *cola, TCB *tcb){
//     encolar(cola->cola_prioridad, tcb, cola->mutex);

// }

bool buscar_en_cola_prioridad(COLA_PRIORIDAD *cola_prioridad, int pid){
    return buscar_en_cola(cola_prioridad->cola_prioridad, cola_prioridad->mutex, pid);
}

// devuelve 1 si existe un tcb del proceso
bool buscar_en_cola(t_queue *cola, pthread_mutex_t mutex, int pid){
    t_queue* cola_aux = queue_create();
    int respuesta = 0;
    while(!queue_is_empty(cola)){
        void* elemento = desencolar(cola, mutex);

        TCB* tcb = (TCB*) elemento;
        if (tcb->pcb_pid == pid) {
            respuesta = 1;
            log_warning(logger, "Tcb encontrado en buscar_cola: %i", tcb->tid);
        }
        queue_push(cola_aux,elemento);
    }
    while(!queue_is_empty(cola_aux)){
        queue_push(cola,queue_pop(cola_aux));
    }
    queue_destroy(cola_aux);
    log_warning(logger, "Respuesta de buscar_en_cola: %i", respuesta);
    return respuesta;
}

void ordenar_cola(t_queue *cola, pthread_mutex_t mutex) {
    t_list *lista = list_create();
    while (!queue_is_empty(cola)) {
        list_add(lista, desencolar(cola, mutex));
    }
    list_sort(lista, (void *)comparar_prioridades);

    for (int i = 0; i < list_size(lista); i++) {
        encolar(cola, list_get(lista, i), mutex);
    }
    list_destroy(lista);
}

void vaciar_colas_prioridades(int pid) {
    for (int i = 0; i < list_size(colas_prioridades); i++) {

        COLA_PRIORIDAD *cola_prioridad = list_get(colas_prioridades, i);
        t_queue *cola_aux = queue_create();
        if (queue_is_empty(cola_prioridad->cola_prioridad)) continue;

        TCB *tcb = desencolar_multinivel(cola_prioridad);
        
        if (tcb->tid == 0) encolar_multinivel(cola_prioridad,tcb);

        while (tcb && tcb->tid != 0) {
            if(tcb->pcb_pid != pid){
                queue_push(cola_aux,tcb);
                log_error(logger,"PID: %d TID:%d NO SE VACIA",tcb->pcb_pid,tcb->tid);
            }else{
                liberar_tcb(tcb);
            }
            if (queue_is_empty(cola_prioridad->cola_prioridad)) break;

            tcb = desencolar_multinivel(cola_prioridad);
        }
        while(!queue_is_empty(cola_aux)){
            encolar_multinivel(cola_prioridad,queue_pop(cola_aux));
        }
        queue_destroy(cola_aux);
    }
}