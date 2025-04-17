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
#include "rpc_api.h"
#include "stdbool.h"

#define MAX_MSG_SIZE 1024

#define OP_DESTROY 0
#define OP_SET_VALUE 1
#define OP_GET_VALUE 2
#define OP_MODIFY_VALUE 3
#define OP_DELETE_KEY 4
#define OP_EXIST 5

int *destroy_1_svc(void *arg, struct svc_req *rqstp) {
  static int result;
  result = destroy();
  printf("[INFO] destroy_1_svc invocado, resultado: %d\n", result);
  return &result;
}

/* Implementación de la operación SET_VALUE */
response_t *set_value_1_svc(request_t *req, struct svc_req *rqstp) {
  static response_t resp;
  memset(&resp, 0, sizeof(resp));

  printf("[INFO] set_value_1_svc: clave=%d\n", req->key);
  struct Coord value3;
  value3.x = req->value3.x;
  value3.y = req->value3.y;
  resp.result = set_value(req->key, req->value1.value1_val, req->N_value2, req->V_value2.V_value2_val, value3);
  return &resp;
}

/* Implementación de la operación GET_VALUE */
response_t *get_value_1_svc(request_t *req, struct svc_req *rqstp) {
  static response_t resp;
  static char value1_buffer[256];
  static double V_value2_buffer[32];
  struct Coord value3;

  memset(&resp, 0, sizeof(resp));
  resp.value1.value1_val = value1_buffer;
  resp.V_value2.V_value2_val = V_value2_buffer;

  printf("[INFO] get_value_1_svc: clave=%d\n", req->key);

  resp.result = get_value(req->key, value1_buffer, &resp.N_value2, V_value2_buffer, &value3);

  resp.value1.value1_len = strlen(value1_buffer);
  resp.V_value2.V_value2_len = resp.N_value2;

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
