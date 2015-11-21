#include "TCASM_symbol.h"

#include <stddef.h>

/**
 * Funcao para inicializar a tabela de simbolos com os simbolos pre-definidos.
 * @param table Ponteiro da tabela hash que sera utilizada.
 */
void TCASM_symbol_init(TCASM_hashtable_t* table) {
  TCASM_symbol_t* sym;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "SECTION", NULL);
  sym->type = TCASM_SYMBOL_DIRECTIVE_SECTION;

  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "SPACE", NULL);
  sym->type = TCASM_SYMBOL_DIRECTIVE_SPACE;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "CONST", NULL);
  sym->type = TCASM_SYMBOL_DIRECTIVE_CONST;

  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "TEXT", NULL);
  sym->type = TCASM_SYMBOL_DIRECTIVE_SECTION_TYPE;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "DATA", NULL);
  sym->type = TCASM_SYMBOL_DIRECTIVE_SECTION_TYPE;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "ADD", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_ADD;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "SUB", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_SUB;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "MULT", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_MULT;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "DIV", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_DIV;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "JMP", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_JMP;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "JMPN", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_JMPN;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "JMPP", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_JMPP;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "JMPZ", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_JMPZ;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "COPY", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_COPY;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_COPY;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "LOAD", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_LOAD;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "STORE", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_STORE;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "INPUT", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_INPUT;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "OUTPUT", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_REGULAR;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_OUTPUT;
  
  sym = (TCASM_symbol_t*) TCASM_hashtable_get(table, "STOP", NULL);
  sym->type = TCASM_SYMBOL_INSTRUCTION_STOP;
  sym->sym_union.instr.opcode = TCASM_SYMBOL_INSTRUCTION_OPCODE_STOP;
}
