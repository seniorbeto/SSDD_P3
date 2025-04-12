#include "claves.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {
  int key;
  char value1[256];
  int N_value2;
  double V_value2[32];
  struct Coord value3;
  struct node *next;
} node_t;

static node_t *head = NULL;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int destroy() {
  pthread_mutex_lock(&lock);
  node_t *current = head;
  while (current) {
    node_t *temp = current;
    current = current->next;
    free(temp);
  }
  head = NULL;
  pthread_mutex_unlock(&lock);
  return 0;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
  if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32) {
    return -1;
  }

  pthread_mutex_lock(&lock);
  node_t *cur = head;
  while (cur) {
    if (cur->key == key) {
      pthread_mutex_unlock(&lock);
      return -1;
    }
    cur = cur->next;
  }

  node_t *new_node = malloc(sizeof(node_t));
  if (!new_node) {
    pthread_mutex_unlock(&lock);
    return -1;
  }
  new_node->key = key;
  strncpy(new_node->value1, value1, 255);
  new_node->value1[255] = '\0';
  new_node->N_value2 = N_value2;
  memcpy(new_node->V_value2, V_value2, (size_t) N_value2 * sizeof(double));
  new_node->value3 = value3;
  new_node->next = head;
  head = new_node;
  pthread_mutex_unlock(&lock);
  return 0;
}


int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
  pthread_mutex_lock(&lock);
  node_t *cur = head;
  while (cur) {
    if (cur->key == key) {
      strncpy(value1, cur->value1, 256);
      *N_value2 = cur->N_value2;
      memcpy(V_value2, cur->V_value2, (size_t)cur->N_value2 * sizeof(double));
      *value3 = cur->value3;
      pthread_mutex_unlock(&lock);
      return 0;
    }
    cur = cur->next;
  }
  pthread_mutex_unlock(&lock);
  return -1;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
  if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32) {
    return -1;
  }
  pthread_mutex_lock(&lock);
  node_t *cur = head;
  while (cur) {
    if (cur->key == key) {
      strncpy(cur->value1, value1, 255);
      cur->value1[255] = '\0';
      cur->N_value2 = N_value2;
      memcpy(cur->V_value2, V_value2, (size_t)N_value2 * sizeof(double));
      cur->value3 = value3;
      pthread_mutex_unlock(&lock);
      return 0;
    }
    cur = cur->next;
  }
  pthread_mutex_unlock(&lock);
  return -1;
}

int delete_key(int key) {
  pthread_mutex_lock(&lock);
  node_t *cur = head, *prev = NULL;
  while (cur) {
    if (cur->key == key) {
      if (prev) {
        prev->next = cur->next;
      } else {
        head = cur->next;
      }
      free(cur);
      pthread_mutex_unlock(&lock);
      return 0;
    }
    prev = cur;
    cur = cur->next;
  }
  pthread_mutex_unlock(&lock);
  return -1;
}

int exist(int key) {
  pthread_mutex_lock(&lock);
  node_t *cur = head;
  while (cur) {
    if (cur->key == key) {
      pthread_mutex_unlock(&lock);
      return 1;
    }
    cur = cur->next;
  }
  pthread_mutex_unlock(&lock);
  return 0;
}
