#include "gui_launcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

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
    /* if: comprueba exec_path == NULL || exec_path[0] == '\0' antes de ejecutar esta rama. */
    if (exec_path == NULL || exec_path[0] == '\0')
        return 0;

#ifdef _WIN32
    STARTUPINFOA startup_info;
    PROCESS_INFORMATION process_info;
    char cmdline[1024];

    memset(&startup_info, 0, sizeof(startup_info));
    memset(&process_info, 0, sizeof(process_info));
    startup_info.cb = sizeof(startup_info);

    snprintf(cmdline, sizeof(cmdline), "\"%s\"", exec_path);
    /* if: comprueba !CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &startup_info, &process_info) antes de ejecutar esta rama. */
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                        DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                        NULL, NULL, &startup_info, &process_info))
        return 0;

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return 1;
#else
    char cmd[1024];

    snprintf(cmd, sizeof(cmd), "setsid \"%s\" >/dev/null 2>&1 &", exec_path);
    return system(cmd) == 0;
#endif
}

/* funcion progvoraz_launch_gui_nonblocking: contiene la logica principal de esta operacion. */
int progvoraz_launch_gui_nonblocking(void)
{
#ifdef _WIN32
    const char *gui_execs[] = {"progvoraz_gui.exe", ".\\progvoraz_gui.exe"};
#elif defined(__APPLE__)
    const char *gui_execs[] = {"./progvoraz_gui", "progvoraz_gui", "./progvoraz_gui_swift", "progvoraz_gui_swift", "./progvoraz_gui_portable", "progvoraz_gui_portable"};
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
