#include "utilidades.h"

t_log *logger;
t_config *config;
char *ip_memoria;
char *puerto_memoria;
char *ip_cpu;
char *puerto_cpu_dispatch;
char *puerto_cpu_interrupt;
char *algoritmo_planificacion;
int *quantum;
char *log_level;
t_list *cola_new;
t_list *cola_ready;
t_list *cola_blocked;
t_list *cola_exit;
int autoincremental_pcb = 0;

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
    cola_new = list_create();
    cola_ready = list_create();
    cola_blocked = list_create();
    cola_exit = list_create();
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
    log_info(logger, "Finalizando ejecucion de KERNEL");
    close(dispatch);
    close(memoria);
    close(interrupt);
    log_destroy(logger);
    config_destroy(config);
    list_destroy_and_destroy_elements(cola_new, destruir_pcb);
    list_destroy_and_destroy_elements(cola_ready, destruir_pcb);
    list_destroy_and_destroy_elements(cola_blocked, destruir_pcb);
    list_destroy_and_destroy_elements(cola_exit, destruir_pcb);
}

/* Ejemplo
void person_destroy(void *ptr)
{
    t_person *person = (t_person *)ptr;
    free(person->name);
    free(person);
} */

void destruir_tcb(void *ptr_tcb)
{
    TCB *tcb = (TCB *)ptr_tcb;
    // free(tcb->prioridad);
    // free(tcb->tid);
    free(tcb);
}

void destruir_pcb(void *ptr_pcb)
{
    PCB *pcb = (PCB *)ptr_pcb;
    list_destroy_and_destroy_elements(pcb->tids, destruir_tcb);
    // funcion para destruir mutex
    // list_destroy(pcb->mutexes);
    // funcion para destruir registros
    // free(pcb->pid);
    free(pcb);
}

int solicitar_memoria(int socket_memoria, int tamanio_memoria, op_code cod_sol)
{

    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, tamanio_memoria);
    t_paquete *paquete = crear_paquete(cod_sol, buffer);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    if (recibir_operacion(socket_memoria) == OK_SOLICITUD_MEMORIA_PROCESO)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

PCB *crear_pcb(char* archivo,int tamanio_memoria,int prioridad_main)
{
    PCB *pcb = malloc(sizeof(PCB));
    pcb->pid = autoincremental_pcb;
    autoincremental_pcb++;
    pcb->status = NEW;
    pcb->Registros = malloc(sizeof(REGISTROS));
    inicializar_registros(pcb->Registros);
    pcb->tids = list_create();
    pcb->autoincremental_tcb = 0;
    pcb->threads = list_create();
    pcb->prioridad_main = prioridad_main;
    pcb->archivo = archivo;
    pcb->tamanio = tamanio_memoria;
    pcb->prioridad_main = prioridad_main;
    return pcb;
}


TCB *crear_tcb(PCB *pcb, int prioridad)
{
    TCB *tcb = malloc(sizeof(TCB));

    tcb->tid = pcb->autoincremental_tcb;
    pcb->autoincremental_tcb++;
    tcb->pcb_pid = pcb->pid;
    if(tcb->tid == 0){
        tcb->prioridad = pcb->prioridad_main;
    }else{
        tcb->prioridad = prioridad;}
    tcb->Registros = pcb->Registros;
    return tcb;
}

void agregar_hilo(TCB *tcb,PCB *pcb){
    list_add(pcb->tids,tcb->tid);
    list_add(pcb->threads, tcb);
}

// int pid;
// t_list *tids;
// t_list *mutex;
// STATUS status;
// REGISTROS Registros;
// int autoincremental_tcb;
// t_list *threads;
char *desc_status[] = {"NEW", "READY", "EXEC", "BLOCKED", "EXIT"};

void imprimir_pcb(PCB *pcb)
{
    printf("PCB ID: %i\n", pcb->pid);
    printf("Archivo a leer: %s\n",pcb->archivo);
    printf("Tamaño del archivo: %i\n",pcb->tamanio);
    printf("LISTA TIDS: \n");
    imprimir_lista_ids(pcb->tids);
    printf("STATUS: %s\n", desc_status[pcb->status]);
    printf("REGISTROS: \n");
    imprimir_registros(pcb->Registros);
    printf("THREADS: \n");
    imprimir_hilos(pcb->threads);
}

void imprimir_registros(REGISTROS *registros)
{
    printf("PC: %i\nAX: %i\nBX: %i\nCX: %i\nDX: %i\nEX: %i\nFX: %i\nGX: %i\nHX: %i\n", registros->PC, registros->AX,
    registros->BX, registros->CX, registros->DX, registros->EX, registros->FX, registros->GX, registros->HX);
}

void imprimir_lista_ids(t_list *tids)
{
    int size = list_size(tids);
    for (unsigned int i = 0; i < size; i++)
    {
        printf("TID: %i\n", list_get(tids, i));
    }
}

void imprimir_hilos(t_list *threads)
{
    int size = list_size(threads);
    for (unsigned int i = 0; i < size; i++)
    {
        TCB* tcb = list_get(threads, i);
        printf("TID: %i\n", tcb->tid);
        printf("Prioridad: %i\n", tcb->prioridad);
        printf("REGISTROS DEL HILO: \n");
        imprimir_registros(tcb->Registros);
    }
}


void inicializar_registros(REGISTROS* registros){
    registros->PC = 0;
    registros->AX = 0;
    registros->BX = 0;
    registros->CX = 0;
    registros->DX = 0;
    registros->EX = 0;
    registros->FX = 0;
    registros->GX = 0;
    registros->HX = 0;
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