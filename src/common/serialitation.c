#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serialitation.h"

/*
 * xml_get_content: busca en 'xml' la primera aparición de <tag>...</tag>
 * y devuelve un puntero a una cadena con el contenido de esa sección.
 * El caller debe liberar la cadena devuelta con free.
 *
 * Devuelve NULL si no se encuentra la sección o en caso de error.
 */
char *xml_get_content(const char *xml, const char *tag) {
  // Construimos las etiquetas de apertura y cierre
  size_t tag_len = strlen(tag);
  char *open_tag = malloc(tag_len + 3); // "<" + tag + ">"
  char *close_tag = malloc(tag_len + 4); // "</" + tag + ">"
  if (!open_tag || !close_tag) {
    free(open_tag);
    free(close_tag);
    return NULL;
  }
  sprintf(open_tag, "<%s>", tag);
  sprintf(close_tag, "</%s>", tag);

  const char *start = strstr(xml, open_tag);
  if (!start) {
    free(open_tag);
    free(close_tag);
    return NULL;
  }
  start += strlen(open_tag); // Avanzamos tras "<tag>"

  const char *end = strstr(start, close_tag);
  if (!end) {
    free(open_tag);
    free(close_tag);
    return NULL;
  }

  // Extraemos el contenido
  size_t content_len = (size_t) (end - start);
  char *content = malloc(content_len + 1);
  if (!content) {
    free(open_tag);
    free(close_tag);
    return NULL;
  }
  strncpy(content, start, content_len);
  content[content_len] = '\0';

  free(open_tag);
  free(close_tag);
  return content;
}

int parse_request_from_xml(const char *xml, request_t *req) {
  // Inicializamos la estructura. Esto es necesario para que los campos no especificados
  // en el XML no contengan basura.
  req->op = 0;
  req->key = 0;
  req->value1[0] = '\0';
  req->N_value2 = 0;
  req->value3.x = 0.0;
  req->value3.y = 0.0;

  // 1) op
  char *op_str = xml_get_content(xml, "op");
  if (op_str) {
    req->op = atoi(op_str);
    free(op_str);
  }

  // 2) key
  char *key_str = xml_get_content(xml, "key");
  if (key_str) {
    req->key = atoi(key_str);
    free(key_str);
  }

  // 3) value1
  char *val1_str = xml_get_content(xml, "value1");
  if (val1_str) {
    strncpy(req->value1, val1_str, sizeof(req->value1) - 1);
    req->value1[sizeof(req->value1) - 1] = '\0';
    free(val1_str);
  }

  // 4) N_value2
  char *nval_str = xml_get_content(xml, "N_value2");
  if (nval_str) {
    req->N_value2 = atoi(nval_str);
    free(nval_str);
  }

  // 5) V_value2
  char *vval_str = xml_get_content(xml, "V_value2");
  if (vval_str) {
    if (req->N_value2 > 0) {
      int count = 0;
      char *token = strtok(vval_str, ",");
      while (token && count < req->N_value2 && count < 32) {
        req->V_value2[count++] = atof(token);
        token = strtok(NULL, ",");
      }
      // Ajustamos N_value2 en caso de que se hayan leído menos valores que el indicado
      // aunque nunca debería pasar.
      req->N_value2 = count;
    }
    free(vval_str);
  }

  // 6) coord -> x
  char *x_str = xml_get_content(xml, "x");
  if (x_str) {
    req->value3.x = atoi(x_str);
    free(x_str);
  }

  // 7) coord -> y
  char *y_str = xml_get_content(xml, "y");
  if (y_str) {
    req->value3.y = atoi(y_str);
    free(y_str);
  }

  return 0; // Éxito
}

int parse_response_from_xml(const char *xml, response_t *resp) {
  resp->result = 0;
  resp->value1[0] = '\0';
  resp->N_value2 = 0;
  resp->value3.x = 0.0;
  resp->value3.y = 0.0;

  char *result_str = xml_get_content(xml, "result");
  if (result_str) {
    resp->result = atoi(result_str);
    free(result_str);
  }

  char *val1_str = xml_get_content(xml, "value1");
  if (val1_str) {
    strncpy(resp->value1, val1_str, sizeof(resp->value1) - 1);
    resp->value1[sizeof(resp->value1) - 1] = '\0';
    free(val1_str);
  }

  char *nval_str = xml_get_content(xml, "N_value2");
  if (nval_str) {
    resp->N_value2 = atoi(nval_str);
    free(nval_str);
  }

  char *vval_str = xml_get_content(xml, "V_value2");
  if (vval_str) {
    if (resp->N_value2 > 0) {
      int count = 0;
      char *token = strtok(vval_str, ",");
      while (token && count < resp->N_value2 && count < 32) {
        resp->V_value2[count++] = atof(token);
        token = strtok(NULL, ",");
      }
      resp->N_value2 = count;
    }
    free(vval_str);
  }

  char *x_str = xml_get_content(xml, "x");
  if (x_str) {
    resp->value3.x = atoi(x_str);
    free(x_str);
  }

  char *y_str = xml_get_content(xml, "y");
  if (y_str) {
    resp->value3.y = atoi(y_str);
    free(y_str);
  }

  return 0;
}

char *format_request_to_xml(const request_t *req) {
  char vbuf[1024]; // Buffer local para la lista separada por comas
  vbuf[0] = '\0';

  if (req->N_value2 > 0) {
    for (int i = 0; i < req->N_value2; i++) {
      char temp[64];
      snprintf(temp, sizeof(temp), "%lf", req->V_value2[i]);
      if (i > 0) {
        strncat(vbuf, ",", sizeof(vbuf) - strlen(vbuf) - 1);
      }
      strncat(vbuf, temp, sizeof(vbuf) - strlen(vbuf) - 1);
    }
  }

  char *xml = malloc(2048);
  if (!xml) {
    return NULL;
  }
  sprintf(xml,
          "<request>"
          "<op>%d</op>"
          "<key>%d</key>"
          "<value1>%s</value1>"
          "<N_value2>%d</N_value2>"
          "<V_value2>%s</V_value2>"
          "<coord>"
          "<x>%d</x>"
          "<y>%d</y>"
          "</coord>"
          "</request>",
          req->op, req->key, req->value1, req->N_value2, vbuf, req->value3.x, req->value3.y);

  return xml;
}

char *format_response_to_xml(const response_t *resp) {
  char vbuf[1024]; // Buffer local para la lista separada por comas
  vbuf[0] = '\0';

  if (resp->N_value2 > 0) {
    for (int i = 0; i < resp->N_value2; i++) {
      char temp[64];
      snprintf(temp, sizeof(temp), "%lf", resp->V_value2[i]);
      if (i > 0) {
        strncat(vbuf, ",", sizeof(vbuf) - strlen(vbuf) - 1);
      }
      strncat(vbuf, temp, sizeof(vbuf) - strlen(vbuf) - 1);
    }
  }

  // USANDO MALLOC HACE QUE SEA RESPONSAIBILIDAD DEL CALLER LIBERAR LA MEMORIA
  char *xml = malloc(2048);
  if (!xml) {
    return NULL;
  }
  sprintf(xml,
          "<response>"
          "<result>%d</result>"
          "<value1>%s</value1>"
          "<N_value2>%d</N_value2>"
          "<V_value2>%s</V_value2>"
          "<coord>"
          "<x>%d</x>"
          "<y>%d</y>"
          "</coord>"
          "</response>",
          resp->result, resp->value1, resp->N_value2, vbuf, resp->value3.x, resp->value3.y);

  return xml;
}
