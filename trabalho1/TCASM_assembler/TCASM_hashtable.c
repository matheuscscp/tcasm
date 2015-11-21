#include "TCASM_hashtable.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t TCASM_hash(size_t array_size, const char* key, size_t* key_size);

/**
 * Funcao para inicializar uma tabela hash.
 * @param hashtable_ptr Ponteiro da tabela.
 * @param value_size Tamanho dos elementos armazenados pela tabela.
 * @param array_size Tamanho do vetor da tabela.
 */
void TCASM_hashtable_init(TCASM_hashtable_t* hashtable_ptr, size_t value_size, size_t array_size) {
  hashtable_ptr->value_size = value_size;
  hashtable_ptr->array_size = array_size ? array_size : TCASM_HASHTABLE_DEFAULT_SIZE;
  
  // cria um vetor de listas nao inicializadas (0 no campo value_size)
  hashtable_ptr->array = (TCASM_list_t*) calloc(hashtable_ptr->array_size, sizeof(TCASM_list_t));
}

/**
 * Funcao para obter um elemento cuja chave eh passada como argumento. Se o
 * elemento nao existe, a funcao cria e retorna o ponteiro.
 * @param hashtable_ptr Ponteiro da tabela.
 * @param key String chave do elemento.
 * @param created Ponteiro para a funcao retornar true se o elemento foi
 * criado, ou false em caso contrario. Se o ponteiro for NULL, a funcao ignora.
 * @return Retorna o ponteiro do elemento procurado. Precisa ser castado para
 * o tipo correto.
 */
void* TCASM_hashtable_get(TCASM_hashtable_t* hashtable_ptr, const char* key, bool* created) {
  if (created != NULL)
    *created = false;
  size_t key_size;
  TCASM_list_t* elem_list = &hashtable_ptr->array[TCASM_hash(hashtable_ptr->array_size, key, &key_size)];
  
  // se a lista nao esta inicializada, ela esta vazia e portanto eh necessario
  // criar a entrada
  if (elem_list->value_size == 0) {
    if (created != NULL)
      *created = true;
    TCASM_hashtable_node_t hashtable_node;
    hashtable_node.key = (char*) malloc(sizeof(char)*(key_size + 1));
    hashtable_node.value = calloc(1, hashtable_ptr->value_size);
    strcpy(hashtable_node.key, key);
    elem_list->value_size = sizeof(TCASM_hashtable_node_t);
    TCASM_list_insert(elem_list, NULL, &hashtable_node);
    return ((TCASM_hashtable_node_t*) elem_list->first->value)->value;
  }
  
  // procura a entrada
  TCASM_list_node_t* elem_list_node = elem_list->first;
  while (elem_list_node != NULL) {
    int cmp = strcmp(((TCASM_hashtable_node_t*) elem_list_node->value)->key, key);
    // node->key == key
    if (cmp == 0)
      return ((TCASM_hashtable_node_t*) elem_list_node->value)->value;
    // node->key < key
    else if (cmp < 0)
      elem_list_node = elem_list_node->next;
    // node->key > key
    else
      break;
  }
  
  // se nao encontrou, cria a entrada
  if (created != NULL)
    *created = true;
  TCASM_hashtable_node_t hashtable_node;
  hashtable_node.key = (char*) malloc(sizeof(char)*(key_size + 1));
  hashtable_node.value = calloc(1, hashtable_ptr->value_size);
  strcpy(hashtable_node.key, key);
  TCASM_list_insert(elem_list, elem_list_node, &hashtable_node);
  if (elem_list_node == NULL)
    return ((TCASM_hashtable_node_t*) elem_list->last->value)->value;
  return ((TCASM_hashtable_node_t*) elem_list_node->prev->value)->value;
}

/**
 * Funcao para obter um elemento cuja chave eh passada como argumento. Se o
 * elemento nao existe, a funcao cria e retorna o ponteiro.
 * @param hashtable_ptr Ponteiro da tabela.
 * @param key String chave do elemento.
 * @param created Ponteiro para a funcao retornar true se o elemento foi
 * criado, ou false em caso contrario. Se o ponteiro for NULL, a funcao ignora.
 * @return Retorna o ponteiro do no de lista que a tabela utiliza para
 * armazenar o elemento.
 */
TCASM_hashtable_node_t* TCASM_hashtable_get_node(TCASM_hashtable_t* hashtable_ptr, const char* key, bool* created) {
  if (created != NULL)
    *created = false;
  size_t key_size;
  TCASM_list_t* elem_list = &hashtable_ptr->array[TCASM_hash(hashtable_ptr->array_size, key, &key_size)];
  
  // se a lista nao esta inicializada, ela esta vazia e portanto eh necessario
  // criar a entrada
  if (elem_list->value_size == 0) {
    if (created != NULL)
      *created = true;
    TCASM_hashtable_node_t hashtable_node;
    hashtable_node.key = (char*) malloc(sizeof(char)*(key_size + 1));
    hashtable_node.value = calloc(1, hashtable_ptr->value_size);
    strcpy(hashtable_node.key, key);
    elem_list->value_size = sizeof(TCASM_hashtable_node_t);
    TCASM_list_insert(elem_list, NULL, &hashtable_node);
    return elem_list->first->value;
  }
  
  // procura a entrada
  TCASM_list_node_t* elem_list_node = elem_list->first;
  while (elem_list_node != NULL) {
    int cmp = strcmp(((TCASM_hashtable_node_t*) elem_list_node->value)->key, key);
    // node->key == key
    if (cmp == 0)
      return elem_list_node->value;
    // node->key < key
    else if (cmp < 0)
      elem_list_node = elem_list_node->next;
    // node->key > key
    else
      break;
  }
  
  // se nao encontrou, cria a entrada
  if (created != NULL)
    *created = true;
  TCASM_hashtable_node_t hashtable_node;
  hashtable_node.key = (char*) malloc(sizeof(char)*(key_size + 1));
  hashtable_node.value = calloc(1, hashtable_ptr->value_size);
  strcpy(hashtable_node.key, key);
  TCASM_list_insert(elem_list, elem_list_node, &hashtable_node);
  if (elem_list_node == NULL)
    return elem_list->last->value;
  return elem_list_node->prev->value;
}

/**
 * Funcao que calcula o hash de uma chave.
 * @param array_size Tamanho do vetor da tabela.
 * @param key String chave do elemento.
 * @param key_size Ponteiro para retornar o tamanho da chave (gambiarra).
 * @return Retorna um indice no vetor de uma tabela hash.
 */
size_t TCASM_hash(size_t array_size, const char* key, size_t* key_size) {
  unsigned h = 0, g, i;
  for (i = 0; key[i] != '\0'; ++i) {
    h = (h << 4) + key[i];
    g = h & 0xF0000000;
    if (g != 0) {
       h = h ^ (g >> 24);
       h = h ^ g;
    }
  }
  *key_size = i;
  return (h*(uint64_t) 999983)%array_size;
}
