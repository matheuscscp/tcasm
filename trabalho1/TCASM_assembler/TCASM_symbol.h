#ifndef TCASM_SYMBOL_H_
#define TCASM_SYMBOL_H_

#include <stdint.h>

#include "TCASM_hashtable.h"
#include "TCASM_list.h"

/**
 * Define o tamanho maximo de um identificador.
 */
#define TCASM_SYMBOL_MAX_IDENTIFIER_SIZE 1000

/**
 * Possiveis valores para o tipo de um simbolo.
 */
typedef enum {
  TCASM_SYMBOL_DIRECTIVE_SECTION = 0,
  TCASM_SYMBOL_DIRECTIVE_SECTION_TYPE,
  TCASM_SYMBOL_DIRECTIVE_SPACE,
  TCASM_SYMBOL_DIRECTIVE_CONST,
  TCASM_SYMBOL_INSTRUCTION_REGULAR,
  TCASM_SYMBOL_INSTRUCTION_COPY,
  TCASM_SYMBOL_INSTRUCTION_STOP,
  TCASM_SYMBOL_ADDRESS_TEXT,
  TCASM_SYMBOL_ADDRESS_VAR,
  TCASM_SYMBOL_ADDRESS_CONST,
  TCASM_SYMBOL_ADDRESS_ARRAY
} TCASM_symbol_type_t;

/**
 * Valores de opcodes de cada instrucao.
 */
enum {
  TCASM_SYMBOL_INSTRUCTION_OPCODE_ADD = 1,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_SUB,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_MULT,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_DIV,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_JMP,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_JMPN,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_JMPP,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_JMPZ,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_COPY,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_LOAD,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_STORE,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_INPUT,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_OUTPUT,
  TCASM_SYMBOL_INSTRUCTION_OPCODE_STOP
};

/**
 * Struct para armazenar as informacoes de um tipo de instrucao.
 */
typedef struct {
  /// Contem o opcode do tipo de instrucao.
  uint16_t opcode;
} TCASM_symbol_instruction_t;

/**
 * Struct para armazenar as informacoes de um endereco de instrucao.
 */
typedef struct {
  /// Armazenara o endereco do rotulo a partir do momento em que
  /// ele for definido.
  uint16_t addr;
  
  /// Lista de referencias ao endereco.
  TCASM_list_t ref_list;
} TCASM_symbol_address_text_t;

/**
 * Struct para armazenar as informacoes de um endereco de uma variavel.
 * Utilizado apenas quando a secao de dados vem ANTES da secao de texto no
 * codigo fonte.
 */
typedef struct {
  /// Lista de referencias ao endereco.
  TCASM_list_t ref_list;
} TCASM_symbol_address_var_t;

/**
 * Struct para armazenar as informacoes de um endereco de uma constante.
 * Utilizado apenas quando a secao de dados vem ANTES da secao de texto no
 * codigo fonte.
 */
typedef struct {
  /// Valor da constante em complemento de 2. (Nao eh pq o TIPO nao tem sinal
  /// que os bits nao estao la bonitinhos.)
  uint16_t value;
  
  /// Lista de referencias ao endereco.
  TCASM_list_t ref_list;
} TCASM_symbol_address_const_t;

/**
 * Struct para armazenar as informacoes de uma referencia a um endereco de
 * instrucao, variavel ou constante.
 */
typedef struct {
  /// Linha da referencia no codigo-fonte.
  unsigned int line;
  
  /// Endereco do operando que precisa ser atualizado no codigo montado.
  uint16_t addr;
} TCASM_symbol_address_reflist_t;

/**
 * Struct para armazenar as informacoes de um endereco de uma variavel ou
 * de uma constante.
 * Utilizado apenas quando a secao de dados vem DEPOIS da secao de texto no
 * codigo fonte.
 */
typedef struct {
  /// Lista de referencias ao endereco.
  TCASM_list_t ref_list;
} TCASM_symbol_address_varconst_t;

/**
 * Struct para armazenar os dados de uma referencia a uma variavel ou constante.
 * Utilizado apenas quando a secao de dados vem DEPOIS da secao de texto no
 * codigo fonte.
 */
typedef struct {
  /// Indica o endereco no codigo montado da instrucao que fez a referencia.
  uint16_t instr_addr;
  
  /// Salva a linha da instrucao que referencia o vetor.
  unsigned int line;
  
  /// Indica o endereco no codigo montado do operando a ser atualizado.
  uint16_t op_addr;
} TCASM_symbol_address_varconst_reflist_t;

/**
 * Struct para armazenar as informacoes de um endereco de vetor.
 */
typedef struct {
  /// Tamanho do vetor.
  uint16_t size;
  
  /// Lista de referencias ao endereco.
  TCASM_list_t ref_list;
} TCASM_symbol_address_array_t;

/**
 * Struct para armazenar os dados de uma referencia a um vetor.
 */
typedef struct {
  /// Offset de posicao no vetor utilizado na referencia.
  uint16_t offset;
  
  /// Salva a linha da instrucao que referencia o vetor.
  unsigned int line;
  
  /// Indica o endereco no codigo montado do operando a ser atualizado.
  uint16_t addr;
} TCASM_symbol_address_array_reflist_t;

/**
 * Union para armazenar as informacoes dos diferentes tipos de simbolos.
 */
typedef union {
  TCASM_symbol_instruction_t instr;
  TCASM_symbol_address_text_t text;
  TCASM_symbol_address_var_t var;
  TCASM_symbol_address_const_t constant;
  TCASM_symbol_address_varconst_t varconst;
  TCASM_symbol_address_array_t array;
} TCASM_symbol_union_t;

/**
 * Struct para armazenar as informacoes de um simbolo.
 * Aqui, um simbolo pode ser um identificador ou uma palavra-chave.
 */
typedef struct {
  /// Tipo de acordo com a enumeracao.
  TCASM_symbol_type_t type;
  
  ///< Union de armazenamento de informacoes para diferentes tipos de simbolos.
  TCASM_symbol_union_t sym_union;
} TCASM_symbol_t;

void TCASM_symbol_init(TCASM_hashtable_t* table);

#endif /* TCASM_SYMBOL_H_ */
