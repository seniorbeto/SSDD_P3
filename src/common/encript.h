/*
 * Implementación del algoritmo XTEA (Extended Tiny Encryption Algorithm).
 *
 * REFERENCIAS
 * ===========
 * https://en.wikipedia.org/wiki/XTEA
 */

#ifndef ENCRIPT_H
#define ENCRIPT_H

#include <stdint.h>
#include <stdlib.h>

void tea_encrypt(uint32_t *v, const uint32_t *k);
void tea_decrypt(uint32_t *v, const uint32_t *k);

/**
 * @brief Esta función encripta el mensaje de entrada con la clave key.
 * Este mensaje puede ser de cualquier tamaño y de cualquier tipo, pero se encriptará en bloques de 8 bytes.
 *
 * @param input Mensaje de entrada.
 * @param size Tamaño del mensaje de entrada.
 * @param output Variable donde se devuelve el tamaño total de datos encriptados
 * @param key Clave de encriptación.
 */
unsigned char* encrypt(const void *input, size_t size, size_t *output, const uint32_t *key);

/**
 * @brief Esta función desencripta el mensaje de entrada con la clave key.
 * Este mensaje puede ser de cualquier tamaño y de cualquier tipo, pero se desencriptará en bloques de 8 bytes.
 *
 * @param input Mensaje de entrada.
 * @param size Tamaño del mensaje de entrada.
 * @param output Variable donde se devuelve el tamaño total de datos desencriptados
 * @param key Clave de encriptación.
 */
unsigned char* decrypt(const void *input, size_t size, size_t *output, const uint32_t *key);

#endif //ENCRIPT_H
