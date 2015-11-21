#ifndef TCASM_HASHTABLE_H_
#define TCASM_HASHTABLE_H_

#include <stdbool.h>
#include <stddef.h>

#include "TCASM_list.h"

/**
 * Tamanho padrao do vetor da tabela hash.
 */
#define TCASM_HASHTABLE_DEFAULT_SIZE 1201

/**
 * Struct para armazenar uma tabela hash.
 */
typedef struct {
  /// Tamanho dos elementos armazenados pela tabela.
  size_t value_size;
  
  /// Tamanho do vetor da tabela.
  size_t array_size;
  
  /// Vetor da tabela.
  TCASM_list_t* array;
} TCASM_hashtable_t;

/**
 * Struct de no de lista para armazenar os dados da tabela hash.
 */
typedef struct {
  /// String chave do elemento.
  char* key;
  
  /// Valor do elemento.
  void* value;
} TCASM_hashtable_node_t;

void TCASM_hashtable_init(TCASM_hashtable_t* hashtable_ptr, size_t value_size, size_t array_size);
void* TCASM_hashtable_get(TCASM_hashtable_t* hashtable_ptr, const char* key, bool* created);
TCASM_hashtable_node_t* TCASM_hashtable_get_node(TCASM_hashtable_t* hashtable_ptr, const char* key, bool* created);

#endif /* TCASM_HASHTABLE_H_ */
