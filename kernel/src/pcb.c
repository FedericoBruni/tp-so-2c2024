#include "pcb.h"

extern int autoincremental_pcb;
extern t_log *logger;

void liberar_tcb(void *ptr_tcb)
{
    TCB *tcb = (TCB *)ptr_tcb;
    liberar_registros(tcb->Registros);
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
    //liberar_registros(pcb->Registros); // libero registros
    list_destroy(pcb->mutex); // and destroy elements? cada elemento es un mutex con memoria dinamica??
    free(pcb->tids);
    list_destroy_and_destroy_elements(pcb->threads, liberar_tcb);
    free(pcb);
}

PCB *crear_pcb(char *archivo_pseudocodigo, int tamanio_memoria, int prioridad_main)
{
    PCB *pcb = malloc(sizeof(PCB));
    pcb->pid = autoincremental_pcb;
    autoincremental_pcb++;
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
    log_info(logger,"Creando TCB de prioridad: %i\n", prioridad);
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
    imprimir_hilos(pcb->threads);
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