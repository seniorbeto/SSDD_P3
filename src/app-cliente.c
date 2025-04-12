/*
 * Creado por Alberto Penas Díaz
 *
 * Práctica 1 de Sistemas Distribuidos (2024/2025)
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/claves.h"

#define MAX_LINE 1024

/**
 * @brief Función gestora del apagado del servidor.
 */
void handle_poweroff() {
  printf("\033[33mSaliendo del cliente... ¡Hasta Pronto!\033[0m\n");
  exit(EXIT_SUCCESS);
}

/**
 * @brief Transforma un string a un número. Basado en el primer
 * laboratorio de la asignatura.
 * @param str string a transformar.
 * @param num puntero a la variable donde se guardará el número.
 * @return 0 si todo fue bien, -1 si hubo un error (el string no es un entero).
 * @retval 0 si todo fue bien, -1 si hubo un error.
 * @retval -1 si el argumento no pudo ser transformado a un entero.
 */
int get_num(char *str, int *num) {
  char *endptr;
  *num = (int) strtol(str, &endptr, 10);
  if (*endptr != '\0') {
    return -1;
  }
  return 0;
}

int main() {
  /*
   * El cliente se comunica con el servidor a través de una minishell.
   * Mmmmhhhh... me recuerda a cierta práctica de Sistemas Operativos....
   *
   * (https://github.com/seniorbeto/SO_practica2/tree/main/2024)
   */
  char line[MAX_LINE];

  printf("\033[33m"); // Esto es para dar color a la terminal
  printf("\n¡Bienvenido a la shell del cliente!\n");
  printf("Creado por Alberto Penas Díaz\n\n");
  printf("Uso:\n");
  printf("  set <key> <value1> <N> <v2[0]> ... <v2[N-1]> <x> <y>\n");
  printf("  get <key>\n");
  printf("  modify <key> <value1> <N> <v2[0]> ... <v2[N-1]> <x> <y>\n");
  printf("  delete <key>\n");
  printf("  exist <key>\n");
  printf("  destroy\n");
  printf("  exit\n");
  printf("\033[0m");

  // FIXME No entiendo por qué no funciona esto.
  signal(SIGINT, handle_poweroff);
  signal(SIGTERM, handle_poweroff);

  while (1) {
    printf("\033[32m>> \033[0m");
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }

    line[strcspn(line, "\n")] = '\0';

    if (strcmp(line, "exit") == 0) {
      break;
    }

    char *cmd = strtok(line, " "); // (esto es literalmente lo mismo al .split() de python)
    if (!cmd) {
      continue;
    }

    if (strcmp(cmd, "set") == 0) {
      char *key_str = strtok(NULL, " ");
      char *value1 = strtok(NULL, " ");
      if (strlen(value1) > 255) {
        printf("El valor 1 debe tener como máximo 255 caracteres\n");
        continue;
      }
      char *N_str = strtok(NULL, " ");
      if (!key_str || !value1 || !N_str) {
        printf("Uso: set <key> <value1> <N> <v2[0]> ... <v2[N-1]> <x> <y>\n");
        continue;
      }
      int key;
      if (get_num(key_str, &key) == -1) {
        printf("La clave debe ser un número entero\n");
        continue;
      }
      int N = atoi(N_str);
      if (N < 1 || N > 32) {
        printf("N debe estar entre 1 y 32\n");
        continue;
      }
      double v2[32];
      int i;
      for (i = 0; i < N; i++) {
        char *num = strtok(NULL, " ");
        if (!num) {
          break;
        }
        v2[i] = atof(num);
      }
      if (i != N) {
        printf("Número insuficiente de valores para v2\n");
        continue;
      }
      char *x_str = strtok(NULL, " ");
      char *y_str = strtok(NULL, " ");
      if (!x_str || !y_str) {
        printf("Uso: set <key> <value1> <N> <v2[0]> ... <v2[N-1]> <x> <y>\n");
        continue;
      }
      struct Coord v3;
      int x, y;
      if (get_num(x_str, &x) == -1 || get_num(y_str, &y) == -1) {
        printf("Las coordenadas deben ser números enteros\n");
        continue;
      }
      v3.x = x;
      v3.y = y;

      int err = set_value(key, value1, N, v2, v3);
      if (err == -1) {
        printf("Error al insertar la tupla\n");
      } else if (err == -2) {
        printf("Error en la comunicación con el servidor. (¿Está encendido?)\n");
      } else {
        printf("Tupla insertada correctamente\n");
      }

    } else if (strcmp(cmd, "get") == 0) {
      // Formato: get <key>
      char *key_str = strtok(NULL, " ");
      if (!key_str) {
        printf("Uso: get <key>\n");
        continue;
      }
      int key;
      if (get_num(key_str, &key) == -1) {
        printf("La clave debe ser un número entero\n");
        continue;
      }
      char buffer[256];
      int N;
      double vec[32];
      struct Coord coord;
      int err = get_value(key, buffer, &N, vec, &coord);
      if (err == 0) {
        printf("Obtenido: key=%d, value1=%s, N=%d, value3=(%d, %d)\n", key, buffer, N, coord.x, coord.y);
        printf("Vector: ");
        for (int i = 0; i < N; i++) {
          printf("%f ", vec[i]);
        }
        printf("\n");
      } else {
        printf("Error al obtener la tupla\n");
      }

    } else if (strcmp(cmd, "modify") == 0) {
      // Formato: modify <key> <value1> <N> <v2...> <x> <y>
      char *key_str = strtok(NULL, " ");
      char *value1 = strtok(NULL, " ");
      if (strlen(value1) > 255) {
        printf("El valor 1 debe tener como máximo 255 caracteres\n");
        continue;
      }
      char *N_str = strtok(NULL, " ");
      if (!key_str || !value1 || !N_str) {
        printf("Uso: modify <key> <value1> <N> <v2[0]> ... <v2[N-1]> <x> <y>\n");
        continue;
      }
      int key;
      if (get_num(key_str, &key) == -1) {
        printf("La clave debe ser un número entero\n");
        continue;
      }
      int N = atoi(N_str);
      if (N < 1 || N > 32) {
        printf("N debe estar entre 1 y 32\n");
        continue;
      }
      double v2[32];
      int i;
      for (i = 0; i < N; i++) {
        char *num = strtok(NULL, " ");
        if (!num) {
          break;
        }
        v2[i] = atof(num);
      }
      if (i != N) {
        printf("Número insuficiente de valores para v2\n");
        continue;
      }
      char *x_str = strtok(NULL, " ");
      char *y_str = strtok(NULL, " ");
      if (!x_str || !y_str) {
        printf("Uso: modify <key> <value1> <N> <v2[0]> ... <v2[N-1]> <x> <y>\n");
        continue;
      }
      struct Coord v3;
      int x, y;
      if (get_num(x_str, &x) == -1 || get_num(y_str, &y) == -1) {
        printf("Las coordenadas deben ser números enteros\n");
        continue;
      }
      v3.x = x;
      v3.y = y;

      int err = modify_value(key, value1, N, v2, v3);
      if (err == -1) {
        printf("Error al modificar la tupla\n");
      } else if (err == -2) {
        printf("Error en la comunicación con el servidor. (Está encendido?)\n");
      } else {
        printf("Tupla modificada correctamente\n");
      }

    } else if (strcmp(cmd, "delete") == 0) {
      // Formato: delete <key>
      char *key_str = strtok(NULL, " ");
      if (!key_str) {
        printf("Uso: delete <key>\n");
        continue;
      }
      int key;
      if (get_num(key_str, &key) == -1) {
        printf("La clave debe ser un número entero\n");
        continue;
      }
      int err = delete_key(key);
      if (err == -2) {
        printf("Error en la comunicación con el servidor. (Está encendido?)\n");
      } else if (err == 0) {
        printf("Tupla eliminada correctamente\n");
      } else {
        printf("Error al eliminar la tupla\n");
      }

    } else if (strcmp(cmd, "exist") == 0) {
      // Formato: exist <key>
      char *key_str = strtok(NULL, " ");
      if (!key_str) {
        printf("Uso: exist <key>\n");
        continue;
      }
      int key;
      if (get_num(key_str, &key) == -1) {
        printf("La clave debe ser un número entero\n");
        continue;
      }
      int err = exist(key);
      if (err == -2) {
        printf("Error en la comunicación con el servidor. (Está encendido?)\n");
      } else if (err == 1) {
        printf("La clave %d existe\n", key);
      } else {
        printf("La clave %d no existe\n", key);
      }

    } else if (strcmp(cmd, "destroy") == 0) {
      int err = destroy();
      if (err == -2) {
        printf("Error en la comunicación con el servidor. (Está encendido?)\n");
      } else if (err == 0) {
        printf("Servidor destruido\n");
      } else {
        printf("Error al destruir el servidor\n");
      }

    } else {
      printf("Comando desconocido\n");
    }
  }

  handle_poweroff();
  return 0;
}
