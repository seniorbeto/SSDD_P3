#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common/claves.h"
#include "common/encript.h"
#include "common/serialitation.h"
#include "common/lines.h"
#include "stdbool.h"

#define MAX_MSG_SIZE 1024

#define OP_DESTROY 0
#define OP_SET_VALUE 1
#define OP_GET_VALUE 2
#define OP_MODIFY_VALUE 3
#define OP_DELETE_KEY 4
#define OP_EXIST 5

/*
 * Lo suyo sería que el cliente y el servidor pactaran esta clave mediante
 * un algoritmo de criptografía asimétrica.
 */
#define CIHPER_KEY {0x12345678, 0x9abcdef0, 0x0fedcba9, 0xfedcba98}

pthread_mutex_t req_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t req_cond = PTHREAD_COND_INITIALIZER;
bool req_ready = false;

// Variables globales
int server_sock;

void handle_poweroff() {
  close(server_sock);
  if (destroy() != 0) {
    perror("\nPRECAUCIÓN: No se ha podido liberar la memoria del servidor correctamente.\n");
  }
  printf("\nSaliendo del servidor...\n");
  exit(EXIT_SUCCESS);
}

void *handle_request(void *arg) {
  int client_sock;

  pthread_mutex_lock(&req_lock);
  client_sock = *(int *) arg;
  free(arg);
  req_ready = true;
  pthread_cond_signal(&req_cond);
  pthread_mutex_unlock(&req_lock);

  printf("[INFO (sock %d)] Cliente conectado por socket con descriptor: %d\n", client_sock, client_sock);

  // Primero, recibimos la longitud del mensaje
  uint32_t msg_len_net;
  if (recv_message(client_sock, (char *)&msg_len_net, sizeof(uint32_t)) < 0) {
    perror("[ERROR] al recibir la longitud del mensaje");
    close(client_sock);
    return NULL;
  }
  uint32_t msg_len = ntohl(msg_len_net);

  if (msg_len > MAX_MSG_SIZE) {
    perror("[ERROR] el mensaje es demasiado grande");
    close(client_sock);
    return NULL;
  }

  printf("[INFO (sock %d)] Recibiendo mensaje cifrado de longitud %d...\n", client_sock, msg_len);

  // Reservamos memoria para el mensaje. Es importante destacar que viene cifrado y que por consiguiente,
  // es posible que contenga el caracter "\0" en mitad del mensaje.
  unsigned char *msg = (unsigned char *) malloc(msg_len);
  if (!msg) {
    perror("[ERROR] al reservar memoria para el mensaje");
    close(client_sock);
    return NULL;
  }

  if (recv_message(client_sock, (char *)msg, msg_len) < 0) {
    perror("[ERROR] al recibir el mensaje cifrado");
    free(msg);
    close(client_sock);
    return NULL;
  }

  request_t req;
  // Nos aseguramos de que la región de memoria que ocupa la variable 'req' que
  // acabamos de declarar esté a 0.
  memset(&req, 0, sizeof(req));

  printf("[INFO (sock %d)] Desencriptando petición...\n", client_sock);
  // Desencriptamos la petición
  const uint32_t key[4] = CIHPER_KEY;
  size_t output_len;
  unsigned char *decrypted_req = decrypt((const void *) msg, (size_t) msg_len, &output_len, key);
  if (!decrypted_req) {
    perror("[ERROR] al desencriptar la petición");
    close(client_sock);
    free(msg);
    return NULL;
  }

  printf("[INFO (sock %d)] Deserializando petición...\n", client_sock);
  if (parse_request_from_xml((char *) decrypted_req, &req) == -1) {
    perror("[ERROR] al deserializar la petición");
    free(decrypted_req);
    free(msg);
    close(client_sock);
    return NULL;
  }
  free(msg);
  free(decrypted_req);

  response_t resp;
  memset(&resp, 0, sizeof(resp));

  printf("[INFO (sock %d)] Petición correcta con código de operación: %d\n",client_sock, req.op);

  switch (req.op) {
    case OP_DESTROY:
      resp.result = destroy();
      break;
    case OP_SET_VALUE:
      resp.result = set_value(req.key, req.value1, req.N_value2, req.V_value2, req.value3);
      break;
    case OP_GET_VALUE:
      resp.result = get_value(req.key, resp.value1, &resp.N_value2, resp.V_value2, &resp.value3);
      break;
    case OP_MODIFY_VALUE:
      resp.result = modify_value(req.key, req.value1, req.N_value2, req.V_value2, req.value3);
      break;
    case OP_DELETE_KEY:
      resp.result = delete_key(req.key);
      break;
    case OP_EXIST:
      resp.result = exist(req.key);
      break;
    default:
      resp.result = -1;
      break;
  }
  printf("[INFO (sock %d)] Respuesta generada con código de resultado: %d\n", client_sock, resp.result);
  printf("[INFO (sock %d)] Serializando respuesta...\n", client_sock);
  // Serializamos la respuesta
  char *xml = format_response_to_xml(&resp);
  if (!xml) {
    perror("[ERROR] al serializar la respuesta");
    close(client_sock);
    return NULL;
  }

  printf("[INFO (sock %d)] Encriptando respuesta...\n", client_sock);
  // Encriptamos la respuesta
  unsigned char *encrypted_resp = encrypt((const void *) xml, strlen(xml), &output_len, key);
  if (!encrypted_resp) {
    perror("[ERROR] al encriptar la respuesta");
    free(xml);
    close(client_sock);
    return NULL;
  }

  printf("[INFO (sock %d)] Enviando respuesta al cliente...\n", client_sock);

  // Primero, enviamos la longitud del mensaje
  msg_len_net = htonl((uint32_t) output_len);
  if (send_message(client_sock, (char *)&msg_len_net, sizeof(uint32_t)) < 0) {
    perror("[ERROR] al enviar la longitud del mensaje");
    close(client_sock);
    free(encrypted_resp);
    return NULL;
  }

  // Ahora enviamos el cuerpo del mensaje (la respuesta encriptada)
  if (send_message(client_sock, (char *)encrypted_resp, output_len) < 0) {
    perror("[ERROR] al enviar la respuesta encriptada");
    close(client_sock);
    free(encrypted_resp);
    return NULL;
  }

  free(encrypted_resp);
  free(xml);
  close(client_sock);

  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    perror("Uso: ./servidor-sock <puerto>");
    exit(EXIT_FAILURE);
  }
  __uint16_t port = (__uint16_t) atoi(argv[1]);
  // No es necesario comprobar si el puerto es mayor que 65535 porque el tipo de dato
  // __uint16_t no puede almacenar un número mayor que 65535.
  if (port < 1024) {
    perror("El puerto debe estar entre 1024 y 65535");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr;

  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("[ERROR] al crear el socket del servidor");
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, handle_poweroff);
  signal(SIGTERM, handle_poweroff); // Para pararlo en CLion

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // El siguiente código es para evitar el error "Address already in use" y poder reutilizar el puerto
  // Esto pasa porque el puerto se queda en estado TIME_WAIT durante un tiempo después de cerrar el socket.
  int opt = 1;
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("[ERROR] al establecer SO_REUSEADDR");
    close(server_sock);
    exit(EXIT_FAILURE);
  }

  // Le damos al socket una dirección
  if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
    perror("[ERROR] al enlazar el socket del servidor");
    close(server_sock);
    exit(EXIT_FAILURE);
  }

  // Marcamos al socket en modo escucha
  if (listen(server_sock, 10) == -1) {
    perror("[ERROR] al poner el servidor en modo escucha");
    close(server_sock);
    exit(EXIT_FAILURE);
  }

  printf("Servidor iniciado. Escuchando en el puerto %d. Esperando peticiones...\n", port);

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    // Para aceptar la conexión, utilizaremos un puntero a un entero para poder pasarlo al hilo.
    // La región de memoria de este entero la reservamos con calloc para asegurarnos de que está a 0.
    int *client_sock = calloc(1, sizeof(int));
    if (!client_sock) {
      perror("[ERROR] al reservar memoria para el socket del cliente");
      continue;
    }

    *client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_addr_len);
    if (*client_sock == -1) {
      perror("[ERROR] al aceptar la conexión del cliente");
      free(client_sock);
      continue;
    }

    // Lanzamos un hilo para manejar la petición del cliente
    pthread_t tid;
    if (pthread_create(&tid, NULL, (void *(*) (void *) ) handle_request, client_sock) != 0) {
      perror("[ERROR] al crear hilo para el cliente");
      close(*client_sock);
      free(client_sock);
    } else {
      pthread_mutex_lock(&req_lock);
      while (!req_ready) {
        pthread_cond_wait(&req_cond, &req_lock);
      }
      req_ready = false;
      pthread_mutex_unlock(&req_lock);
      pthread_detach(tid);
    }
  }
  return 0;
}
