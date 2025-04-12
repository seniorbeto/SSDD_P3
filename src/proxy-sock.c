#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "common/claves.h"
#include "common/encript.h"
#include "common/serialitation.h"
#include "common/lines.h"

#define MAX_MSG_SIZE 1024

// Códigos de operación
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

int get_ip_address(char *ip) {
  char *ip_local = getenv("IP_TUPLAS");
  if (!ip_local) {
    return -1;
  }
  strcpy(ip, ip_local);
  return 0;
}

int get_port(__uint16_t *port) {
  char *port_ch = getenv("PORT_TUPLAS");
  if (!port_ch) {
    return -1;
  }
  *port = (__uint16_t) atoi(port_ch);
  return 0;
}

static int send_request(request_t *req, response_t *resp) {
  int sfd;
  struct sockaddr_in server_addr;

  __uint16_t server_port;
  if (get_port(&server_port) < 0) {
    perror("[ERROR] al obtener el puerto del servidor. ¿Está definida la variable de entorno PORT_TUPLAS?");
    return -2;
  }

  // En la siguiente comprobación no hace falta comprobar que el puerto sea menor que 65535
  // ya que el tipo de dato __uint16_t no puede ser mayor que 65535.
  if (server_port < 1024) {
    perror("[ERROR] el puerto del servidor debe estar entre 1024 y 65535");
    return -1;
  }

  char *server_ip = (char *) malloc(32);
  if (get_ip_address(server_ip) < 0) {
    perror("[ERROR] al obtener la dirección IP del servidor. ¿Está definida la variable de entorno IP_TUPLAS?");
    return -2;
  }

  if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("[ERROR] al abrir el socket");
    return -2;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port); // Convertmos el puerto al formato de red (big-endian)

  struct hostent *server = gethostbyname(server_ip);
  if (!server) {
    perror("[ERROR] al obtener la dirección IP del servidor");
    free(server_ip);
    close(sfd);
    return -2;
  }
  free(server_ip);

  memcpy(&server_addr.sin_addr, server->h_addr, (size_t) server->h_length);

  if (connect(sfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    perror("[ERROR] al conectar con el servidor");
    close(sfd);
    return -2;
  }

  // Serializamos la petición mediante la librería "serialitation" (a xml)
  char *serial_req = format_request_to_xml(req);
  if (!serial_req) {
    perror("[ERROR] al serializar la petición");
    close(sfd);
    return -1;
  }

  const uint32_t key[4] = CIHPER_KEY;
  size_t output_len;
  unsigned char *encrypted_req = encrypt((const void *) serial_req, strlen(serial_req), &output_len, key);
  if (!encrypted_req) {
    perror("[ERROR] al encriptar la petición");
    free(serial_req);
    close(sfd);
    return -1;
  }
  free(serial_req);

  // Primero, enviamos la longitud del mensaje
  uint32_t msg_len_net = htonl((uint32_t) output_len);
  if (send_message(sfd, (char *) &msg_len_net, sizeof(uint32_t)) < 0) {
    perror("[ERROR] al enviar la longitud del mensaje al servidor");
    free(encrypted_req);
    close(sfd);
    return -2;
  }

  // Después, enviamos el cuerpo del mensaje (la petición encriptada)
  if (send_message(sfd, (char *) encrypted_req, output_len) < 0) {
    perror("[ERROR] al enviar la petición al servidor");
    free(encrypted_req);
    close(sfd);
    return -2;
  }
  free(encrypted_req);

  // Recibimos primero la longitud del mensaje
  uint32_t msg_len;
  if (recv_message(sfd, (char *) &msg_len, sizeof(uint32_t)) < 0) {
    perror("[ERROR] al recibir la longitud del mensaje del servidor");
    close(sfd);
    return -2;
  }
  msg_len = ntohl(msg_len); // Lo convertimos el endiannes del host

  if (msg_len > MAX_MSG_SIZE) {
    perror("[ERROR] tamaño de mensaje recibido es mayor que MAX_MSG_SIZE");
    close(sfd);
    return -2;
  }

  // Ahora reservamos memoria para el cuerpo del mensaje (la respuesta del servidor)
  unsigned char *msg = malloc(msg_len);
  if (!msg) {
    perror("[ERROR] al reservar memoria para la respuesta del servidor");
    close(sfd);
    return -1;
  }

  if (recv_message(sfd, (char *) msg, msg_len) < 0) {
    perror("[ERROR] al recibir la respuesta del servidor");
    free(msg);
    close(sfd);
    return -2;
  }

  // Desencriptamos la respuesta
  unsigned char *decrypted_resp = decrypt((const void *) msg, (size_t) msg_len, &output_len, key);
  if (!decrypted_resp) {
    perror("[ERROR] al desencriptar la respuesta");
    close(sfd);
    free(msg);
    return -1;
  }
  free(msg);

  // Deserializamos la respuesta
  if (parse_response_from_xml((char *)decrypted_resp, resp) < 0) {
    perror("[ERROR] al deserializar la respuesta");
    free(decrypted_resp);
    close(sfd);
    return -1;
  }
  free(decrypted_resp);
  close(sfd);
  return 0;
}

int destroy() {
  request_t req;
  response_t resp;
  memset(&req, 0, sizeof(req));
  req.op = OP_DESTROY;
  if (send_request(&req, &resp) != 0) {
    return -2;
  }
  return resp.result;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
  if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32)
    return -1;

  request_t req;
  response_t resp;
  memset(&req, 0, sizeof(req));
  req.op = OP_SET_VALUE;
  req.key = key;
  strncpy(req.value1, value1, 255);
  req.value1[255] = '\0';
  req.N_value2 = N_value2;
  memcpy(req.V_value2, V_value2, (size_t) N_value2 * sizeof(double));
  req.value3 = value3;

  if (send_request(&req, &resp) != 0) {
    return -2;
  }
  return resp.result;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
  request_t req;
  response_t resp;
  memset(&req, 0, sizeof(req));
  req.op = OP_GET_VALUE;
  req.key = key;

  if (send_request(&req, &resp) != 0) {
    return -2;
  }

  if (resp.result == 0) {
    strncpy(value1, resp.value1, 256);
    *N_value2 = resp.N_value2;
    memcpy(V_value2, resp.V_value2, (size_t) resp.N_value2 * sizeof(double));
    *value3 = resp.value3;
  }
  return resp.result;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
  if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32)
    return -1;

  request_t req;
  response_t resp;
  memset(&req, 0, sizeof(req));
  req.op = OP_MODIFY_VALUE;
  req.key = key;
  strncpy(req.value1, value1, 255);
  req.value1[255] = '\0';
  req.N_value2 = N_value2;
  memcpy(req.V_value2, V_value2, (size_t) N_value2 * sizeof(double));
  req.value3 = value3;

  if (send_request(&req, &resp) != 0) {
    return -2;
  }
  return resp.result;
}

int delete_key(int key) {
  request_t req;
  response_t resp;
  memset(&req, 0, sizeof(req));
  req.op = OP_DELETE_KEY;
  req.key = key;

  if (send_request(&req, &resp) != 0) {
    return -2;
  }
  return resp.result;
}

int exist(int key) {
  request_t req;
  response_t resp;
  memset(&req, 0, sizeof(req));
  req.op = OP_EXIST;
  req.key = key;

  if (send_request(&req, &resp) != 0) {
    return -2;
  }
  return resp.result;
}
