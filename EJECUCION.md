# ProgVoraz - Guia de ejecucion

Este proyecto calcula el cambio de monedas en dos modos:

- Modo `a`: monedas infinitas.
- Modo `b`: monedas limitadas por stock (actualiza `stock.txt`).

Tambien incluye:

- Interfaz de consola (TUI) para modo clasico.
- Interfaz grafica de ventana en Windows y macOS con panel de administrador.

## Requisitos

- GCC/Clang disponible en `PATH` (Windows, Linux o macOS).
- Archivos de datos en la carpeta del proyecto:
  - `monedas.txt`
  - `stock.txt`

Comprobacion rapida de GCC:

```bash
gcc --version
```

## Compilar

### Opcion Docker (recomendada para evitar toolchain local)

Compila y empaqueta consola + GUI portable dentro de contenedor Linux:

```bash
docker compose build
```

Ejecuta consola:

```bash
docker compose run --rm progvoraz-console
```

Ejecuta GUI portable (terminal):

```bash
docker compose run --rm progvoraz-gui-portable
```

Persistencia de archivos en Docker Compose:

- `monedas.txt` se monta en solo lectura desde la raiz del proyecto.
- `stock.txt` se monta en modo lectura/escritura desde la raiz del proyecto.
- Los cambios de stock realizados dentro del contenedor se reflejan en el `stock.txt` local.

Ejecuta pruebas automatizadas de modos `a`, `b`, `c`, GUI limitado y GUI ilimitado:

```bash
docker compose run --rm progvoraz-test
```

Prueba en host Windows (PowerShell):

```powershell
docker compose build
docker compose run --rm progvoraz-test
```

Prueba equivalente en WSL:

```bash
docker compose build
docker compose run --rm progvoraz-test
```

### Visual Studio Code (recomendado en este repositorio)

El proyecto incluye configuracion en `.vscode/`.

- Build consola: tarea `build: consola`.
- Build GUI: tarea `build: gui`.
- Run consola: tarea `run: consola`.
- Run GUI: tarea `run: gui`.
- Debug: perfiles `Debug Consola (Windows)` y `Debug GUI (Windows)`.

### Opcion recomendada (Makefile moderno)

Desde la raiz del proyecto:

```bash
make debug
```

Compilacion optimizada:

```bash
make release
```

GUI en Windows:

```bash
make gui
```

GUI portable en Linux (sin WinAPI):

```bash
make gui
```

## Modo ventana (GUI) - comandos directos

Nota Docker: el contenedor Linux cubre GUI portable en terminal (gui_portable.c). La GUI Win32 y la GUI nativa macOS se mantienen para ejecucion nativa en su sistema operativo.

### Compilar GUI en Windows (GCC directo)

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -O3 -flto gui_window.c moneda_gestion.c bigint.c algoritmo_cambio.c vector_dinamico.c -o progvoraz_gui.exe -mwindows
```

### Ejecutar GUI en Windows

```powershell
.\progvoraz_gui.exe
```

### Compilar y ejecutar GUI con Make

```bash
make gui
make run-gui
```

### Compilar GUI nativa en macOS (Swift/AppKit)

```bash
swiftc gui_macos.swift -o progvoraz_gui
./progvoraz_gui
```

### GUI portable en Linux (sin interfaz WinAPI)

En Linux, `make gui` compila `gui_portable.c`, que ofrece un panel administrador en terminal con operaciones de anadir/quitar stock.

### Opcion alternativa (GCC directo, Linux)

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O3 -flto main.c moneda_gestion.c bigint.c algoritmo_cambio.c vector_dinamico.c -o progvoraz
```

### Opcion alternativa (GCC directo, Windows)

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -O3 -flto main.c moneda_gestion.c bigint.c algoritmo_cambio.c vector_dinamico.c -o progvoraz.exe
```

## Ejecutar

Linux:

```bash
./progvoraz
```

Windows:

```powershell
.\progvoraz.exe
```

GUI Windows:

```powershell
.\progvoraz_gui.exe
```

GUI Linux/macOS:

```bash
./progvoraz_gui
```

Si usaste Make en Linux/macOS:

```bash
./progvoraz
```

## Flujo de uso

1. Elegir opcion:
   - `a` para monedas infinitas.
   - `b` para monedas limitadas.
2. Escribir nombre de moneda (por ejemplo: `euro`, `dolar`, `yen`).
3. Introducir cantidad en centimos.
4. Introducir `0` para salir.

Comandos de navegacion disponibles:

- `volver`: regresa al menu anterior donde aplique.
- `modo`: vuelve al menu de seleccion de modo.
- `salir`: cierra la aplicacion desde cualquier pantalla.

## Panel administrador (GUI)

1. Seleccionar moneda y pulsar `Cargar`.
2. Elegir denominacion.
3. Introducir cantidad (entero no negativo).
4. Pulsar `Anadir` o `Quitar`.

Tambien puedes operar por lote (como flujo administrador en consola, moneda por moneda):

1. En el cuadro de lote, escribir una cantidad por linea, respetando el orden de denominaciones mostrado en stock.
2. Pulsar `Anadir lote` o `Quitar lote` para aplicar todas las denominaciones en una sola operacion.

El panel persiste los cambios en `stock.txt` al instante.

## Modos en GUI

La GUI de Windows permite elegir modo directamente:

- `Stock limitado`: calcula devolucion usando stock actual y descuenta del archivo al confirmar.
- `Stock ilimitado`: calcula devolucion ignorando stock (no modifica `stock.txt`).

Para valores de stock muy grandes, las listas de stock y resultado incluyen scroll horizontal para ver el numero completo.

## Ejemplo de prueba automatizada (PowerShell)

Ejecuta una sesion en modo `b`, moneda `dolar`, cantidad `100` y luego sale:

```powershell
@"
b
dolar
100
0
"@ | .\progvoraz.exe
```

## Ejemplo de prueba automatizada (Linux Bash)

Ejecuta una sesion en modo `b`, moneda `dolar`, cantidad `100` y luego sale:

```bash
printf 'b\ndolar\n100\n0\n' | ./progvoraz
```

## Formato esperado de datos

### monedas.txt

Bloques por moneda:

```text
euro
200
100
50
...
dolar
100
50
25
...
```

### stock.txt

Mismo orden de bloques que `monedas.txt`:

- Un numero positivo representa stock disponible.
- `-1` representa sin stock (internamente se trata como `0`).

## Notas tecnicas de la mejora

- Se corrigio la lectura de archivos para evitar errores por uso de `feof`.
- Se corrigieron validaciones de limites en el vector dinamico.
- Se agrego validacion de entrada para opcion y cantidad.
- La actualizacion de stock reescribe `stock.txt` en el mismo archivo abierto (`r+`) sin temporales.
- Se modernizo el flujo para GitHub con `Makefile`, `.gitignore` y CI en `.github/workflows/ci.yml`.

## Nota sobre Windows Phone

Windows Phone / Windows 10 Mobile no dispone de runner oficial ni toolchain mantenida en GitHub Actions para este proyecto C/Win32.

Como cobertura equivalente en CI se ejecutan pruebas en `windows-latest` para:

- Consola: flujos de usuario en modo `a` y `b`.
- Consola: flujo de administrador en modo `c`.
- Ventana (Win32): compilacion y prueba de arranque de la GUI.

