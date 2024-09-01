#include <utils/hello.h>
#include "utilidades.h"

int main(int argc, char* argv[]) {
    t_config* config = iniciar_config();
    char* log_level = config_get_string_value(config,"LOG_LEVEL");

    logger = log_create("logFS.log","FileSystem",true,log_level_from_string(log_level));

    int server_fd = iniciar_servidor();

    terminar_ejecucion(server_fd);

    return 0;
}
