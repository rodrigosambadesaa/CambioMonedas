#include <string.h>
#include "moneda_gestion.h"
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    if (strcmp(argv[1], "snapshot") == 0)
        return crear_snapshot_stock(argv[2]) ? 0 : 1;
    if (strcmp(argv[1], "restore") == 0)
        return restaurar_snapshot_stock(argv[2]) ? 0 : 1;
    return 2;
}
