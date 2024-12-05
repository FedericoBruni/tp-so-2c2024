#include "utilidades.h"

t_log *logger;
t_config *config;
char *puerto_escucha;
char *ip_filesystem;
char *puerto_filesystem;
int *tam_memoria;
char *path_instrucciones;
int *retardo_respuesta;
char *esquema;
char *algoritmo_busqueda;
char **particiones;
char *log_level;
t_list *contextos_procesos;
t_list *contextos_hilos;
t_list *archivos;
MEMORIA_USUARIO* memoria_usuario;
extern int cliente_fd_kernel;
extern int cliente_fd_dispatch;

void test();

void iniciar_memoria()
{
    config = iniciar_config("memoria.config");
    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
    puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");
    tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
    esquema = config_get_string_value(config, "ESQUEMA");
    algoritmo_busqueda = config_get_string_value(config, "ALGORITMO_BUSQUEDA");
    particiones = config_get_array_value(config, "PARTICIONES");
    log_level = config_get_string_value(config, "LOG_LEVEL");
    logger = iniciar_logger("memoria.log", "Memoria", log_level_from_string(log_level));
    contextos_procesos = list_create();
    contextos_hilos = list_create();
    archivos = list_create();
    cargar_memoria_usuario();

}

int conectarse_a_filesystem(void)
{
    return crear_conexion(ip_filesystem, puerto_filesystem, logger);
}

void terminar_ejecucion(int socket_conexion, int socket_servidor_kernel, int socket_servidor_cpu)
{
    log_info(logger, "Finalizando ejecución de MEMORIA");
    close(socket_conexion);
    close(socket_servidor_kernel);
    close(socket_servidor_cpu);
    log_destroy(logger);
    config_destroy(config);
    exit(EXIT_SUCCESS);
}

void imprimir_ctx_cpu(CONTEXTO_CPU *contexto_cpu) {
    printf("CTX de <PID: %i>:<TID: %i>\n", contexto_cpu->contexto_hilo->pid, contexto_cpu->contexto_hilo->tid);
    printf("REGISTROS: PC: %i, AX: %i, BX: %i, CX: %i, DX: %i\n", contexto_cpu->contexto_hilo->Registros->PC, contexto_cpu->contexto_hilo->Registros->AX,
    contexto_cpu->contexto_hilo->Registros->CX, contexto_cpu->contexto_hilo->Registros->DX);
    printf("REGISTROS: EX: %i, FX: %i, GX: %i, HX: %i, BASE: %i, LIMITE: %i\n", contexto_cpu->contexto_hilo->Registros->EX, contexto_cpu->contexto_hilo->Registros->FX,
    contexto_cpu->contexto_hilo->Registros->GX, contexto_cpu->contexto_hilo->Registros->HX, contexto_cpu->contexto_proceso->BASE,
    contexto_cpu->contexto_proceso->LIMITE);
    printf("--------------------------------------------------\n");
}

void imprimir_ctx_hilo(CONTEXTO_HILO *contexto_hilo) {
    printf("CTX de <PID: %i>:<TID: %i>\n",contexto_hilo->pid,contexto_hilo->tid);
    printf("REGISTROS: PC: %i, AX: %i, BX: %i, CX: %i, DX: %i\n", contexto_hilo->Registros->PC, contexto_hilo->Registros->AX,
    contexto_hilo->Registros->CX, contexto_hilo->Registros->DX);
    printf("REGISTROS: EX: %i, FX: %i, GX: %i, HX: %i\n", contexto_hilo->Registros->EX, contexto_hilo->Registros->FX,
    contexto_hilo->Registros->GX, contexto_hilo->Registros->HX);
    printf("--------------------------------------------------\n");
}

void enviar_contexto(int cliente_fd_dispatch){
    t_buffer *buffer = recibir_buffer_completo(cliente_fd_dispatch);
    
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);

    log_info(logger, "## Contexto Solicitado - (PID:TID) - (%d:%d)",pid,tid);

    CONTEXTO_CPU *contexto_cpu = malloc(sizeof(CONTEXTO_CPU));
    contexto_cpu = buscar_contextos(tid,pid);


    
    
    t_buffer *bufferRta = crear_buffer();
    cargar_contexto_hilo(bufferRta, contexto_cpu->contexto_hilo);
    cargar_contexto_proceso(bufferRta, contexto_cpu->contexto_proceso);
    int cod_rta = CONTEXTO_ENVIADO;
    t_paquete *paquete = crear_paquete(cod_rta,bufferRta);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);

    //enviar_contexto(cliente_fd_dispatch,contexto_cpu);
    //int rta_sol_mem = CONTEXTO_ENVIADO;
    //send(cliente_fd_dispatch, &rta_sol_mem, sizeof(op_code), 0);
    
}

void enviar_instruccion(int cliente_fd_dispatch) {
    t_buffer *buffer = recibir_buffer_completo(cliente_fd_dispatch);
    int pc = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);
    int tid = extraer_int_del_buffer(buffer);

    char* instruccion = obtener_instruccion(pc, pid, tid);

    log_info(logger,"## Obtener instrucción - (PID:TID) - (%d:%d) - Instrucción: %s",pid,tid,instruccion);

    t_buffer *bufferRta = crear_buffer();
    t_paquete *paquete;
    if (!instruccion) {
        //paquete = crear_paquete(EOF_INSTRUCCION, bufferRta);
        int resp_eof = EOF_INSTRUCCION;
        send(cliente_fd_dispatch, &resp_eof, sizeof(op_code), 0);
    } else {
        
        cargar_string_al_buffer(bufferRta, instruccion);
        paquete = crear_paquete(PROXIMA_INSTRUCCION, bufferRta);
        enviar_paquete(paquete, cliente_fd_dispatch);
        eliminar_paquete(paquete);
        
    }
    
    

}

CONTEXTO_CPU* buscar_contextos(int tid, int pid){
    bool _hayHilo(void *ptr){
        CONTEXTO_HILO *ctx_hilo = (CONTEXTO_HILO*) ptr;
        return ctx_hilo->tid == tid && ctx_hilo->pid == pid;
    }

    bool _hayProceso(void *ptr2){
        CONTEXTO_PROCESO *ctx_proc = (CONTEXTO_PROCESO*) ptr2;
        return ctx_proc->pid == pid;
    }

    CONTEXTO_HILO *contexto_hilo = list_find(contextos_hilos,_hayHilo);
    CONTEXTO_PROCESO *contexto_proceso = list_find(contextos_procesos, _hayProceso);

    CONTEXTO_CPU *contexto_cpu = malloc(sizeof(CONTEXTO_CPU));
    contexto_cpu->contexto_hilo = contexto_hilo;
    contexto_cpu->contexto_proceso = contexto_proceso;
    return contexto_cpu;
}

CONTEXTO_PROCESO *buscar_contexto_proceso(int pid){
    bool _hayProceso(void *ptr2){
        CONTEXTO_PROCESO *ctx_proc = (CONTEXTO_PROCESO*) ptr2;
        return ctx_proc->pid == pid;
    }
    CONTEXTO_PROCESO *contexto_proceso = list_find(contextos_procesos, _hayProceso);
    return contexto_proceso;
}
CONTEXTO_HILO *buscar_contexto_hilo(int pid,int tid){
    bool _hayHilo(void *ptr2){
        CONTEXTO_HILO *ctx_proc = (CONTEXTO_HILO*) ptr2;
        return ctx_proc->pid == pid && ctx_proc->tid == tid;
    }
    CONTEXTO_HILO *contexto_hilo = list_find(contextos_hilos, _hayHilo);
    return contexto_hilo;
}



void actualizar_contexto(int cliente_fd_dispatch){
    
    t_buffer *buffer = recibir_buffer_completo(cliente_fd_dispatch);
    CONTEXTO_HILO *ctx_hilo = malloc(sizeof(CONTEXTO_HILO));
    CONTEXTO_PROCESO *ctx_proceso = malloc(sizeof(CONTEXTO_PROCESO));
    ctx_hilo = extraer_contexto_hilo(buffer);
    ctx_proceso = extraer_contexto_proceso(buffer);
    
    log_info(logger, "## Contexto Actualizado - (PID:TID) - (%d:%d)",ctx_hilo->pid,ctx_hilo->tid);

    bool _es_hilo(void *ptr) {
        CONTEXTO_HILO *ctx_hilo_en_memoria = (CONTEXTO_HILO *)ptr;
        return ctx_hilo_en_memoria->tid == ctx_hilo->tid && ctx_hilo_en_memoria->pid == ctx_hilo->pid;
    }

    bool _es_proceso(void *ptr) {
        CONTEXTO_PROCESO *ctx_proceso_en_memoria = (CONTEXTO_PROCESO *)ptr;
        return ctx_proceso_en_memoria->pid == ctx_proceso->pid;
    }

    CONTEXTO_HILO *hilo_actual = list_find(contextos_hilos, _es_hilo);
    CONTEXTO_PROCESO *proceso_actual = list_find(contextos_procesos, _es_proceso);

    if (hilo_actual && proceso_actual) {
        memcpy(hilo_actual->Registros, ctx_hilo->Registros, sizeof(REGISTROS));
        log_info(logger, "Contexto de hilo y proceso actualizado para TID: %d y PID: %d", ctx_hilo->tid, ctx_hilo->pid);
    } else {
        log_error(logger, "No se encontro el contexto para TID: %d y PID: %d", ctx_hilo->tid, ctx_hilo->pid);
    }

    int cod_op = CONTEXTO_ACTUALIZADO_OK;
    send(cliente_fd_dispatch, &cod_op, sizeof(cod_op), 0);
}


void imprimir_elemento(char *key, void *element) {
    printf("Clave: %s, Valor: %s\n", key, (char *)element);
}

void cargar_archivo(char* nombre_archivo, int tid,int pid) {
    // Abrirlo
    // Parsearlo
    // Meterlo en el diccionario
    // Clave (PC) -> Valor (INSTRUCCIÓN)
    t_dictionary* diccionario = dictionary_create();
    int total_length = strlen(path_instrucciones) + strlen(nombre_archivo) + 2; // El "/" antes de la ruta, y el '\0' final
    char ruta_archivo[total_length];
    strcpy(ruta_archivo, path_instrucciones);
    strcat(ruta_archivo, "/");
    strcat(ruta_archivo, nombre_archivo);



    FILE* archivo = fopen(ruta_archivo, "r");
    if(archivo == NULL){
        log_error(logger, "Error al abrir el archivo");
        return -1;
    }

    char linea[256];
    int i = 0;
    char clave[256];
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        snprintf(clave, 256, "%d", i);
        char* key = clave;
        dictionary_put(diccionario, key, strdup(linea));
        i++;
        //printf("%s", linea);
    }
    fclose(archivo);
    // printf("Diccionario\n");
    // dictionary_iterator(diccionario, imprimir_elemento);
    CONTEXTO_ARCHIVO* archivo_instrucciones = malloc(sizeof(CONTEXTO_ARCHIVO));
    archivo_instrucciones->tid = tid;
    archivo_instrucciones->pid = pid;
    archivo_instrucciones->instrucciones = diccionario;
    list_add(archivos, archivo_instrucciones);
}



void leer(CONTEXTO_ARCHIVO* archivo){
    dictionary_iterator(archivo->instrucciones,imprimir_elemento);
}
void leer_archivos(){
    list_iterate(archivos,leer);
}

int obtener_tamano(char** lista) {
    int size = 0;
    while (lista[size] != NULL) {
        size++;
    }
    return size;
}

void cargar_memoria_usuario() {
    memoria_usuario = malloc(sizeof(MEMORIA_USUARIO));
    memoria_usuario->memoria_usuario = malloc(tam_memoria);
    memoria_usuario->particiones = list_create();
    if (string_equals_ignore_case(esquema, "FIJAS")) {
        

        // PARTICIONES=[512, 16, 32, 16, 256, 64, 128]
        int particiones_size = obtener_tamano(particiones);

        
        int actual = 0;
        for (int i = 0; i < particiones_size; i++) {
            Particion* particion = malloc(sizeof(Particion));
            particion->estaOcupado = 0;
            particion->inicio = actual;
            particion->tamanio = atoi(particiones[i]);
            actual = particion->tamanio + particion->inicio;
            list_add(memoria_usuario->particiones, particion);
        }
    }
    else if (string_equals_ignore_case(esquema, "DINAMICAS")) {
        Particion* particion = malloc(sizeof(Particion));
        particion->estaOcupado = 0;
        particion->inicio = 0;
        particion->tamanio = tam_memoria;
        list_add(memoria_usuario->particiones, particion);
    }

}

void imprimir_memoria_usuario() {
    for(int i = 0; i < list_size(memoria_usuario->particiones); i++) {
        Particion* particion = list_get(memoria_usuario->particiones, i);
        printf("Esta ocupado: %i\n", particion->estaOcupado);
        printf("Inicio: %i\n", particion->inicio);
        printf("Tamanio: %i\n", particion->tamanio);
        printf("------------\n");
    }
}

int buscar_indice(Particion *particion){
    for(int i = 0; i < list_size(memoria_usuario->particiones); i++){
        Particion* particionAux = list_get(memoria_usuario->particiones,i);
        if(particion->estaOcupado == particionAux->estaOcupado && particion->inicio == particionAux->inicio && particion->tamanio == particionAux->tamanio)
            return i;
    }
    return -1;
}


Particion *buscar_first_fijas(int tamanio) {
    t_list* lista_particiones = memoria_usuario->particiones;
    bool _libreYEntra(void *ptr){
        Particion *particion = (Particion*) ptr;
        return (particion->estaOcupado == 0 && particion->tamanio >= tamanio);
    }

    Particion* particionAAsignar = list_find(lista_particiones, _libreYEntra);
    if(particionAAsignar == NULL){
        //no hay espacio en memoria
        return NULL;
    }else{
        particionAAsignar->estaOcupado=1;
        return particionAAsignar;
    }
}

Particion *buscar_first_dinamicas(int tamanio) {
    t_list* lista_particiones = memoria_usuario->particiones;
    bool _libreYEntra(void *ptr){
        Particion *particion = (Particion*) ptr;
        return (particion->estaOcupado == 0 && particion->tamanio >= tamanio);
    }

    Particion* particionAAsignar = list_find(lista_particiones, _libreYEntra);
    if(particionAAsignar == NULL){
        //no hay espacio en memoria
        return NULL;
    }else {
        Particion* particionSiguiente = malloc(sizeof(Particion));
        particionSiguiente->estaOcupado = 0;
        particionSiguiente->inicio = particionAAsignar->inicio + tamanio;
        particionSiguiente->tamanio = particionAAsignar->tamanio - tamanio;
        particionAAsignar->estaOcupado=1;
        particionAAsignar->tamanio = tamanio;
        if(particionSiguiente->tamanio == 0){
            free(particionSiguiente);
            return particionAAsignar;
        }
        list_add_in_index(memoria_usuario->particiones, buscar_indice(particionAAsignar)+1, particionSiguiente);
        return particionAAsignar;
    }
}


Particion *buscar_first(int tamanio) {

    if (string_equals_ignore_case(esquema,"FIJAS")){
        return buscar_first_fijas(tamanio);
    }
    else if(string_equals_ignore_case(esquema,"DINAMICAS")){
        return buscar_first_dinamicas(tamanio);
    }else{
       log_error(logger,"Error, esquema de memoria desconocido");
    }
}

Particion *buscar_best_fijas(int tamanio){
    bool _estaOcupadaYEntra(void *ptr){
        Particion *particion = (Particion*) ptr;
        return (particion->estaOcupado == 0 && particion->tamanio >= tamanio);
    }

    t_list* lista_filtrada = list_filter(memoria_usuario->particiones,_estaOcupadaYEntra);
    int tam_menor = tam_memoria;
    Particion* menor_particion = NULL;
    for (int i = 0; i < list_size(lista_filtrada); i++){
        Particion* particionAux = list_get(lista_filtrada, i);
        if (particionAux->tamanio - tamanio < tam_menor) {
            tam_menor = particionAux->tamanio - tamanio;
            menor_particion = particionAux;
        }
            
    }
    if (menor_particion)
        menor_particion->estaOcupado = 1;
    return menor_particion;
}

Particion *buscar_best_dinamicas(int tamanio){
    bool _estaOcupadaYEntra(void *ptr){
        Particion *particion = (Particion*) ptr;
        return (particion->estaOcupado == 0 && particion->tamanio >= tamanio);
    }

    t_list* lista_filtrada = list_filter(memoria_usuario->particiones,_estaOcupadaYEntra);
    int tam_menor = tam_memoria;
    Particion* menor_particion = NULL;
    for (int i = 0; i < list_size(lista_filtrada); i++){
        Particion* particionAux = list_get(lista_filtrada, i);
        if (particionAux->tamanio - tamanio < tam_menor) {
            tam_menor = particionAux->tamanio - tamanio;
            menor_particion = particionAux;
        }
            
    }
    if (menor_particion){
        Particion *particionSiguiente = malloc(sizeof(Particion));
        particionSiguiente->estaOcupado = 0;
        particionSiguiente->inicio = menor_particion->inicio + tamanio;
        particionSiguiente->tamanio = menor_particion->tamanio-tamanio;
        menor_particion->estaOcupado = 1;
        menor_particion->tamanio = tamanio;
        if(particionSiguiente->tamanio == 0){
            free(particionSiguiente);
            return menor_particion;
        }
        list_add_in_index(memoria_usuario->particiones, buscar_indice(menor_particion)+1, particionSiguiente);
       return menor_particion;
    }
}

Particion *buscar_best(int tamanio) {
    if (string_equals_ignore_case(esquema,"FIJAS")){
        return buscar_best_fijas(tamanio);
    }
    else if(string_equals_ignore_case(esquema,"DINAMICAS")){
        return buscar_best_dinamicas(tamanio);
    }else{
       log_error(logger,"Error, esquema de memoria desconocido");
    }
    
    
}

Particion *buscar_worst_fijas(int tamanio) {
    bool _estaOcupadaYEntra(void *ptr){
        Particion *particion = (Particion*) ptr;
        return (particion->estaOcupado == 0 && particion->tamanio >= tamanio);
    }

    t_list* lista_filtrada = list_filter(memoria_usuario->particiones,_estaOcupadaYEntra);
    int tam_mayor = -1;
    Particion* mayor_particion = NULL;
    for (int i = 0; i < list_size(lista_filtrada); i++){
        Particion* particionAux = list_get(lista_filtrada, i);
        if (particionAux->tamanio - tamanio > tam_mayor) {
            tam_mayor = particionAux->tamanio - tamanio;
            mayor_particion = particionAux;
        }
            
    }
    if (mayor_particion)
        mayor_particion->estaOcupado = 1;
    return mayor_particion;
}

Particion *buscar_worst_dinamicas(int tamanio) {
    bool _estaOcupadaYEntra(void *ptr){
        Particion *particion = (Particion*) ptr;
        return (particion->estaOcupado == 0 && particion->tamanio >= tamanio);
    }

    t_list* lista_filtrada = list_filter(memoria_usuario->particiones,_estaOcupadaYEntra);
    int tam_mayor = -1;
    Particion* mayor_particion = NULL;
    for (int i = 0; i < list_size(lista_filtrada); i++){
        Particion* particionAux = list_get(lista_filtrada, i);
        if (particionAux->tamanio - tamanio > tam_mayor) {
            tam_mayor = particionAux->tamanio - tamanio;
            mayor_particion = particionAux;
        }
            
    }
    if (mayor_particion){
        Particion *particionSiguiente = malloc(sizeof(Particion));
        particionSiguiente->estaOcupado = 0;
        particionSiguiente->inicio = mayor_particion->inicio + tamanio;
        particionSiguiente->tamanio = mayor_particion->tamanio-tamanio;
        mayor_particion->estaOcupado = 1;
        mayor_particion->tamanio = tamanio;
        if(particionSiguiente->tamanio == 0){
            free(particionSiguiente);
            return mayor_particion;
        }
        list_add_in_index(memoria_usuario->particiones, buscar_indice(mayor_particion)+1, particionSiguiente);
       return mayor_particion;
    }
}

Particion *buscar_worst(int tamanio) {
    if (string_equals_ignore_case(esquema, "FIJAS")){
        return buscar_worst_fijas(tamanio);
    }
    else if (string_equals_ignore_case(esquema, "DINAMICAS")) {
        return buscar_worst_dinamicas(tamanio);
    }
}

// 512 / 600  --> 512-500 = 12 600-500 = 100 
// 500



Particion *buscar_particion(int tamanio){
    if (string_equals_ignore_case(algoritmo_busqueda, "FIRST")){
        
        
        return buscar_first(tamanio);
    }
    else if(string_equals_ignore_case(algoritmo_busqueda, "BEST")){
        return buscar_best(tamanio);
    }
    else if(string_equals_ignore_case(algoritmo_busqueda, "WORST")){
        return buscar_worst(tamanio);
    }else{
        log_error(logger, "Error, algoritmo de busqueda desconocido");
        return EXIT_FAILURE;
    }
    
}

t_list *buscar_hilos_de_proceso(int pid){
    bool _esDelProceso(void *ptr2){
        CONTEXTO_HILO *ctx_proc = (CONTEXTO_HILO*) ptr2;
        return ctx_proc->pid == pid;
    }
    return list_filter(contextos_hilos,_esDelProceso);
}

//contexto_proceso->BASE = particion->inicio;
//contexto_proceso->LIMITE = particion->inicio + particion->tamanio;

void agrupar_particiones(Particion* particion) {
    int indice = buscar_indice(particion);
    if ((indice > 0)  && (list_size(memoria_usuario->particiones)  > indice)){
        Particion *particionSiguiente = list_get(memoria_usuario->particiones,indice + 1);
        Particion *particionAnterior = list_get(memoria_usuario->particiones, indice - 1);
        if(particionSiguiente->estaOcupado == 0){
            particion->tamanio = particion->tamanio + particionSiguiente->tamanio;
            list_remove_element(memoria_usuario->particiones, particionSiguiente);
            free(particionSiguiente);
        }
        if(particionAnterior->estaOcupado == 0){
            particion->inicio = particionAnterior->inicio;
            particion->tamanio = particion->tamanio + particionAnterior->tamanio;
            list_remove_element(memoria_usuario->particiones, particionAnterior);
            free(particionAnterior);
        }
    } else if(indice == 0){
        Particion *particionSiguiente = list_get(memoria_usuario->particiones,indice+1);
        if(particionSiguiente->estaOcupado == 0){
            particion->tamanio = particion->tamanio + particionSiguiente->tamanio;
            list_remove_element(memoria_usuario->particiones, particionSiguiente);
            free(particionSiguiente);
        }
    }else if(indice == list_size(memoria_usuario->particiones) - 1 ){
        Particion *particionAnterior = list_get(memoria_usuario->particiones, indice + 1);
        if(particionAnterior->estaOcupado == 0){
            particion->inicio = particionAnterior->inicio;
            particion->tamanio = particion->tamanio + particionAnterior->tamanio;
            list_remove_element(memoria_usuario->particiones, particionAnterior);
            free(particionAnterior);
        }
    }
    //if(particionSiguiente)
    
}

void finalizacion_de_proceso(int pid){
    CONTEXTO_PROCESO *contexto_proceso = buscar_contexto_proceso(pid); // pid base limite
    
    bool _esLaParticion(void *ptr){
    Particion *particion = (Particion*) ptr;
    return (particion->inicio == contexto_proceso->BASE && contexto_proceso->LIMITE == particion->inicio + particion->tamanio);
    }

    

    // buscar los hilos del proceso
    // borrarlos -> borrar los registros free(CONTEXTO_HILO)
    t_list *lista_a_borrar = buscar_hilos_de_proceso(pid);
    for(int i = 0; i < list_size(lista_a_borrar);i++){
        CONTEXTO_HILO *contexto_hilo = list_get(lista_a_borrar, i);
        eliminar_hilo_y_contexto(contexto_hilo->tid, contexto_hilo->pid);
    }

    Particion* particion = list_find(memoria_usuario->particiones,_esLaParticion);
    log_info(logger, "## Proceso Destruido - PID: %d - Tamaño: %d",pid,particion->tamanio);
    particion->estaOcupado=0;
    log_info(logger, "Finalizado proceso: %d",pid);
    // marcar la partición del proceso como libre
    // chequear si hay que agruppar particiones libres
    if (string_equals_ignore_case(esquema, "DINAMICAS")) {
        agrupar_particiones(particion);
    }
}

/*
typedef struct{
	int tid;
	int pid;
	t_dictionary* instrucciones;
}CONTEXTO_ARCHIVO;
*/
void liberar_contexto_hilo(CONTEXTO_HILO *contexto_hilo) {
    free(contexto_hilo->Registros);
    free(contexto_hilo);
}

void liberar_archivo(CONTEXTO_ARCHIVO *archivo) {
    dictionary_destroy_and_destroy_elements(archivo->instrucciones, free);
    free(archivo);
}

void eliminar_hilo_y_contexto(int tid, int pid){
    //borrar contexto hilo
    //borrar archivo

    CONTEXTO_HILO *contexto_hilo = buscar_contexto_hilo(pid, tid);
    CONTEXTO_ARCHIVO *archivo = buscar_archivo(pid,tid);
    // esto devuelve 0 si no existe el ctx_hilo
    if (list_remove_element(contextos_hilos, contexto_hilo))
        liberar_contexto_hilo(contexto_hilo);
    else {
        log_error(logger, "No existe el par (<PID: %i>:<TID: %i>)\n", pid, tid);
        return EXIT_FAILURE;
    }    
    if(list_remove_element(archivos,archivo))
        liberar_archivo(archivo);
    else{
        log_error(logger,"No existe el archivo del par (<PID: %d>),(<TID:%d>)\n",pid,tid);
        return EXIT_FAILURE;
    }
    
    log_info(logger,"## Hilo <Destruido> - (PID:TID) - (<%d>:<%d>)",pid,tid);


    

}

CONTEXTO_ARCHIVO *buscar_archivo(int pid, int tid){
    bool _esArchivo(void* ptr){
        CONTEXTO_ARCHIVO *ctx = (CONTEXTO_ARCHIVO *) ptr;
        return ctx->pid == pid && ctx->tid == tid;
    }
    return list_find(archivos,_esArchivo);
}

int leer_memoria(int direccion){
    return *((int*)memoria_usuario->memoria_usuario + direccion);
}

void escribir_memoria(int direccion, int valor){
    *((int*)memoria_usuario->memoria_usuario + direccion) = valor;
}

/*
Obtener instrucción
Deberemos devolverle la instrucción correspondiente al hilo y al Program Counter recibido. 
Por ejemplo, si el hilo 3 del proceso 1 pide la instrucción número 4, deberemos devolver la 5ta instrucción del 
pseudocódigo correspondiente a ese hilo.
*/

char* obtener_instruccion(int key, int pid, int tid){
    char key_diccionario[256];
    snprintf(key_diccionario, 256, "%d", key);
    
    
    CONTEXTO_ARCHIVO *archivo = buscar_archivo(pid, tid);
    char* instruccion = dictionary_get(archivo->instrucciones, key_diccionario);
    if (!instruccion) printf("NULL\n");
    return instruccion;
}

void deserializar_write_mem(int cliente_fd_dispatch) {
    t_buffer* buffer = recibir_buffer_completo(cliente_fd_dispatch);
    int valor = extraer_int_del_buffer(buffer);
    int direccion = extraer_int_del_buffer(buffer);
    log_info(logger,"## Escritura - (PID:TID) - (d:d) - Dir.Física: %d - Tamaño: %d",direccion,sizeof(valor));
    escribir_memoria(direccion, valor);
    //WRITE_MEM_RTA
    int rta = WRITE_MEM_RTA;
    send(cliente_fd_dispatch, &rta, sizeof(op_code), 0);
}

void deserializar_read_mem(cliente_fd_dispatch) {
    t_buffer* buffer = recibir_buffer_completo(cliente_fd_dispatch);
    int direccion = extraer_int_del_buffer(buffer);
    int dato = leer_memoria(direccion);
    log_info(logger,"## Lecutra - (PID:TID) - (d:d) - Dir.Física: %d - Tamaño: %d",direccion,sizeof(dato));
    enviar_lectura(dato);
}

void enviar_lectura(int dato) {
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer, dato);
    t_paquete *paquete = crear_paquete(READ_MEM_RTA, buffer);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
}



void dump_memory(int tid,  int pid){
    CONTEXTO_CPU *contexto_a_dumpear = buscar_contextos(tid,pid);
    char *contenido = parse_contexto_cpu(contexto_a_dumpear);
    int tamanio = contexto_a_dumpear->contexto_proceso->LIMITE - contexto_a_dumpear->contexto_proceso->BASE + 1;
    int fd_filesystem = conectarse_a_filesystem();
    t_buffer *buffer = crear_buffer();
    cargar_int_al_buffer(buffer,tid);
    cargar_int_al_buffer(buffer,pid);
    cargar_int_al_buffer(buffer,tamanio);
    cargar_string_al_buffer(buffer,contenido);

    log_info(logger,"## Memory Dump solicitado - (PID:TID) - (%d:%d)",pid,tid);

    t_paquete *paquete = crear_paquete(SOL_DUMP, buffer);
    enviar_paquete(paquete, fd_filesystem);
    eliminar_paquete(paquete);
    if (recibir_operacion(fd_filesystem) == MEM_DUMPEADA)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

char *parse_contexto_cpu(CONTEXTO_CPU *contexto) {
    char *str = malloc(1024);
    sprintf(str, "Contexto Proceso:\n");

    // Proceso
    sprintf(str + strlen(str), "  Proceso:\n");
    sprintf(str + strlen(str), "    PID: %d\n", contexto->contexto_proceso->pid);
    sprintf(str + strlen(str), "    BASE: %u\n", contexto->contexto_proceso->BASE);
    sprintf(str + strlen(str), "    LIMITE: %u\n", contexto->contexto_proceso->LIMITE);

    // Hilo
    sprintf(str + strlen(str), "  Hilo:\n");
    sprintf(str + strlen(str), "    TID: %d\n", contexto->contexto_hilo->tid);
    sprintf(str + strlen(str), "    PID: %d\n", contexto->contexto_hilo->pid);
    sprintf(str + strlen(str), "    Archivo Pseudocodigo: %s\n", contexto->contexto_hilo->archivo_pseudocodigo);

    // Registros
    sprintf(str + strlen(str), "    Registros:\n");
    REGISTROS *regs = contexto->contexto_hilo->Registros;
    sprintf(str + strlen(str), "      PC: %u\n", regs->PC);
    sprintf(str + strlen(str), "      AX: %u\n", regs->AX);
    sprintf(str + strlen(str), "      BX: %u\n", regs->BX);
    sprintf(str + strlen(str), "      CX: %u\n", regs->CX);
    sprintf(str + strlen(str), "      DX: %u\n", regs->DX);
    sprintf(str + strlen(str), "      EX: %u\n", regs->EX);
    sprintf(str + strlen(str), "      FX: %u\n", regs->FX);
    sprintf(str + strlen(str), "      GX: %u\n", regs->GX);
    sprintf(str + strlen(str), "      HX: %u\n", regs->HX);

    return str;
}



void test(){

    
    CONTEXTO_HILO *contexto_hilo = malloc(sizeof(CONTEXTO_HILO));
    contexto_hilo->pid = 0;
    contexto_hilo->tid = 0;
    contexto_hilo->archivo_pseudocodigo = "archivo1";
    cargar_archivo(contexto_hilo->archivo_pseudocodigo, contexto_hilo->tid,contexto_hilo->pid);
    obtener_instruccion(0,0,0);
    obtener_instruccion(1,0,0);
    obtener_instruccion(2,0,0);
    // *((int*)memoria_usuario->memoria_usuario + 50) = 15;
    // leer_memoria(50);
    // escribir_memoria(51,16);
    // leer_memoria(51);


    // CONTEXTO_PROCESO* ctx1 = malloc(sizeof(CONTEXTO_PROCESO));
    // ctx1->pid = 0;
    // CONTEXTO_PROCESO* ctx2 = malloc(sizeof(CONTEXTO_PROCESO));
    // ctx2->pid = 1;
    // CONTEXTO_PROCESO* ctx3 = malloc(sizeof(CONTEXTO_PROCESO));
    // ctx3->pid = 2;
    // CONTEXTO_PROCESO* ctx4 = malloc(sizeof(CONTEXTO_PROCESO));
    // ctx4->pid = 3;
    //  CONTEXTO_PROCESO* ctx5 = malloc(sizeof(CONTEXTO_PROCESO));
    // ctx5->pid = 4;


    // Particion *part1 = buscar_worst_dinamicas(50);
    // ctx1->BASE = part1->inicio;
    // ctx1->LIMITE = part1->inicio + part1->tamanio;
    // list_add(contextos_procesos,ctx1);
    // Particion *part2 = buscar_worst_dinamicas(100);
    // ctx2->BASE = part2->inicio;
    // ctx2->LIMITE = part2->inicio + part2->tamanio;
    // list_add(contextos_procesos,ctx2);
    // Particion *part3 = buscar_worst_dinamicas(400);
    // ctx3->BASE = part3->inicio;
    // ctx3->LIMITE = part3->inicio + part3->tamanio;
    // list_add(contextos_procesos,ctx3);
    // Particion *part4 = buscar_worst_dinamicas(250);
    // ctx4->BASE = part4->inicio;
    // ctx4->LIMITE = part4->inicio + part4->tamanio;
    // list_add(contextos_procesos,ctx4);
    // Particion *part5 = buscar_worst_dinamicas(200);
    // ctx5->BASE = part5->inicio;
    // ctx5->LIMITE = part5->inicio + part5->tamanio;
    // list_add(contextos_procesos,ctx5);

    // //imprimir_memoria_usuario();
    // finalizacion_de_proceso(1);
    // //imprimir_memoria_usuario();
    // finalizacion_de_proceso(3);

    // CONTEXTO_PROCESO* ctx6 = malloc(sizeof(CONTEXTO_PROCESO));
    // ctx6->pid = 4;
    // Particion *part6 = buscar_worst_dinamicas(50);
    // ctx6->BASE = part6->inicio;
    // ctx6->LIMITE = part6->inicio + part6->tamanio;
    // list_add(contextos_procesos,ctx6);

    // imprimir_memoria_usuario();
}





