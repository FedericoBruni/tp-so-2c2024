#include "syscalls.h"

    extern t_list* cola_new;
/*
PROCESS_CREATE, esta syscall recibirá 3 parámetros de la CPU, el primero será el nombre del archivo de pseudocódigo que 
deberá ejecutar el proceso, el segundo parámetro es el tamaño del proceso en Memoria y el tercer parámetro es la prioridad 
del hilo main (TID 0). 
El Kernel c: 12
[INFO] 20:reará un nuevo PCB y un TCB asociado con TID 0 y lo dejará en estado NEW.
*/

void PROCESS_CREATE(char* archivo,int tamanio_memoria,int prioridad){
    PCB* pcb_inicial = crear_pcb(archivo,tamanio_memoria,prioridad); // dsp agregar arch_pseudocodigo y tam_proceso
    list_add(cola_new,pcb_inicial);
    //signal
    // TCB* tcb_inicial = crear_tcb(pcb_inicial,prioridad); // dsp agregar tcb_inicial
    // agregar_hilo(tcb_inicial,pcb_inicial);
    
}

