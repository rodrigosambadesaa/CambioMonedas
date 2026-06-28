# Auditoría reproducible de memoria y recursos

Este proyecto incluye una batería única para validar fugas de memoria, errores de acceso y cierre de recursos en los modos CLI/batch/stream/exportación.

## Comando único

```bash
make memory-safety
```

Equivale a ejecutar sanitizers, Valgrind, comprobación de file descriptors y estrés.

## Docker

```bash
docker build -f docker/Dockerfile.debug -t progvoraz-debug .
docker run --rm --memory=256m progvoraz-debug
docker run --rm --memory=256m progvoraz-debug make test-sanitize
docker run --rm --memory=256m progvoraz-debug make valgrind
docker run --rm --memory=256m progvoraz-debug make fd-check
docker run --rm --memory=256m progvoraz-debug make stress-test
```

## Targets disponibles

| Target | Qué verifica |
|---|---|
| `make debug-sanitize` | Compila con `-g -O1 -fsanitize=address,leak,undefined -fno-omit-frame-pointer`. |
| `make test-sanitize` | Ejecuta unitarias y modos CLI principales con ASan/LSan/UBSan. |
| `make valgrind` | Ejecuta Valgrind con `--leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=definite,possible --error-exitcode=1`. |
| `make fd-check` | Repite ejecuciones y comprueba que `/proc/<pid>/fd` no crece; usa `strace` si está instalado. |
| `make stress-test` | Ejecuta 200 entradas variadas y registra RSS pico con `/usr/bin/time`. |
| `make memory-safety` | Batería completa. |

## Matriz de modos probados

| Modo probado | Comando usado | Resultado esperado | Recursos verificados |
|---|---|---|---|
| Unitario BigInt/algoritmos | `./.dist/test_progvoraz` | Código 0 sin errores sanitizer/Valgrind | `malloc/calloc/realloc/free` internos. |
| Ayuda/version | `./progvoraz --help`, `./progvoraz --version` | Código 0 sin fugas | `FILE*` de `VERSIONES.md` y salida temprana. |
| Batch por archivo | `./progvoraz --input tests/sample_batch.csv --output /tmp/progvoraz_*.csv --log /tmp/progvoraz_*.log` | Código 0 sin fugas | `FILE*` entrada/salida/log y BigInt temporales. |
| Stream/Docker | `printf ... | ./progvoraz --stream` | Código 0 sin fugas | stdin/stdout, líneas dinámicas, rutas de error por moneda/monto inválido. |
| Export stock JSON | `./progvoraz --export-stock-json /tmp/progvoraz_stock.json` | Código 0 sin fugas | `FILE*` JSON y archivos de monedas/stock. |
| Export report | `./progvoraz --export-report /tmp/progvoraz_report.txt` | Código 0 sin fugas | `FILE*` de reporte y carga de inventario. |
| File descriptors | `scripts/fd_check.sh` | FD final no supera el baseline en más de 3 | `fopen/fclose`, `open/close`, rutas repetidas, `strace` opcional. |
| Estrés | `scripts/stress_test.sh` | 200 ejecuciones terminan con RSS acotado | Reservas dinámicas repetidas y salidas tempranas. |

## Hallazgos corregidos

- Se corrigió una fuga al saltar la cabecera CSV en modo batch/stream: la línea leída dinámicamente ahora se libera antes de `continue`.
- Se corrigió un acceso inválido potencial en el recorte de cadenas vacías: ya no se calcula `s + strlen(s) - 1` cuando la cadena recortada queda vacía.
- Se endureció el servidor HTTP ante cuerpos incompletos y lecturas de archivos temporales fallidas: libera el cuerpo, cierra sockets/archivos y valida `malloc`/`fread`.

## Pendientes

No se automatiza la GUI interactiva porque requiere entorno gráfico. La auditoría cubre el binario principal, batch, stream, exportaciones y rutas HTTP internas que compilan en el binario; para pruebas funcionales de servidor con tráfico real se recomienda ejecutar el contenedor debug y añadir peticiones `curl` específicas del despliegue.
