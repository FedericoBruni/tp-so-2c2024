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
int autoincremental_pcb = 0;
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_blocked;
sem_t sem_hay_memoria;
sem_t sem_crear_hilo;
sem_t sem_finalizar_proceso;
sem_t sem_finalizar_hilo;
PCB *pcb_en_ejecucion;
TCB *tcb_a_crear = NULL;




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
    cola_finalizacion = queue_create();
    iniciar_semaforos();
}

void iniciar_semaforos(void){
    inicializar_mutex(&mutex_new, "New");
    inicializar_mutex(&mutex_ready, "Ready");
    inicializar_mutex(&mutex_blocked, "Blocked");
    inicializar_mutex(&mutex_exec, "Execute");
    inicializar_mutex(&mutex_exit, "Exit");

    inicializar_semaforo(&sem_hay_memoria, "Hay memoria", 0);
    inicializar_semaforo(&sem_crear_hilo, "Crear hilo", 0);
    inicializar_semaforo(&sem_finalizar_proceso, "Finalizar proceso", 0);
    inicializar_semaforo(&sem_finalizar_hilo, "Finalizar hilo", 0);

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



// typedef struct
// {
// 	uint32_t PC;
// 	uint32_t AX;
// 	uint32_t BX;
// 	uint32_t CX;
// 	uint32_t DX;
// 	uint32_t EX;
// 	uint32_t FX;
// 	uint32_t GX;
// 	uint32_t HX;
// } REGISTROS;

// typedef struct
// {

// 	int pid;
// 	t_list *tids;
// 	t_list *mutex;
// 	STATUS status;
// 	REGISTROS Registros;

// } PCB;

// typedef struct
// {
// 	int tid;
// 	int prioridad;
// 	REGISTROS Registros;
// } TCB;