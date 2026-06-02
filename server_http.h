/* servidor HTTP minimal para modo Docker/servicio
 * - Endpoint GET /health -> 200 OK
 * - Endpoint POST /batch -> acepta CSV en el cuerpo, procesa usando batch_process_file
 * Nota: Implementacion POSIX; en Windows se expone un stub que retorna error.
 */
#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

int run_http_server(const char *port);

#endif
