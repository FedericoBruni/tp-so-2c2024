#include "utilidades.h"
#include <signal.h>

extern t_log *logger;
char *archivo_pseudocodigo;
int tamanio_proceso;
extern t_list *cola_new;
bool hilo_creacion_muerto;
bool hilo_fin_proc_muerto;
int fd_cpu_dispatch;
int fd_cpu_interrupt;
extern sem_t sem_syscall_fin;
extern sem_t sem_fin_ejecucion;
pthread_t hilo_io;
pthread_t hilo_creacion_de_procesos;
pthread_t hilo_finalizacion_de_procesos;
pthread_t hilo_creacion_de_hilos;
pthread_t hilo_finalizacion_hilos;
pthread_t hilo_planificador_corto_plazo;
pthread_t hilo_terminar;

void *handler_señal(int signal){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);  // Bloquea SIGINT
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_create(&hilo_terminar,NULL,terminar_ejecucion,NULL);
    pthread_join(hilo_terminar,NULL);
    sigemptyset(&set);
    sigaddset(&set, SIGINT);  // Desbloquea SIGINT
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    exit(EXIT_SUCCESS);
}



int main(int argc, char *argv[])
{

    iniciar_kernel();
    signal(SIGINT,handler_señal);

    if (argc < 3)
    {
        exit(EXIT_FAILURE);
    }
    archivo_pseudocodigo = argv[1];
    tamanio_proceso = atoi(argv[2]);

    // Conectarse a memoria
    int fd_memoria = conectarse_a_memoria();
    // Conectarse a cpu dispatch
    fd_cpu_dispatch = conectarse_a_cpu_dispatch();
    // Conectarse a cpu interrupt
    fd_cpu_interrupt = conectarse_a_cpu_interrupt();

    // Handshakes
    if (realizar_handshake(logger, fd_memoria, HANDSHAKE_KERNEL_MEMORIA) == -1)
    {
        exit(EXIT_FAILURE);
    }
    close(fd_memoria);
    if (realizar_handshake(logger, fd_cpu_dispatch, HANDSHAKE_KERNEL_CPU_DISPATCH) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (realizar_handshake(logger, fd_cpu_interrupt, HANDSHAKE_KERNEL_CPU_INTERRUPT) == -1)
    {
        exit(EXIT_FAILURE);
    }    

    
    pthread_create(&hilo_creacion_de_procesos, NULL, (void *)creacion_de_procesos, NULL); // Crea el hilo y le pasa la funcion a ejecutarse
    pthread_detach(hilo_creacion_de_procesos);

    
    pthread_create(&hilo_finalizacion_de_procesos, NULL, (void *)finalizacion_de_procesos, NULL);
    pthread_detach(hilo_finalizacion_de_procesos);

    
    pthread_create(&hilo_creacion_de_hilos, NULL, (void *)creacion_de_hilos, NULL);
    pthread_detach(hilo_creacion_de_hilos);

    
    pthread_create(&hilo_finalizacion_hilos,NULL,(void*) finalizacion_de_hilos,NULL);
    pthread_detach(hilo_finalizacion_hilos);

    // [512, 16, 32, 16, 256, 64, 128]
    SYS_PROCESS_CREATE(archivo_pseudocodigo, tamanio_proceso, 0);
    sem_wait(&sem_syscall_fin);
    // PROCESS_CREATE("archivo2", 100, 0);
    // PROCESS_CREATE("archivo3",60, 5);
    // PROCESS_CREATE("archivo4", 353, 3);
    // PROCESS_CREATE("archivo4", 50, 6);
    // PROCESS_CREATE("archivo4", 50, 6);



    //pthread_t hilo_io;
    pthread_create(&hilo_io, NULL, (void *)ejecucion_io, NULL);
    pthread_detach(hilo_io);

    
    pthread_create(&hilo_planificador_corto_plazo, NULL, (void *)planificador_corto_plazo, NULL);
    pthread_join(hilo_planificador_corto_plazo,NULL);



    //terminar_ejecucion(fd_cpu_dispatch, fd_memoria, fd_cpu_interrupt);
    sem_wait(&sem_fin_ejecucion);
    return 0;
}
