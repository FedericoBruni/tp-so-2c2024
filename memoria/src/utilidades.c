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

void enviar_contexto(int cliente_fd_dispatch){
    t_buffer *buffer = recibir_buffer_completo(cliente_fd_dispatch);
    
    int tid = extraer_int_del_buffer(buffer);
    int pid = extraer_int_del_buffer(buffer);

    CONTEXTO_CPU *contexto_cpu = malloc(sizeof(CONTEXTO_CPU));
    contexto_cpu = buscar_contextos(tid,pid);
    
    t_buffer *bufferRta = crear_buffer();
    cargar_contexto_hilo(bufferRta, contexto_cpu->contexto_hilo);
    cargar_contexto_proceso(bufferRta, contexto_cpu->contexto_proceso);
    t_paquete *paquete = crear_paquete(CONTEXTO_ENVIADO,bufferRta);
    enviar_paquete(paquete, cliente_fd_dispatch);
    eliminar_paquete(paquete);
    printf("Contexto enviado -> TID: %i, PID: %i\n", tid, pid);
    //enviar_contexto(cliente_fd_dispatch,contexto_cpu);
    //int rta_sol_mem = CONTEXTO_ENVIADO;
    //send(cliente_fd_dispatch, &rta_sol_mem, sizeof(op_code), 0);
    
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




void actualizar_contexto(int cliente_fd_dispatch){
    
    t_buffer *buffer = recibir_buffer_completo(cliente_fd_dispatch);
    CONTEXTO_HILO *ctx_hilo = malloc(sizeof(CONTEXTO_HILO));
    CONTEXTO_PROCESO *ctx_proceso = malloc(sizeof(CONTEXTO_PROCESO));
    ctx_hilo = extraer_contexto_hilo(buffer);
    ctx_proceso = extraer_contexto_proceso(buffer);
    
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

    printf("String concatenado: %s\n", ruta_archivo);


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
    printf("TID: %d\n", archivo->tid);
    printf("PID: %d\n", archivo->pid);
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
    imprimir_memoria_usuario();

}

void imprimir_memoria_usuario() {
    for(int i = 0; i < list_size(memoria_usuario->particiones); i++) {
        Particion* particion = list_get(memoria_usuario->particiones, i);
        printf("Esta ocupado: %i\n", particion->estaOcupado);
        printf("Inicio: %i\n", particion->inicio);
        printf("Tamanio: %i\n", particion->tamanio);
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
        printf("particion agregada con base: %d y tamaño: %d",particionSiguiente->inicio,particionSiguiente->tamanio);
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



//contexto_proceso->BASE = particion->inicio;
//contexto_proceso->LIMITE = particion->inicio + particion->tamanio;

void finalizacion_de_proceso(int pid){
    CONTEXTO_PROCESO *contexto_proceso = buscar_contexto_proceso(pid); // pid base limite
    
    bool _esLaParticion(void *ptr){
    Particion *particion = (Particion*) ptr;
    return (particion->inicio == contexto_proceso->BASE && contexto_proceso->LIMITE == particion->inicio + particion->tamanio);
    }

    // buscar los hilos del proceso
    // borrarlos -> borrar los registros free(CONTEXTO_HILO)
    Particion* particion = list_find(memoria_usuario->particiones,_esLaParticion);
    particion->estaOcupado=0;
    log_info(logger, "Finalizado proceso: %d",pid);
    // marcar la partición del proceso como libre
    // chequear si hay que agruppar particiones libres
}








