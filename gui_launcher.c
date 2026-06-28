#include "gui_launcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* funcion file_exists_gui: contiene la logica principal de esta operacion. */
static int file_exists_gui(const char *path)
{
    FILE *f;

    /* if: comprueba path == NULL || path[0] == '\0' antes de ejecutar esta rama. */
    if (path == NULL || path[0] == '\0')
        return 0;

    f = fopen(path, "r");
    /* if: comprueba f != NULL antes de ejecutar esta rama. */
    if (f != NULL)
    {
        fclose(f);
        return 1;
    }

    return 0;
}

/* funcion launch_exec_nonblocking: contiene la logica principal de esta operacion. */
static int launch_exec_nonblocking(const char *exec_path)
{
    char cmd[1024];

    /* if: comprueba exec_path == NULL || exec_path[0] == '\0' antes de ejecutar esta rama. */
    if (exec_path == NULL || exec_path[0] == '\0')
        return 0;

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", exec_path);
#else
    snprintf(cmd, sizeof(cmd), "setsid \"%s\" >/dev/null 2>&1 &", exec_path);
#endif

    return system(cmd) == 0;
}

/* funcion progvoraz_launch_gui_nonblocking: contiene la logica principal de esta operacion. */
int progvoraz_launch_gui_nonblocking(void)
{
#ifdef _WIN32
    const char *gui_execs[] = {"progvoraz_gui.exe", ".\\progvoraz_gui.exe"};
#else
    const char *gui_execs[] = {"./progvoraz_gui", "progvoraz_gui"};
#endif

    /* for: itera segun size_t i = 0; i < sizeof(gui_execs) / sizeof(gui_execs[0]); i++ para recorrer el bloque. */
    for (size_t i = 0; i < sizeof(gui_execs) / sizeof(gui_execs[0]); i++)
    {
        /* if: comprueba file_exists_gui(gui_execs[i]) && launch_exec_nonblocking(gui_execs[i]) antes de ejecutar esta rama. */
        if (file_exists_gui(gui_execs[i]) && launch_exec_nonblocking(gui_execs[i]))
            return 1;
    }

    return 0;
}
