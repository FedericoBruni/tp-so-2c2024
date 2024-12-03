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
    crear_archivo_metadata(1,2,3);
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

    char *buffer = calloc(block_size,sizeof(char));
    for(size_t i = 0; i<block_count;i++){
        fwrite(buffer,sizeof(char),block_size,archivoBloqueDeDatos);
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

void reservar_bloques(int cantidad){
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
                bloques_reservados++;
                bloques_restantes--;
            }
        }
    }
    fclose(archivoBitmap);
    log_trace(logger,"Bloques reservados: %d",bloques_reservados);
}

void crear_archivo_metadata(int pid, int tid, int tamanio){

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);

    struct tm *timestamp_info = localtime(&ts.tv_sec);
    int ms = ts.tv_nsec / 1000000;
    char timestamp[20];
    strftime(timestamp,sizeof(timestamp),"%H:%M:%S",timestamp_info);
    snprintf(timestamp+strlen(timestamp),sizeof(timestamp)-strlen(timestamp),":%03d",ms);
    log_trace(logger, "Tiempo actual: %s",timestamp);
}