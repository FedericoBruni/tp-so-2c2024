#include "pcb.h"

extern int autoincremental_pcb;
extern t_log *logger;
extern char* algoritmo_planificacion;
extern PCB *pcb_en_ejecucion;
extern TCB *tcb_en_ejecucion;
extern t_queue *cola_new;
extern t_queue *cola_ready;
extern t_queue *cola_finalizacion;
extern t_queue *cola_new_hilo;
extern t_queue *cola_fin_pcb;
extern t_queue *cola_blocked;
extern t_list* colas_prioridades;
extern pthread_mutex_t mutex_liberar_tcb;
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_blocked;


void liberar_tcb(void *ptr_tcb)
{
    TCB *tcb = (TCB *)ptr_tcb;
    //log_warning(logger, "Liberando <TID: %i>", tcb->tid);
    liberar_registros(tcb->Registros);
    if(tcb_en_ejecucion != NULL){
    if(tcb->tid == tcb_en_ejecucion->tid && tcb->pcb_pid == tcb_en_ejecucion->pcb_pid){
        tcb_en_ejecucion = NULL;  // NO TOCAR ROMPE TODO
    }}

    if((tcb->archivo_pseudocodigo && tcb->pcb_pid != 0) || tcb->tid != 0  ){
     free(tcb->archivo_pseudocodigo);
    }

    if(tcb->status != EXIT){
        TCB *tcb_encolado;
    if(string_equals_ignore_case(algoritmo_planificacion,"MULTINIVEL")){
        tcb_encolado = buscar_tcb_en_multinivel(tcb->tid,tcb->pcb_pid);
    }else{
        tcb_encolado = buscar_tcb_en_cola(cola_ready,mutex_ready,tcb->tid,tcb->pcb_pid);
    }
    if(tcb_encolado == NULL){
        tcb_encolado = buscar_tcb_en_cola(cola_blocked,mutex_blocked,tcb->tid,tcb->pcb_pid);
        if(tcb_encolado == NULL){
            if(tcb_en_ejecucion != NULL){
            if(tcb_en_ejecucion->tid == tcb->tid && tcb_en_ejecucion->pcb_pid==tcb->pcb_pid){
                tcb_encolado=tcb_en_ejecucion;
            }else{
            log_trace(logger,"No se encontro el hilo (PID:TID) - (<%i>:<%i>)", tcb->pcb_pid, tcb->tid);
            }}
        }   
    }
        //free(tcb_encolado);
    }


    free(tcb);
    
}


void liberar_registros(REGISTROS *registros)
{
    REGISTROS *registros_a_liberar = (REGISTROS *)registros;
    free(registros_a_liberar);
}

void liberar_pcb(void *ptr_pcb)
{
    PCB *pcb = (PCB *)ptr_pcb;
    list_destroy(pcb->mutex); // and destroy elements? cada elemento es un mutex con memoria dinamica??
    //log_warning(logger, "Liberando <PID:%i>, con <%i THREADS>", pcb->pid, list_size(pcb->threads));
    
    if(pcb->pid == pcb_en_ejecucion->pid){
        pcb_en_ejecucion =NULL;
    }

    //liberar_tcb(list_get(pcb->threads, 0));
    pthread_mutex_lock(&mutex_liberar_tcb);
    list_destroy_and_destroy_elements(pcb->threads, liberar_tcb);
    pthread_mutex_unlock(&mutex_liberar_tcb);
    //list_destroy(pcb->threads);
    


    //list_destroy_and_destroy_elements(pcb->threads, liberar_tcb); lo saque pq rompe. en el test siempre va a haber un único thread cuando se llame a process exit, el 0
    list_destroy(pcb->tids);
    free(pcb);
}

PCB *crear_pcb(char *archivo_pseudocodigo, int tamanio_memoria, int prioridad_main)
{
    PCB *pcb = malloc(sizeof(PCB));
    pcb->pid = autoincremental_pcb;
    autoincremental_pcb++;
    //log_error(logger,"AUTOINCREMENTAL PCB: %d",autoincremental_pcb);
    pcb->status = NEW;
    pcb->tids = list_create();
    pcb->autoincremental_tcb = 0;
    pcb->threads = list_create();
    pcb->mutex = list_create();
    pcb->prioridad_main = prioridad_main;
    pcb->archivo_pseudocodigo = archivo_pseudocodigo;
    pcb->tamanio = tamanio_memoria;
    pcb->prioridad_main = prioridad_main;
    pcb->BASE =0;
    pcb->LIMITE =0;
    return pcb;
}

TCB *crear_tcb(PCB *pcb, int prioridad, char* archivo_pseudocodigo)
{
    TCB *tcb = malloc(sizeof(TCB));
    tcb->tid = pcb->autoincremental_tcb;
    pcb->autoincremental_tcb = pcb->autoincremental_tcb +1;
    tcb->pcb_pid = pcb->pid;
    tcb->prioridad = prioridad;
    tcb->Registros = malloc(sizeof(REGISTROS));
    inicializar_registros(tcb->Registros);
    tcb->pcb = pcb;
    tcb->archivo_pseudocodigo = archivo_pseudocodigo;
    tcb->bloqueadoPor = NULL;
    agregar_hilo(tcb,pcb);
    log_trace(logger,"## (PID) - (<%d>) - Creando Hilo <%d> - Prioridad: <%d>", pcb->pid, tcb->tid, prioridad);
    return tcb;
}

void agregar_hilo(TCB *tcb, PCB *pcb)
{
    list_add(pcb->tids, tcb->tid);
    list_add(pcb->threads, tcb);
}

char *desc_status[] = {"NEW", "READY", "EXEC", "BLOCKED", "EXIT"};

void imprimir_pcb(PCB *pcb)
{
    printf("PCB ID: %i\n", pcb->pid);
    printf("Archivo a leer: %s\n", pcb->archivo_pseudocodigo);
    printf("Tamaño del archivo: %i\n", pcb->tamanio);
    printf("LISTA TIDS: \n");
    imprimir_lista_ids(pcb->tids);
    printf("STATUS: %s\n", desc_status[pcb->status]);
    printf("THREADS: \n");
    //imprimir_hilos(pcb->threads);
    printf("TAMANIO LISTA MUTEX: %i\n", list_size(pcb->mutex));
}

void imprimir_registros(REGISTROS *registros)
{
    printf("PC: %i\nAX: %i\nBX: %i\nCX: %i\nDX: %i\nEX: %i\nFX: %i\nGX: %i\nHX: %i\nBASE: %i\nLIMITE: %i\n", registros->PC, registros->AX,
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
void imprimir_pcb_sin_hilos(PCB *pcb)
{
    printf("PCB ID: %i\n", pcb->pid);
    printf("Archivo a leer: %s\n", pcb->archivo_pseudocodigo);
    printf("Tamaño del archivo: %i\n", pcb->tamanio);
    printf("LISTA TIDS: \n");
    imprimir_lista_ids(pcb->tids);
    printf("STATUS: %s\n", desc_status[pcb->status]);
}

void imprimir_hilos(t_list *threads)
{
    int size = list_size(threads);
    for (unsigned int i = 0; i < size; i++)
    {
        TCB *tcb = list_get(threads, i);
        printf("TID: %i\n", tcb->tid);
        printf("Prioridad: %i\n", tcb->prioridad);
        printf("STATUS: %s\n", desc_status[tcb->status]);
        printf("ARCHIVO A LEER: %s\n", tcb->archivo_pseudocodigo);
        printf("REGISTROS DEL HILO: \n");
        imprimir_registros(tcb->Registros);
        printf("DATOS DEL PROCESO PADRE: \n");
        imprimir_pcb_sin_hilos(tcb->pcb);
    }
}

void imprimir_hilo(TCB* tcb)
{
    printf("TID: %i\n", tcb->tid);
    printf("Prioridad: %i\n", tcb->prioridad);
    printf("STATUS: %s\n", desc_status[tcb->status]);
    printf("ARCHIVO A LEER: %s\n", tcb->archivo_pseudocodigo);
    printf("REGISTROS DEL HILO: \n");
    imprimir_registros(tcb->Registros);
    printf("DATOS DEL PROCESO PADRE: \n");
    imprimir_pcb_sin_hilos(tcb->pcb);
}

void inicializar_registros(REGISTROS *registros)
{
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

void cambiar_estado_hilo(TCB *tcb, STATUS estado)
{
    tcb->status = estado;
}


TCB* buscar_tcb_en_cola(t_queue* cola, pthread_mutex_t mutex,int tid, int pid){
    t_queue* cola_aux = queue_create();
    TCB* rta = NULL;
    while(!queue_is_empty(cola)){
        void* elemento = desencolar(cola,mutex);

        TCB* tcb = (TCB*) elemento;
        if(tcb->tid == tid && tcb->pcb_pid == pid){
            rta = tcb;
        }else{
            queue_push(cola_aux,elemento);
        }
    }
    while(!queue_is_empty(cola_aux)){
        encolar(cola,queue_pop(cola_aux),mutex);
    }
    queue_destroy(cola_aux);
    return rta;
}

TCB *buscar_tcb_en_multinivel( int tid, int pid){
    t_queue *cola_aux = queue_create();
    TCB* rta = NULL;
    for(int i = 0; i<list_size(colas_prioridades);i++){
        COLA_PRIORIDAD *cola_prio = list_get(colas_prioridades,i);
        while(!queue_is_empty(cola_prio->cola_prioridad)){
            void *elemento = desencolar_multinivel(cola_prio);
            TCB *tcb = (TCB*) elemento;
            if(tcb->tid == tid && tcb->pcb_pid == pid){
                rta=tcb;
            }else{
                queue_push(cola_aux,elemento);
            }
        }

        while(!queue_is_empty(cola_aux)){
            encolar_multinivel(cola_prio,queue_pop(cola_aux));
        }
    }
    queue_destroy(cola_aux);
    return rta;
}

