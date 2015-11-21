#ifndef TCASM_LIST_H_
#define TCASM_LIST_H_

#include <stddef.h>

/**
 * Struct para armazenar um no de uma lista, contendo ponteiro para o valor,
 * para o no anterior e para o proximo no.
 */
typedef struct TCASM_list_node_s {
  void* value;
  struct TCASM_list_node_s* prev;
  struct TCASM_list_node_s* next;
} TCASM_list_node_t;

/**
 * Struct para armazenar as informacoes de uma lista, contendo o tamanho de
 * um elemento, ponteiro para o primeiro elemento, para o ultimo e uma
 * variavel para armazenar o tamanho.
 */
typedef struct {
  size_t value_size;
  TCASM_list_node_t* first;
  TCASM_list_node_t* last;
  size_t size;
} TCASM_list_t;

void TCASM_list_init(TCASM_list_t* list_ptr, size_t value_size);
void TCASM_list_insert(TCASM_list_t* list_ptr, TCASM_list_node_t* position, void* value);
void TCASM_list_erase(TCASM_list_t* list_ptr, TCASM_list_node_t* position);

#endif /* TCASM_LIST_H_ */
