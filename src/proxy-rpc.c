#include <arpa/inet.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "common/claves.h"
#include "rpc_api.h"

#define MAX_MSG_SIZE 1024

// Códigos de operación
#define OP_DESTROY 0
#define OP_SET_VALUE 1
#define OP_GET_VALUE 2
#define OP_MODIFY_VALUE 3
#define OP_DELETE_KEY 4
#define OP_EXIST 5

int get_ip_address(char *ip) {
  char *ip_local = getenv("IP_TUPLAS");
  if (!ip_local) {
    return -1;
  }
  strcpy(ip, ip_local);
  return 0;
}

/* Función auxiliar que crea el cliente RPC */
static CLIENT *get_rpc_client() {
  char server_ip[32];
  if (get_ip_address(server_ip) < 0) {
    fprintf(stderr, "[ERROR] No se pudo obtener la dirección IP del servidor.\n");
    return NULL;
  }
  /* KEYS y VKEYS son los identificadores de programa y versión definidos en el .x */
  CLIENT *clnt = clnt_create(server_ip, KEYS, VKEYS, "udp");
  if (clnt == NULL) {
    clnt_pcreateerror(server_ip);
  }
  return clnt;
}

/* Implementación de destroy() usando RPC */
int destroy() {
  CLIENT *clnt = get_rpc_client();
  if (!clnt)
    {return -2;}

  int *result = destroy_1(NULL, clnt);
  if (result == NULL) {
    clnt_perror(clnt, "Error en destroy_1");
    clnt_destroy(clnt);
    return -2;
  }
  int ret = *result;
  clnt_destroy(clnt);
  return ret;
}

/* Implementación de set_value() usando RPC */
int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
  if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32)
    {return -1;}

  request_t req;
  memset(&req, 0, sizeof(req));
  req.op = OP_SET_VALUE;
  req.key = key;
  req.value1.value1_len = strlen(value1);
  req.value1.value1_val = value1; /* Asumimos que 'value1' está correctamente asignado */
  req.N_value2 = N_value2;
  req.V_value2.V_value2_len = N_value2;
  req.V_value2.V_value2_val = V_value2;
  req.value3.x = value3.x;
  req.value3.y = value3.y;

  CLIENT *clnt = get_rpc_client();
  if (!clnt)
    {return -2;}

  response_t *resp = set_value_1(&req, clnt);
  if (resp == NULL) {
    clnt_perror(clnt, "Error en set_value_1");
    clnt_destroy(clnt);
    return -2;
  }
  int ret = resp->result;
  clnt_destroy(clnt);
  return ret;
}

/* Implementación de get_value() usando RPC */
int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
  request_t req;
  memset(&req, 0, sizeof(req));
  req.op = OP_GET_VALUE;
  req.key = key;

  CLIENT *clnt = get_rpc_client();
  if (!clnt)
    {return -2;}

  response_t *resp = get_value_1(&req, clnt);
  if (resp == NULL) {
    clnt_perror(clnt, "Error en get_value_1");
    clnt_destroy(clnt);
    {return -2;}
  }
  int ret = resp->result;
  if (ret == 0) {
    /* Copiamos los datos recibidos a las variables proporcionadas */
    if (resp->value1.value1_val != NULL && resp->value1.value1_len < 256) {
      strncpy(value1, resp->value1.value1_val, resp->value1.value1_len);
      value1[resp->value1.value1_len] = '\0';
    }
    *N_value2 = resp->N_value2;
    memcpy(V_value2, resp->V_value2.V_value2_val, resp->N_value2 * sizeof(double));
    value3->x = resp->value3.x;
    value3->y = resp->value3.y;
  }
  clnt_destroy(clnt);
  return ret;
}

/* Implementación de modify_value() usando RPC */
int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
  if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32)
    {return -1;}

  request_t req;
  memset(&req, 0, sizeof(req));
  req.op = OP_MODIFY_VALUE;
  req.key = key;
  req.value1.value1_len = strlen(value1);
  req.value1.value1_val = value1;
  req.N_value2 = N_value2;
  req.V_value2.V_value2_len = N_value2;
  req.V_value2.V_value2_val = V_value2;
  req.value3.x = value3.x;
  req.value3.y = value3.y;

  CLIENT *clnt = get_rpc_client();
  if (!clnt)
    {return -2;}

  response_t *resp = modify_value_1(&req, clnt);
  if (resp == NULL) {
    clnt_perror(clnt, "Error en modify_value_1");
    clnt_destroy(clnt);
    return -2;
  }
  int ret = resp->result;
  clnt_destroy(clnt);
  return ret;
}

/* Implementación de delete_key() usando RPC */
int delete_key(int key) {
  request_t req;
  memset(&req, 0, sizeof(req));
  req.op = OP_DELETE_KEY;
  req.key = key;

  CLIENT *clnt = get_rpc_client();
  if (!clnt)
    {return -2;}

  response_t *resp = delete_key_1(&req, clnt);
  if (resp == NULL) {
    clnt_perror(clnt, "Error en delete_key_1");
    clnt_destroy(clnt);
    return -2;
  }
  int ret = resp->result;
  clnt_destroy(clnt);
  return ret;
}

/* Implementación de exist() usando RPC */
int exist(int key) {
  request_t req;
  memset(&req, 0, sizeof(req));
  req.op = OP_EXIST;
  req.key = key;

  CLIENT *clnt = get_rpc_client();
  if (!clnt)
    {
    return -2;
  }

  response_t *resp = exist_1(&req, clnt);
  if (resp == NULL) {
    clnt_perror(clnt, "Error en exist_1");
    clnt_destroy(clnt);
    return -2;
  }
  int ret = resp->result;
  clnt_destroy(clnt);
  return ret;
}
