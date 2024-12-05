#include "utilidades.h"

t_log *logger;
t_config *config;
char *puerto;
char *mount_dir;
int block_size;
int block_count;
int retardo_acceso_bloque;
FILE *archivoBitmap;
FILE *archivoBloqueDeDatos;

void iniciar_filesystem(void)
{
    config = iniciar_config("filesystem.config");
    char *log_level = config_get_string_value(config, "LOG_LEVEL");
    puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    mount_dir = config_get_string_value(config, "MOUNT_DIR");
    block_size = config_get_int_value(config, "BLOCK_SIZE");
    block_count = config_get_int_value(config, "BLOCK_COUNT");
    retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
    logger = iniciar_logger("logFS.log", "FileSystem", log_level_from_string(log_level));
    crear_bitmap();
    crear_bloques_de_datos();
}

void terminar_ejecucion(int socket)
{
    log_info(logger, "Finalizando ejecución de FILESYSTEM");
    close(socket);
    log_destroy(logger);
    config_destroy(config);
    exit(EXIT_SUCCESS);
}


void crear_bitmap(){
    int tam_bitmap = (block_count +7)/8; 
    int total_length = strlen(mount_dir) + strlen("bitmap.dat") + 2; // El "/" antes de la ruta, y el '\0' final
    char ruta_archivo[total_length];
    strcpy(ruta_archivo, mount_dir);
    strcat(ruta_archivo, "/");
    strcat(ruta_archivo, "bitmap.dat");
    archivoBitmap = fopen(ruta_archivo,"wb");
    if(archivoBitmap == NULL){
        log_error(logger,"Error al crear el archivo bitmap.dat");
        exit(EXIT_FAILURE);
    }
    char *bitmap = calloc(tam_bitmap,sizeof(char));

    if(fwrite(bitmap,1,tam_bitmap,archivoBitmap) != (size_t)tam_bitmap){
        log_error(logger,"Error al escribir en el archivo bitmap.dat");
        free(bitmap);
        exit(EXIT_FAILURE);
    }
    fclose(archivoBitmap);
    free(bitmap);
}

void crear_bloques_de_datos(){
    int total_length = strlen(mount_dir) + strlen("bloques.dat") + 2; // El "/" antes de la ruta, y el '\0' final
    char ruta_archivo[total_length];
    strcpy(ruta_archivo, mount_dir);
    strcat(ruta_archivo, "/");
    strcat(ruta_archivo, "bloques.dat");
    archivoBloqueDeDatos = fopen(ruta_archivo,"wb");
    if(archivoBloqueDeDatos == NULL){
        log_error(logger,"Error al crear el archivo bitmap.dat");
        exit(EXIT_FAILURE);
    }

    int *buffer = calloc(block_size,sizeof(int));
    for(size_t i = 0; i<block_count;i++){
        fwrite(buffer,sizeof(int),block_size,archivoBloqueDeDatos);
    }
    free(buffer);
    fclose(archivoBloqueDeDatos);
}

bool verificar_espacio_disponible(int tam_archivo){
    int bloques_necesarios = ceil(tam_archivo/block_size) + 1; //1 bloque necesario por el de indice mas la cantidad por archivo
    int bloques_libres = cant_bloques_libres();
    if(bloques_libres < bloques_necesarios){
        return false;
    }
    return true;
}

int cant_bloques_libres(){
    int tam_bitmap = (block_count +7)/8;
    int total_length = strlen(mount_dir) + strlen("bitmap.dat") + 2; // El "/" antes de la ruta, y el '\0' final
    char ruta_archivo[total_length];
    strcpy(ruta_archivo, mount_dir);
    strcat(ruta_archivo, "/");
    strcat(ruta_archivo, "bitmap.dat");
    archivoBitmap = fopen(ruta_archivo,"rb");

    int bloques_libres=0;
    char byte;
    int bits_restantes = block_count % 8;

    for(int i = 0; i < tam_bitmap;i++){
        fread(&byte,sizeof(char),1,archivoBitmap);

        if(i == tam_bitmap-1 && bits_restantes > 0){
            byte &= (0xFF>>(8-bits_restantes));;
        }

        for(int j = 0;j<8;j++){
            if(i==tam_bitmap-1 && j>=bits_restantes && bits_restantes!=0){
                break;
            }
            if (((byte >> j) & 1) == 0) {
                bloques_libres++;
            }
            
        }
    }
    fclose(archivoBitmap);
    log_trace(logger,"Cant de bloques libres: %i",bloques_libres);
    return bloques_libres;
}

int *reservar_bloques(int cantidad, char* nombre_archivo){
    int tam_bitmap = (block_count +7)/8;
    int total_length = strlen(mount_dir) + strlen("bitmap.dat") + 2; // El "/" antes de la ruta, y el '\0' final
    char ruta_archivo[total_length];
    strcpy(ruta_archivo, mount_dir);
    strcat(ruta_archivo, "/");
    strcat(ruta_archivo, "bitmap.dat");
    archivoBitmap = fopen(ruta_archivo,"r+b");

    char byte;
    int bloques_restantes = cantidad;
    int bits_restantes = block_count % 8;
    int bloques_reservados = 0;

    //array para guardar los bloques reservados, el 0 siempre es el indice, el resto son de datos
    int *bloques = malloc(cantidad * sizeof(int));

    for(int i = 0; i<tam_bitmap;i++){
        fread(&byte,sizeof(char),1,archivoBitmap);

        for(int j = 0; j<8 && bloques_restantes > 0; j++){
            if(i==tam_bitmap-1 && j>=bits_restantes && bits_restantes!=0){
                break;
            }

            if(((byte >> j)& 1) == 0){
                byte |= (1<<j);
                fseek(archivoBitmap,i,SEEK_SET);
                fwrite(&byte,sizeof(char),1,archivoBitmap);
                bloques[bloques_reservados] = i*8+j;
                log_trace(logger, "## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d",bloques[bloques_reservados],nombre_archivo,cant_bloques_libres());
                bloques_reservados++;
                bloques_restantes--;
            }
        }
    }
    fclose(archivoBitmap);
    log_trace(logger,"Bloques reservados: %d",bloques_reservados);
    return bloques;
}

void crear_archivo_metadata(int pid, int tid, int tamanio){
    char *timestamp = obtenerTimeStamp();
    char nombre_archivo[50];
    snprintf(nombre_archivo,sizeof(nombre_archivo),"%d-%d-%s",pid,tid,timestamp);

    int total_length = strlen(mount_dir) + strlen("/files/") + strlen(nombre_archivo+strlen(".dmp")) + 1;  // /files/ + nombre del archivo
    char ruta_metadata[total_length];
    strcpy(ruta_metadata, mount_dir);
    strcat(ruta_metadata, "/files/");
    strcat(ruta_metadata, nombre_archivo);
    strcat(ruta_metadata,".dmp");

    FILE *metadata = fopen(ruta_metadata,"wb");
    fclose(metadata);
    
}

char *obtenerTimeStamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    setenv("TZ", "GMT+3", 1);
    tzset(); // Actualiza la zona horaria en el programa
    struct tm *timestamp_info = localtime(&ts.tv_sec);
    static char timestamp[20];
    // Generar el timestamp inicial (HH:MM:SS)
    strftime(timestamp, 20, "%H:%M:%S", timestamp_info);

    // Agregar milisegundos
    size_t used_length = strlen(timestamp);
    size_t remaining_length = 20 - used_length;

    if (remaining_length > 0) {
        snprintf(timestamp + used_length, remaining_length, ":%03d", ts.tv_nsec / 1000000);
    }
    log_trace(logger, "ts: %s", timestamp);
    return timestamp;
}

bool crear_archivo(int pid, int tid, int tamanio, char *contenido){
    int tamanio_contenido = strlen(contenido);
    int bloques_necesarios = (tamanio_contenido + block_size-1)/block_size + 1;
    log_trace(logger,"bloques necesarios: %d",bloques_necesarios);

    if(cant_bloques_libres() < bloques_necesarios){
        log_error(logger,"No hay espacio suficiente en el filesystem");
        return false;
    }



    char *timestamp = obtenerTimeStamp();
    log_trace(logger,"timestamp: %s",timestamp);
    char nombre_archivo[50];
    snprintf(nombre_archivo,sizeof(nombre_archivo),"%d-%d-%s",pid,tid,timestamp);

    int *bloques_reservados = reservar_bloques(bloques_necesarios,nombre_archivo);

    int bloque_indice = bloques_reservados[0];
    int *bloques_datos = &bloques_reservados[1];

    log_info(logger,"## Archivo Creado: %s - Tamaño: %d",nombre_archivo,tamanio);

    int tam_ruta_metadata = strlen(mount_dir) + strlen("/files/") + strlen(nombre_archivo+strlen(".dmp")) + 1;  // /files/ + nombre del archivo
    char ruta_metadata[tam_ruta_metadata];
    strcpy(ruta_metadata, mount_dir);
    strcat(ruta_metadata, "/files/");
    strcat(ruta_metadata, nombre_archivo);
    strcat(ruta_metadata,".dmp");

    FILE *metadata = fopen(ruta_metadata,"wb");
    fprintf(metadata,"Tamaño:%i Bloque_Indice: %d\n",tamanio,bloque_indice);
    fclose(metadata);

    int tam_ruta_bloques = strlen(mount_dir) + strlen("/bloques.dat") + 1; // y el '\0' final
    char ruta_archivo[tam_ruta_bloques];
    strcpy(ruta_archivo, mount_dir);
    strcat(ruta_archivo, "/bloques.dat");
    archivoBloqueDeDatos = fopen(ruta_archivo,"r+b");

    //Escribir bloque de indice
    fseek(archivoBloqueDeDatos,bloque_indice*block_size, SEEK_SET);
    for(int i = 0;i<bloques_necesarios - 1;i++){
        fwrite(&bloques_datos[i],sizeof(int),1,archivoBloqueDeDatos);
        log_info(logger,"## Acceso Bloque - Archivo: %s - Tipo Bloque: INDICE - Bloque File System: %d",nombre_archivo,bloque_indice);
    }

    //escribir datos en los bloques de datos
    int bytes_escritos = 0;
    
    for(int i = 0;i<bloques_necesarios-1;i++){
        int bytes_a_escribir;
        if(tamanio_contenido-bytes_escritos > block_size){
            bytes_a_escribir = block_size;
        }else{
            bytes_a_escribir = tamanio_contenido - bytes_escritos;
            //log_trace();
        }
        fseek(archivoBloqueDeDatos,bloques_datos[i]*block_size,SEEK_SET);
        fwrite(contenido + bytes_escritos, sizeof(char),bytes_a_escribir,archivoBloqueDeDatos);
        log_info(logger,"## Acceso Bloque - Archivo: %s - Tipo Bloque: DATOS - Bloque File System: %d",nombre_archivo,bloques_datos[i]);
        bytes_escritos += bytes_a_escribir;
    }
    fclose(archivoBloqueDeDatos);
    free(bloques_reservados);
    log_trace(logger,"## Fin de solicitud - Archivo: %s",nombre_archivo);
    return true;
}

