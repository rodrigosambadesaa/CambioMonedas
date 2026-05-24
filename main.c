#include <string.h>
#include "app_console.h"
#include "batch_cli.h"

int main(int argc, char **argv)
{
    const char *input = NULL;
    const char *output = NULL;
    const char *logpath = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            input = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output = argv[++i];
        else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc)
            logpath = argv[++i];
    }

    if (input != NULL)
    {
        if (!batch_process_file(input, output, logpath))
            return 2;
        return 0;
    }

    return app_console_run();
}
