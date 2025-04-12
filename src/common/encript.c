
#include "encript.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DELTA 0x9e3779b9
#define NUM_ROUNDS 64
#define BLOCK_SIZE 8

/*
 * La siguiente flag sirve para activar el modo debug. En este modo, no se encripta ni desencripta
 * el mensaje, simplemente se copia el mensaje de entrada al mensaje de salida. Esto servirá para
 * poder ver la diferencia en un analizador de tráfico de red (como Wireshark) entre un mensaje
 * encriptado y uno sin encriptar.
 */
const unsigned char DEBUG = 0;

void tea_encrypt(uint32_t *v, const uint32_t *k) {
  uint32_t v0 = v[0], v1 = v[1];
  uint32_t sum = 0;
  for (int i = 0; i < NUM_ROUNDS; i++) {
    v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    sum += DELTA;
    v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
  }
  v[0] = v0;
  v[1] = v1;
}

void tea_decrypt(uint32_t *v, const uint32_t *k) {
  uint32_t v0 = v[0], v1 = v[1];
  uint32_t sum = DELTA * NUM_ROUNDS;
  for (int i = 0; i < NUM_ROUNDS; i++) {
    v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
    sum -= DELTA;
    v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
  }
  v[0] = v0;
  v[1] = v1;
}

unsigned char* encrypt(const void *input, size_t size, size_t *output, const uint32_t *key) {
  if (DEBUG) {
    // En modo debug, simplemente copiamos el input al output
    unsigned char *output_data = (unsigned char *) malloc(size);
    if (!output_data) {
      perror("[ERROR] alocando memoria en modo debug");
      return NULL;
    }
    memcpy(output_data, input, size);
    *output = size;
    return output_data;
  }
  // Es necesario aplicar padding para que el tamaño sea múltiplo de BLOCK_SIZE
  size_t pad_len = BLOCK_SIZE - (size % BLOCK_SIZE);
  if (pad_len == 0) {
    pad_len = BLOCK_SIZE;
  }
  size_t total_size = size + pad_len;

  // Asignamos memoria para el output. Será responsabilidad del caller liberarla.
  unsigned char *output_data = (unsigned char *) malloc(total_size);
  if (!output_data) {
    perror("[ERROR] alocando memoria para el padding de encriptación");
    return NULL;
  }

  memcpy(output_data, input, size);
  memset(output_data + size, (unsigned char)pad_len, pad_len);

  size_t num_blocks = total_size / BLOCK_SIZE;
  uint32_t *data = (uint32_t *) output_data;
  for (size_t i = 0; i < num_blocks; i++) {
    tea_encrypt(&data[i * 2], key);
  }
  *output = total_size;
  return output_data;
}

unsigned char* decrypt(const void *input, size_t size, size_t *output, const uint32_t *key) {

  if (DEBUG) {
    // En modo debug, simplemente copiamos el input al output
    unsigned char *output_data = (unsigned char *) malloc(size);
    if (!output_data) {
      perror("[ERROR] alocando memoria en modo debug");
      return NULL;
    }
    memcpy(output_data, input, size);
    *output = size;
    return output_data;
  }

  if (size % BLOCK_SIZE != 0) {
    perror("[ERROR] tamaño de datos no es múltiplo de BLOCK_SIZE");
    return NULL;
  }

  unsigned char *output_data = (unsigned char *) malloc(size);
  if (!output_data) {
    perror("[ERROR] alocando memoria para el padding de desencriptación");
    return NULL;
  }

  memcpy(output_data, input, size);
  size_t num_blocks = size / BLOCK_SIZE;
  uint32_t *data = (uint32_t *) output_data;
  for (size_t i = 0; i < num_blocks; i++) {
    tea_decrypt(&data[i * 2], key);
  }

  // Leemos el padding (el último byte simboliza cuántos bytes de padding hay)
  unsigned char pad_len = output_data[size - 1];
  if (pad_len < 1 || pad_len > BLOCK_SIZE) {
    perror("[ERROR] tamaño de padding incorrecto");
    free(output_data);
    return NULL;
  }

  // Verificamos que el padding sea correcto
  for (size_t i = 1; i <= pad_len; i++) {
    if (output_data[size - i] != pad_len) {
      perror("[ERROR] padding incorrecto");
      free(output_data);
      return NULL;
    }
  }
  *output = size - pad_len;
  return output_data;
}
