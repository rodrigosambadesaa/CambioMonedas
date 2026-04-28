# Documentacion de funciones C (ProgVoraz)

Este documento describe todas las funciones definidas en los archivos `.c` del proyecto.

## bigint.c

- `duplicar(const char *s)`: Reserva memoria y devuelve una copia de la cadena recibida.
- `texto_valido_entero_no_negativo(const char *text)`: Verifica que un texto represente un entero no negativo valido.
- `normalizar(const char *text)`: Normaliza un entero textual eliminando ceros a la izquierda y devolviendo una nueva cadena.
- `bigint_init(BigInt *n, const char *text)`: Inicializa un `BigInt` desde texto decimal no negativo.
- `bigint_set(BigInt *dest, const BigInt *src)`: Copia el valor de un `BigInt` origen al destino.
- `bigint_free(BigInt *n)`: Libera recursos internos de un `BigInt`.
- `bigint_compare(const BigInt *a, const BigInt *b)`: Compara dos `BigInt` (`-1`, `0`, `1`).
- `bigint_is_zero(const BigInt *n)`: Indica si un `BigInt` vale cero.
- `bigint_add(const BigInt *a, const BigInt *b, BigInt *out)`: Suma dos `BigInt`.
- `bigint_subtract(const BigInt *a, const BigInt *b, BigInt *out)`: Resta `b` a `a` (si `a >= b`).
- `bigint_multiply(const BigInt *a, const BigInt *b, BigInt *out)`: Multiplica dos `BigInt`.
- `bigint_divmod(const BigInt *a, const BigInt *b, BigInt *quotient, BigInt *remainder)`: Division entera con cociente y residuo.
- `bigint_array_create(BigIntArray *arr, size_t len)`: Crea un arreglo de `BigInt` de longitud `len`.
- `bigint_array_set(BigIntArray *arr, size_t idx, const BigInt *value)`: Asigna un elemento del arreglo en `idx`.
- `bigint_array_free(BigIntArray *arr)`: Libera el arreglo de `BigInt` y su contenido.

## algoritmo_cambio.c

- `calcular_cambio_optimo(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion)`: Calcula una solucion de cambio en modo ilimitado. Para importes acotados usa programacion dinamica y minimiza el numero de monedas; para importes BigInt muy grandes conserva la busqueda descendente compatible.
- `calcular_cambio_optimo_stock(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, BigIntArray *solucion)`: Calcula cambio respetando stock. Para importes acotados minimiza el numero de monedas con programacion dinamica y, si el importe excede ese rango, usa la ruta BigInt previa con poda por stock.

## moneda_gestion.c

- `es_texto_bigint_valido(const char *texto)`: Valida que un token contenga solo digitos decimales.
- `duplicar_cadena(const char *texto)`: Duplica una cadena en memoria dinamica.
- `recortar_fin_linea(char *texto)`: Elimina `\r`/`\n` al final de una linea.
- `cargar_datos_moneda(const char *archivo, const char *nombreMoneda, int convertirMenosUnoACero, BigIntArray *resultado)`: Carga bloque de denominaciones o stock de una moneda desde archivo.
- `cargar_denominaciones_moneda(const char *nombreMoneda, BigIntArray *resultado)`: Carga denominaciones desde `monedas.txt`.
- `cargar_stock_moneda(const char *nombreMoneda, BigIntArray *resultado)`: Carga stock desde `stock.txt` normalizando `-1` a `0`.
- `actualizar_stock_moneda(const char *nombreMoneda, const BigIntArray *stock)`: Persistencia del bloque de stock de una moneda en `stock.txt`.

## main.c

- `limpiar_pantalla(void)`: Limpia pantalla de consola con secuencias ANSI.
- `dibujar_linea(void)`: Imprime una linea decorativa del menu.
- `dibujar_titulo(void)`: Muestra encabezado principal del sistema.
- `leer_linea(char *buffer, size_t tam)`: Lee una linea de entrada y recorta salto de linea.
- `a_minusculas(char *texto)`: Convierte un texto a minusculas.
- `normalizar_clave(const char *origen, char *destino, size_t tamDestino)`: Normaliza nombre de moneda (minusculas y sin acentos comunes).
- `es_token_denominacion(const char *token)`: Determina si un token es numerico (o `-1`).
- `clave_ya_existe(char claves[][MAX_MONEDA_NOMBRE + 1], size_t cantidad, const char *clave)`: Comprueba duplicados de clave.
- `copiar_clave(char destino[MAX_MONEDA_NOMBRE + 1], const char *origen)`: Copia segura de clave de moneda.
- `cargar_claves_monedas(const char *archivo, char claves[][MAX_MONEDA_NOMBRE + 1], size_t maxClaves, size_t *cantidad)`: Extrae nombres de monedas desde archivo.
- `imprimir_lista_monedas(const char claves[][MAX_MONEDA_NOMBRE + 1], size_t cantidad)`: Imprime monedas disponibles.
- `mostrar_monedas_disponibles(int opcion)`: Lista monedas segun modo (ilimitado/limitado).
- `pedir_opcion(void)`: Solicita opcion principal de modo de trabajo.
- `pedir_cantidad(BigInt *cantidad)`: Solicita cantidad en centimos para calculo de cambio.
- `pedir_cantidad_admin(BigInt *cantidad)`: Solicita cantidad para operaciones de administrador.
- `pedir_indice_denominacion(size_t maximo, size_t *indice)`: Solicita indice de denominacion valido.
- `copiar_arreglo_bigint(const BigIntArray *origen, BigIntArray *destino)`: Duplica arreglo de `BigInt`.
- `limpiar_arreglo(BigIntArray *arr)`: Libera arreglo de `BigInt`.
- `imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock)`: Muestra resultado de cambio.
- `imprimir_stock_administrador(const BigIntArray *monedas, const BigIntArray *stock)`: Muestra panel de stock en consola.
- `aplicar_cambio_administrador(BigIntArray *stock, size_t idx, const BigInt *delta, int esSuma)`: Suma o resta stock de una denominacion.
- `pedir_subopcion_cambio(void)`: Solicita subopcion de cambio (`tradicional`/`especifico`).
- `pedir_cantidades_por_denominacion(const BigIntArray *monedas, const char *titulo, BigIntArray *cantidades)`: Captura cantidades por cada denominacion.
- `validar_cambio_especifico_ilimitado(const BigIntArray *monedas, const BigIntArray *entregadas, const BigIntArray *devolucion, BigInt *totalEntregado, BigInt *totalDevolucion)`: Valida en modo ilimitado que los totales entregado/devuelto coincidan.
- `calcular_total_valor(const BigIntArray *monedas, const BigIntArray *cantidades, BigInt *total)`: Calcula total monetario de un vector de cantidades.
- `main(void)`: Orquesta flujo principal de la aplicacion en consola (modos `a`, `b`, `c`).

## gui_window.c

- `es_numero(const char *s)`: Valida texto numerico entero no negativo.
- `liberar_datos_moneda(void)`: Libera denominaciones/stock cargados en memoria GUI.
- `cargar_nombres_moneda(void)`: Carga lista de monedas disponibles desde `monedas.txt`.
- `refrescar_combo_monedas(void)`: Refresca `ComboBox` de monedas.
- `mostrar_error(const char *titulo, const char *mensaje)`: Muestra mensaje modal de error.
- `mostrar_info(const char *titulo, const char *mensaje)`: Muestra mensaje modal informativo.
- `limpiar_resultado_cambio(void)`: Limpia lista de resultados de devolucion.
- `configurar_modo_stock(int limitado)`: Ajusta modo de trabajo (limitado/ilimitado) y habilitacion de controles.
- `precargar_ceros_en_edit(HWND edit)`: Precarga un `EDIT` multilinea con ceros por denominacion.
- `precargar_cantidades_lote(void)`: Precarga cajas de lote/cambio especifico con ceros.
- `leer_cantidades_desde_edit(HWND edit, BigIntArray *deltas)`: Parsea cantidades desde un control `EDIT` multilinea.
- `leer_cantidades_lote(BigIntArray *deltas)`: Alias especializado para leer lote de administrador.
- `ajustar_scroll_horizontal_lista(HWND lista, int max_chars)`: Ajusta ancho de scroll horizontal del `LISTBOX`.
- `copiar_arreglo_bigint(const BigIntArray *origen, BigIntArray *destino)`: Duplica arreglo de `BigInt`.
- `refrescar_lista_stock(void)`: Redibuja lista de stock/denominaciones y limpia resultados.
- `calcular_cambio_stock(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, BigIntArray *solucion)`: Calcula devolucion con stock disponible.
- `calcular_cambio_ilimitado(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion)`: Calcula devolucion en modo ilimitado.
- `leer_monto_cambio(BigInt *monto)`: Lee y valida monto desde la caja de texto de monto.
- `calcular_y_mostrar_cambio(void)`: Ejecuta calculo de devolucion y actualiza UI/persistencia segun modo.
- `cargar_moneda_seleccionada(void)`: Carga denominaciones y (si aplica) stock de la moneda seleccionada.
- `leer_cantidad_delta(BigInt *delta)`: Lee cantidad de ajuste de stock desde UI.
- `aplicar_cambio_stock(int esSuma)`: Aplica suma/resta simple de stock por denominacion.
- `aplicar_cambio_stock_lote(int esSuma)`: Aplica lote de sumas/restas de stock.
- `calcular_total_valor(const BigIntArray *denom, const BigIntArray *cantidades, BigInt *total)`: Calcula total monetario de cantidades por denominacion.
- `aplicar_cambio_especifico(void)`: Aplica cambio especifico en GUI (limitado e ilimitado segun modo actual).
- `crear_controles(HWND hwnd)`: Crea todos los controles de la ventana principal.
- `wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)`: Procedimiento principal de mensajes Win32.
- `WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)`: Punto de entrada GUI de Windows.
- `main(void)`: Stub alternativo para compilaciones fuera de Windows.

## gui_portable.c

- `leer_linea(char *buffer, size_t tam)`: Lee linea de consola y recorta salto.
- `a_minusculas(char *texto)`: Convierte cadena a minusculas.
- `es_numero(const char *s)`: Valida texto entero no negativo.
- `cargar_nombres_moneda(char monedas[MAX_MONEDAS][MAX_NOMBRE], int *cantidad)`: Carga nombres de monedas disponibles.
- `imprimir_panel(const char *moneda, const BigIntArray *denom, const BigIntArray *stock)`: Imprime panel portable de denominaciones/stock.
- `pedir_indice(size_t max, size_t *indice)`: Solicita indice valido de denominacion.
- `pedir_cantidad(BigInt *delta)`: Solicita cantidad para admin portable.
- `pedir_monto(BigInt *monto)`: Solicita monto para devolucion.
- `copiar_arreglo_bigint(const BigIntArray *origen, BigIntArray *destino)`: Duplica arreglo de `BigInt`.
- `calcular_cambio_stock(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, BigIntArray *solucion)`: Devuelve solucion de cambio en modo limitado.
- `calcular_cambio_ilimitado(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion)`: Devuelve solucion de cambio en modo ilimitado.
- `pedir_cantidades_por_denominacion(const BigIntArray *denom, const char *titulo, BigIntArray *cantidades)`: Captura cantidades para cambio especifico.
- `calcular_total_valor(const BigIntArray *denom, const BigIntArray *cantidades, BigInt *total)`: Calcula total monetario por denominaciones.
- `aplicar_cambio_especifico(const char *moneda, const BigIntArray *denom, BigIntArray *stock, const BigIntArray *entregadas, const BigIntArray *devolucion)`: Aplica cambio especifico en modo limitado y persiste stock.
- `validar_cambio_especifico_ilimitado(const BigIntArray *denom, const BigIntArray *entregadas, const BigIntArray *devolucion)`: Verifica en modo ilimitado que el total entregado y el total solicitado sean equivalentes.
- `imprimir_resultado_cambio(const BigInt *monto, const BigIntArray *denom, const BigIntArray *solucion)`: Muestra desglose de devolucion.
- `calcular_y_aplicar_cambio(ModoGUI modo, const char *moneda, const BigIntArray *denom, BigIntArray *stock)`: Ejecuta calculo tradicional y persiste stock cuando corresponde.
- `pedir_modo(ModoGUI *modo)`: Solicita modo `limitado`/`ilimitado`.
- `main(void)`: Punto de entrada portable para Linux/Unix.
- `main(void)`: Stub alternativo cuando no aplica compilacion portable.

## vector_dinamico.c

- `crear(vectorP *v1, unsigned long tam1)`: Reserva e inicializa vector dinamico de longitud dada.
- `asignar(vectorP v1, unsigned long posicion, TELEMENTO valor)`: Asigna valor en una posicion del vector.
- `liberar(vectorP *v1)`: Libera memoria del vector dinamico.
- `recuperar(vectorP v1, unsigned long posicion)`: Recupera valor almacenado en una posicion.
- `tamano(vectorP *v1)`: Devuelve longitud actual del vector.
