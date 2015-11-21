#include "TCASM_list.h"

#include <stdlib.h>
#include <string.h>

/**
 * Funcao para inicializar uma lista.
 * @param list_ptr Ponteiro para a lista a ser inicializada.
 */
void TCASM_list_init(TCASM_list_t* list_ptr, size_t value_size) {
  list_ptr->value_size = value_size;
  list_ptr->first = NULL;
  list_ptr->last = NULL;
  list_ptr->size = 0;
}

/**
 * Funcao para inserir um elemento em uma lista.
 * @param list_ptr Ponteiro da lista.
 * @param position Ponteiro da posicao que o elemento ira ocupar.
 * O valor NULL insere no fim da lista.
 * @param value Valor que sera inserido.
 */
void TCASM_list_insert(TCASM_list_t* list_ptr, TCASM_list_node_t* position, void* value) {
  TCASM_list_node_t* tmp = (TCASM_list_node_t*) malloc(sizeof(TCASM_list_node_t));
  tmp->value = malloc(list_ptr->value_size);
  memcpy(tmp->value, value, list_ptr->value_size);
  tmp->next = position;
  if (position != NULL) {
    tmp->prev = position->prev;
    position->prev = tmp;
    if (tmp->prev != NULL)
      tmp->prev->next = tmp;
    else
      list_ptr->first = tmp;
  }
  else {
    tmp->prev = list_ptr->last;
    list_ptr->last = tmp;
    if (tmp->prev != NULL)
      tmp->prev->next = tmp;
    else
      list_ptr->first = tmp;
  }
  list_ptr->size++;
}

/**
 * Funcao para remover da lista o elemento apontado por position.
 * Se position for NULL, nao remove nada.
 * @param list_ptr Ponteiro da lista que sofrera a operacao.
 * @param position Ponteiro do elemento que sera apagado.
 */
void TCASM_list_erase(TCASM_list_t* list_ptr, TCASM_list_node_t* position) {
  if (position == NULL)
    return;
  if (position->prev != NULL)
    position->prev->next = position->next;
  else
    list_ptr->first = position->next;
  if (position->next != NULL)
    position->next->prev = position->prev;
  else
    list_ptr->last = position->prev;
  free(position);
  list_ptr->size--;
}
