#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <rpc/rpc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common/claves.h"
#include "common/encript.h"
#include "rpc_api.h"
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

int *destroy_1_svc(void *arg, struct svc_req *rqstp) {
  static int result;
  /* Se llama a la función interna (ya adaptada) para destruir/eliminar recursos */
  result = destroy();
  printf("[INFO] destroy_1_svc invocado, resultado: %d\n", result);
  return &result;
}

/* Implementación de la operación SET_VALUE */
response_t *set_value_1_svc(request_t *req, struct svc_req *rqstp) {
  static response_t resp;
  memset(&resp, 0, sizeof(resp));

  printf("[INFO] set_value_1_svc: clave=%d\n", req->key);
  /* Se llama a la función de lógica interna para set_value.
     Se asume que req->value1.value1_val es la cadena (ya desempaquetada) y
     que req->V_value2.V_value2_val es el array de doubles. */
  struct Coord value3;
  value3.x = req->value3.x;
  value3.y = req->value3.y;
  resp.result = set_value(req->key, req->value1.value1_val, req->N_value2, req->V_value2.V_value2_val, value3);
  return &resp;
}

/* Implementación de la operación GET_VALUE */
response_t *get_value_1_svc(request_t *req, struct svc_req *rqstp) {
  static response_t resp;
  /* Reservamos buffers estáticos para la cadena y el arreglo de doubles.
     Se asume un máximo de 256 caracteres para value1 y hasta 32 doubles en V_value2 */
  static char value1_buffer[256];
  static double V_value2_buffer[32];
  struct Coord value3;

  memset(&resp, 0, sizeof(resp));

  /* Asignamos los buffers a los campos de la respuesta */
  resp.value1.value1_val = value1_buffer;
  resp.V_value2.V_value2_val = V_value2_buffer;

  printf("[INFO] get_value_1_svc: clave=%d\n", req->key);

  /* Llamamos a la función de negocio; se espera que ésta llene los buffers pasados */
  resp.result = get_value(req->key, value1_buffer, &resp.N_value2, V_value2_buffer, &value3);

  /* Asignamos la longitud de la cadena obtenida (suponiendo que get_value retorna una cadena terminada en '\0') */
  resp.value1.value1_len = strlen(value1_buffer);
  /* Establecemos el tamaño del arreglo de doubles de salida igual al número devuelto en N_value2 */
  resp.V_value2.V_value2_len = resp.N_value2;

  /* Se asigna la estructura Coord obtenida a la respuesta */
  resp.value3.x = value3.x;
  resp.value3.y = value3.y;

  return &resp;
}
/* Implementación de la operación MODIFY_VALUE */
response_t *modify_value_1_svc(request_t *req, struct svc_req *rqstp) {
  static response_t resp;
  memset(&resp, 0, sizeof(resp));

  printf("[INFO] modify_value_1_svc: clave=%d\n", req->key);
  struct Coord value3;
  value3.x = req->value3.x;
  value3.y = req->value3.y;
  resp.result = modify_value(req->key, req->value1.value1_val, req->N_value2, req->V_value2.V_value2_val, value3);
  return &resp;
}

/* Implementación de la operación DELETE_KEY */
response_t *delete_key_1_svc(request_t *req, struct svc_req *rqstp) {
  static response_t resp;
  memset(&resp, 0, sizeof(resp));

  printf("[INFO] delete_key_1_svc: clave=%d\n", req->key);
  resp.result = delete_key(req->key);
  return &resp;
}

/* Implementación de la operación EXIST */
response_t *exist_1_svc(request_t *req, struct svc_req *rqstp) {
  static response_t resp;
  memset(&resp, 0, sizeof(resp));


  printf("[INFO] exist_1_svc: clave=%d\n", req->key);
  resp.result = exist(req->key);
  return &resp;
}
