#ifndef APTP_H
#define APTP_H

#include "claves.h"

typedef struct {
    int op;
    int key;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
} request_t;

typedef struct {
    int result;
    char value1[256];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
} response_t;

int parse_response_from_xml(const char *xml, response_t *resp);

int parse_request_from_xml(const char *xml, request_t *req);

char *format_response_to_xml(const response_t *resp);

char *format_request_to_xml(const request_t *req);

#endif //APTP_H
