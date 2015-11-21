#include "TCASM_assembler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "TCASM_hashtable.h"
#include "TCASM_list.h"
#include "TCASM_symbol.h"

// =============================================================================
// enumeracoes privadas
// =============================================================================

/**
 * Enumeracao dos estados da maquina de montagem.
 */
typedef enum {
  TCASM_STATE_SECTION = 0,
  TCASM_STATE_SECTION_TYPE,
  TCASM_STATE_TEXT_STATEMENT,
  TCASM_STATE_DATA_STATEMENT_DATABEFORE,
  TCASM_STATE_DATA_STATEMENT_DATAAFTER,
  TCASM_STATE_TEXT,
  TCASM_STATE_DATA_CREATE_DATABEFORE,
  TCASM_STATE_DATA_CREATE_DATAAFTER,
  TCASM_STATE_DATA_DEFINE_VARCONST,
  TCASM_STATE_DATA_DEFINE_ARRAY,
  TCASM_STATE_REGULAR,
  TCASM_STATE_BRANCH,
  TCASM_STATE_COPY
} TCASM_state_t;

// =============================================================================
// tipos privados
// =============================================================================

/**
 * Struct-valor da lista de referencias pendentes.
 */
typedef struct {
  /// Ponteiro para um simbolo que possui referencias pendentes.
  TCASM_hashtable_node_t* sym;
} TCASM_reflist_node_t;

/**
 * Struct-valor da lista da dados. Usado apenas quando a secao de dados vem
 * antes da secao de codigo.
 */
typedef struct {
  /// Indica se eh um dado anonimo ou um dado normal.
  bool anonymous;
  
  /// Ponteiro para um simbolo que armazena informacoes de um dado.
  TCASM_symbol_t* sym;
} TCASM_datalist_node_t;

// =============================================================================
// declaracao de variaveis globais privadas
// =============================================================================

/**
 * Arquivo de entrada com assembly TCASM.
 */
static FILE* TCASM_fin;

/**
 * Tabela de simbolos (identificadores ou palavras-chave).
 */
static TCASM_hashtable_t TCASM_symbol_table;

/**
 * Buffer com o tamanho exato da memoria da maquina hipotetica para armazenar
 * o codigo gerado.
 */
static uint16_t TCASM_assembled_code[65536];

/**
 * Contador de bytes utilizados na montagem.
 */
static size_t TCASM_assembled_code_size = 0;

/**
 * Contador de bytes de offset de dados, quando a secao de dados vem antes
 * da secao de texto.
 */
static size_t TCASM_data_size = 0;

/**
 * Flag para dizer se a secao de codigo ja foi lida.
 */
static bool TCASM_text_read = false;

/**
 * Flag para dizer se a secao de dados ja foi lida.
 */
static bool TCASM_data_read = false;

/**
 * Variavel para armazenar o estado atual da maquina de montagem.
 */
static TCASM_state_t TCASM_state = TCASM_STATE_SECTION;

/**
 * Linha sendo lida atualmente no arquivo fonte.
 */
static unsigned int TCASM_line = 1;

/**
 * Linha em que a ultima sentenca comecou.
 */
static unsigned int TCASM_statement_line;

/**
 * Caractere que foi lido por ultimo e esta sendo tratado.
 */
static int TCASM_char;

/**
 * Quantidade de caracteres retirados do arquivo na ultima leitura.
 */
static size_t TCASM_read_chars;

/**
 * Quantidade de linhas retirados do arquivo na ultima leitura.
 */
static unsigned int TCASM_read_lines;

/**
 * Flag para indicar se houve uma mudanca de linha. Utilizar
 * check_new_line() para consultar esta flag.
 * @see TCASM_check_new_line
 */
static bool TCASM_changed_line = false;

/**
 * Palavra sendo lida atualmente.
 */
static char TCASM_word[TCASM_SYMBOL_MAX_IDENTIFIER_SIZE + 1];

/**
 * Tamanho da palavra sendo lida atualmente.
 */
static size_t TCASM_symbol_size;

/**
 * Ponteiro para no da lista de hash do simbolo que esta sendo definido.
 */
static TCASM_hashtable_node_t* TCASM_hashnode;

/**
 * Simbolo que esta sendo definido.
 */
static TCASM_symbol_t* TCASM_symbol;

/**
 * Lista de referencias pendentes.
 */
static TCASM_list_t TCASM_reflist;

/**
 * Lista de dados declarados. Usado apenas quando a secao de dados vem antes
 * da secao de texto.
 */
static TCASM_list_t TCASM_datalist;

/**
 * Opcode da instrucao que esta sendo montada.
 */
static uint16_t TCASM_opcode;

/**
 * Flag para indicar se esta lendo o segundo operando de um copy.
 */
static bool TCASM_second_op = false;

// =============================================================================
// declaracao de funcoes privadas
// =============================================================================

// util
static void TCASM_write_uint16_zeroarray(uint16_t dst[], size_t pos, size_t count);
static void TCASM_strinit(char* to, size_t* size, int from);
static void TCASM_strinitempty(char* to, size_t* size);
static void TCASM_strcat(char* to, size_t* size, int from);

static void TCASM_read_file(const char* in);
static void TCASM_write_file(const char* out);
static int TCASM_ignore_line();
static bool TCASM_read_char();
static bool TCASM_read_char_from_file();
static void TCASM_read_symbol();
static void TCASM_read_colon();
static void TCASM_read_comma();
static int TCASM_read_array_ref();
static TCASM_symbol_type_t TCASM_read_data_type();
static void TCASM_check_new_line();
static void TCASM_check_same_line();
static void TCASM_check_new_column();
static void TCASM_dump_text_reflist();
static void TCASM_dump_varconst_reflist_databefore(TCASM_list_t* ref_list);
static void TCASM_dump_var_reflist_dataafter();
static void TCASM_dump_const_reflist_dataafter(int const_value);
static void TCASM_dump_array_reflist_databefore(TCASM_list_t* ref_list);
static void TCASM_dump_array_reflist_dataafter();
static void TCASM_dump_datalist();
static void TCASM_dump_reflist_databefore();
static void TCASM_dump_reflist_dataafter();
static void TCASM_create_anonymous_data_databefore();
static void TCASM_create_anonymous_data_dataafter();
static void TCASM_decode_instruction();
static void TCASM_state_regular_create_ref_list();
static void TCASM_state_regular_add_ref();
static void TCASM_handle_state_section();
static void TCASM_handle_state_section_type();
static void TCASM_handle_state_text_statement();
static void TCASM_handle_state_data_statement_databefore();
static void TCASM_handle_state_data_statement_dataafter();
static void TCASM_handle_state_text();
static void TCASM_handle_state_data_create_databefore();
static void TCASM_handle_state_data_create_dataafter();
static void TCASM_handle_state_data_define_varconst();
static void TCASM_handle_state_data_define_array();
static void TCASM_handle_state_regular();
static void TCASM_handle_state_branch();
static void TCASM_handle_state_copy();

// =============================================================================
// definicao de funcoes publicas
// =============================================================================

/**
 * Monta o arquivo de entrada com assembly da maquina hipotetica.
 * @param in Nome do arquivo fonte.
 * @param out Nome do arquivo de saida para escrever o codigo montado.
 */
void TCASM_assemble(const char* in, const char* out) {
  TCASM_hashtable_init(&TCASM_symbol_table, sizeof(TCASM_symbol_t), TCASM_HASHTABLE_DEFAULT_SIZE);
  TCASM_symbol_init(&TCASM_symbol_table);
  TCASM_list_init(&TCASM_reflist, sizeof(TCASM_reflist_node_t));
  TCASM_list_init(&TCASM_datalist, sizeof(TCASM_datalist_node_t));
  TCASM_read_file(in);
  TCASM_write_file(out);
}

// =============================================================================
// definicao de funcoes privadas
// =============================================================================

/**
 * Funcao para escrever o valor zero varias vezes em um vetor.
 * @param dst Ponteiro do vetor.
 * @param pos Posicao no vetor onde o inteiro sera escrito varias vezes.
 * @param count Quantidade de repeticoes.
 */
inline void TCASM_write_uint16_zeroarray(uint16_t dst[], size_t pos, size_t count) {
  for (size_t i = 0; i < count; ++i)
    dst[pos + i] = 0;
}

/**
 * Funcao para inicializar uma string com um char.
 * @param to Ponteiro da string que sera inicializada.
 * @param size Ponteiro do tamanho da string que sera inicializado com 1.
 * @param from Caractere que ira inicializar a string.
 */
inline void TCASM_strinit(char* to, size_t* size, int from) {
  to[0] = (char) from;
  to[1] = '\0';
  *size = 1;
}

/**
 * Funcao para inicializar uma string vazia.
 * @param to Ponteiro da string que sera inicializada.
 * @param size Ponteiro do tamanho da string que sera inicializado com 0.
 */
inline void TCASM_strinitempty(char* to, size_t* size) {
  to[0] = '\0';
  *size = 0;
}

/**
 * Funcao para concatenar um caractere no final de uma string.
 * @param to Ponteiro da string que sera inicializada.
 * @param size Ponteiro do tamanho da string ira ser incrementado.
 * @param from Caractere que entrara no final da string.
 */
inline void TCASM_strcat(char* to, size_t* size, int from) {
  to[(*size)++] = (char) from;
  to[*size] = '\0';
}

/**
 * Realiza a etapa de analise do arquivo fonte.
 * @param in Nome do arquivo fonte.
 */
void TCASM_read_file(const char* in) {
  if ((TCASM_fin = fopen(in, "r")) == NULL) {
    fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo \"%s\" para leitura\n", in);
    exit(EXIT_FAILURE);
  }
  
  // loop para ler o arquivo fonte char por char
  while (true) {
    // encerra o loop se nao eh possivel ler mais caracteres
    if (!TCASM_read_char())
      break;
    
    // chama a funcao do estado atual para tratar o char lido
    switch (TCASM_state) {
      case TCASM_STATE_SECTION:
        TCASM_handle_state_section();
        break;
        
      case TCASM_STATE_SECTION_TYPE:
        TCASM_handle_state_section_type();
        break;
        
      case TCASM_STATE_TEXT_STATEMENT:
        TCASM_handle_state_text_statement();
        break;
        
      case TCASM_STATE_DATA_STATEMENT_DATABEFORE:
        TCASM_handle_state_data_statement_databefore();
        break;
        
      case TCASM_STATE_DATA_STATEMENT_DATAAFTER:
        TCASM_handle_state_data_statement_dataafter();
        break;
        
      case TCASM_STATE_TEXT:
        TCASM_handle_state_text();
        break;
        
      case TCASM_STATE_DATA_CREATE_DATABEFORE:
        TCASM_handle_state_data_create_databefore();
        break;
        
      case TCASM_STATE_DATA_CREATE_DATAAFTER:
        TCASM_handle_state_data_create_dataafter();
        break;
        
      case TCASM_STATE_DATA_DEFINE_VARCONST:
        TCASM_handle_state_data_define_varconst();
        break;
        
      case TCASM_STATE_DATA_DEFINE_ARRAY:
        TCASM_handle_state_data_define_array();
        break;
        
      case TCASM_STATE_REGULAR:
        TCASM_handle_state_regular();
        break;
        
      case TCASM_STATE_BRANCH:
        TCASM_handle_state_branch();
        break;
        
      case TCASM_STATE_COPY:
        TCASM_handle_state_copy();
        break;
        
      default:
        break;
    }
  }
  
  fclose(TCASM_fin);
  
  if (TCASM_state == TCASM_STATE_SECTION || TCASM_state == TCASM_STATE_DATA_STATEMENT_DATABEFORE || TCASM_assembled_code_size == 0) {
    fprintf(stderr, "Erro: Arquivo fonte sem nenhuma instrucao\n");
    exit(EXIT_FAILURE);
  }
  
  if (TCASM_state != TCASM_STATE_TEXT_STATEMENT && TCASM_state != TCASM_STATE_DATA_STATEMENT_DATAAFTER) {
    fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  if (TCASM_data_read && TCASM_state == TCASM_STATE_TEXT_STATEMENT) {
    TCASM_dump_datalist();
    TCASM_dump_reflist_databefore();
  }
  else
    TCASM_dump_reflist_dataafter();
}

/**
 * Escreve o codigo montado no arquivo de saida.
 * @param out Nome do arquivo de saida para escrever o codigo montado.
 */
void TCASM_write_file(const char* out) {
  FILE* fout;
  if ((fout = fopen(out, "wb")) == NULL) {
    fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo \"%s\" para escrita\n", out);
    exit(EXIT_FAILURE);
  }
  
  fwrite(TCASM_assembled_code, sizeof(uint16_t), TCASM_assembled_code_size, fout);
  
  fclose(fout);
}

/**
 * Funcao para consumir todos os caracteres ate o fim da linha atual.
 * @return Retorna a quantidade de caracteres retirados.
 */
int TCASM_ignore_line() {
  int count = 0;
  
  do {
    if (!TCASM_read_char_from_file())
      return count;
    ++count;
  } while (TCASM_char != '\r' && TCASM_char != '\n');
  
  if (TCASM_char == '\r') {
    if (!TCASM_read_char_from_file())
      return count;
    ++count;
  }
  
  return count;
}

/**
 * Retira caracteres do arquivo de entrada ate encontrar um que nao seja
 * whitespace.
 * @return Retorna true se foi possivel ler um char, ou false se nao foi.
 */
bool TCASM_read_char() {
  TCASM_read_chars = 0;
  TCASM_read_lines = 0;
  
  // loop mantido ate ler um char valido, ou ate o arquivo acabar
  while (true) {
    if (TCASM_read_char_from_file())
      ++TCASM_read_chars;
    else
      return false;
    
    if (TCASM_char == ' ' || TCASM_char == '\t')
      continue;
    
    // lida com comentario
    if (TCASM_char == ';') {
      TCASM_read_chars += TCASM_ignore_line();
      if (feof(TCASM_fin))
        return false;
      ++TCASM_line;
      ++TCASM_read_lines;
      TCASM_changed_line = true;
      continue;
    }
    
    // lida com quebra de linha
    if (TCASM_char == '\r' || TCASM_char == '\n') {
      ++TCASM_line;
      ++TCASM_read_lines;
      TCASM_changed_line = true;
      if (TCASM_char == '\r') {
        if (!TCASM_read_char_from_file())
          return false;
        ++TCASM_read_chars;
      }
      continue;
    }
    
    // encerra o loop, pois o caractere lido eh valido
    break;
  }
  
  return true;
}

/**
 * Retira um caractere do arquivo fonte transformando em maiusculo se preciso.
 * @return Retorna true se foi possivel ler o char, ou false ao fim do arquivo.
 */
bool TCASM_read_char_from_file() {
  int buf = fgetc(TCASM_fin);
  
  if (feof(TCASM_fin))
    return false;
  
  TCASM_char = buf;
  if (TCASM_char >= 'a' && TCASM_char <= 'z')
    TCASM_char += 'A' - 'a';
  
  return true;
}

/**
 * Funcao para ler uma palavra do arquivo fonte. Podera ser uma palavra-chave,
 * ou um identificador.
 */
void TCASM_read_symbol() {
  if ((TCASM_char < 'A' || TCASM_char > 'Z') && (TCASM_char != '_')) {
    fprintf(stderr, "Erro linha %u: Simbolo iniciado com caractere invalido\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  TCASM_strinit(TCASM_word, &TCASM_symbol_size, TCASM_char);
  
  // retira caracteres do arquivo para formar a palavra
  while (true) {
    if (!TCASM_read_char_from_file()) {
      fprintf(stderr, "Erro linha %u: Simbolo invalido\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    
    if ((TCASM_char < 'A' || TCASM_char > 'Z') && (TCASM_char != '_') && (TCASM_char < '0' || TCASM_char > '9')) {
      TCASM_char = TCASM_word[TCASM_symbol_size - 1];
      fseek(TCASM_fin, -1, SEEK_CUR);
      return;
    }
    
    if (TCASM_symbol_size == TCASM_SYMBOL_MAX_IDENTIFIER_SIZE) {
      fprintf(stderr, "Erro linha %u: Tentando criar identificador com mais de %d caracteres\n", TCASM_statement_line, TCASM_SYMBOL_MAX_IDENTIFIER_SIZE);
      exit(EXIT_FAILURE);
    }
    
    TCASM_strcat(TCASM_word, &TCASM_symbol_size, TCASM_char);
  }
}

/**
 * Funcao para tentar ler um colon (dois-pontos) depois de um identificador.
 */
void TCASM_read_colon() {
  if (TCASM_read_char()) {
    TCASM_check_same_line();
    if (TCASM_char == ':')
      return;
  }
  
  // if (!TCASM_read_char() || TCASM_char != ':')
  fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tentar ler um comma (virgula).
 */
void TCASM_read_comma() {
  if (TCASM_read_char()) {
    TCASM_check_same_line();
    if (TCASM_char == ',')
      return;
  }
  
  // if (!TCASM_read_char() || TCASM_char != ',')
  fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para ler o offset em uma referencia a um vetor.
 * @return Retorna o offset da referencia.
 */
int TCASM_read_array_ref() {
  bool bracket = TCASM_char == '[';
  if (TCASM_char != '+' && !bracket) {
    fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  // verificando se existem mais caracteres validos nesta linha (indice)
  if (!TCASM_read_char()) {
    fprintf(stderr, "Erro linha %u: Indice invalido de vetor\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  TCASM_check_same_line();
  
  // lendo o indice
  fseek(TCASM_fin, -1, SEEK_CUR);
  int i;
  if (fscanf(TCASM_fin, "%i", &i) != 1) {
    fprintf(stderr, "Erro linha %u: Indice invalido de vetor\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  if (bracket) {
    if (!TCASM_read_char()) {
      fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    TCASM_check_same_line();
    
    if (TCASM_char != ']') {
      fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
  }
  
  return i;
}

/**
 * Funcao para ler a palavra-chave que indica um tipo de dado.
 * @return Retorna TCASM_SYMBOL_DIRECTIVE_SPACE ou TCASM_SYMBOL_DIRECTIVE_CONST.
 */
TCASM_symbol_type_t TCASM_read_data_type() {
  // nao reclame! eh otimizacao
  if (TCASM_char == 'S') {
    TCASM_char = fgetc(TCASM_fin);
    if (TCASM_char == 'P' || TCASM_char == 'p') {
      TCASM_char = fgetc(TCASM_fin);
      if (TCASM_char == 'A' || TCASM_char == 'a') {
        TCASM_char = fgetc(TCASM_fin);
        if (TCASM_char == 'C' || TCASM_char == 'c') {
          TCASM_char = fgetc(TCASM_fin);
          if (!feof(TCASM_fin) && (TCASM_char == 'E' || TCASM_char == 'e'))
            return TCASM_SYMBOL_DIRECTIVE_SPACE;
        }
      }
    }
  }
  
  if (TCASM_char == 'C') {
    TCASM_char = fgetc(TCASM_fin);
    if (TCASM_char == 'O' || TCASM_char == 'o') {
      TCASM_char = fgetc(TCASM_fin);
      if (TCASM_char == 'N' || TCASM_char == 'n') {
        TCASM_char = fgetc(TCASM_fin);
        if (TCASM_char == 'S' || TCASM_char == 's') {
          TCASM_char = fgetc(TCASM_fin);
          if (!feof(TCASM_fin) && (TCASM_char == 'T' || TCASM_char == 't'))
            return TCASM_SYMBOL_DIRECTIVE_CONST;
        }
      }
    }
  }
  
  // if (feof(TCASM_fin) || (data_type != "SPACE" && data_type != "CONST"))
  fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao que interrompe a montagem se o ultimo char valido lido nao esta
 * em uma nova linha.
 */
void TCASM_check_new_line() {
  if (!TCASM_changed_line) {
    fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  TCASM_changed_line = false;
  TCASM_statement_line = TCASM_line;
}

/**
 * Funcao que interrompe a montagem se o ultimo char valido lido nao esta
 * na mesma linha.
 */
void TCASM_check_same_line() {
  if (TCASM_read_lines > 0) {
    fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
}

/**
 * Funcao que interrompe a montagem se o ultimo char valido lido nao este
 * a pelo menos uma coluna de distancia.
 */
void TCASM_check_new_column() {
  if (TCASM_read_chars == 0) {
    fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
}

/**
 * Funcao que esvazia a lista de referencias do rotulo que acabou de ser
 * definido.
 */
void TCASM_dump_text_reflist() {
  TCASM_list_t* ref_list = &TCASM_symbol->sym_union.text.ref_list;
  while (ref_list->size != 0) {
    TCASM_assembled_code[((TCASM_symbol_address_reflist_t*) ref_list->first->value)->addr] = TCASM_assembled_code_size;
    free(ref_list->first->value);
    TCASM_list_erase(ref_list, ref_list->first);
  }
}

/**
 * Funcao para esvaziar a lista de referencias a uma variavel ou constante
 * quando a secao de dados vem ANTES da secao de texto no codigo-fonte.
 * @param ref_list Ponteiro da lista de referencias.
 */
void TCASM_dump_varconst_reflist_databefore(TCASM_list_t* ref_list) {
  while (ref_list->size > 0) {
    TCASM_assembled_code[((TCASM_symbol_address_reflist_t*) ref_list->first->value)->addr] = TCASM_assembled_code_size;
    free(ref_list->first->value);
    TCASM_list_erase(ref_list, ref_list->first);
  }
}

/**
 * Funcao que esvazia a lista de referencias da variavel que acabou de ser
 * definida. Usada apenas quando a secao de dados vem DEPOIS
 * da secao de texto.
 */
void TCASM_dump_var_reflist_dataafter() {
  TCASM_list_t* ref_list = &TCASM_symbol->sym_union.varconst.ref_list;
  while (ref_list->size > 0) {
    TCASM_assembled_code[((TCASM_symbol_address_varconst_reflist_t*) ref_list->first->value)->op_addr] = TCASM_assembled_code_size;
    free(ref_list->first->value);
    TCASM_list_erase(ref_list, ref_list->first);
  }
}

/**
 * Funcao que esvazia a lista de referencias da constante que acabou de ser
 * definida. Usada apenas quando a secao de dados vem DEPOIS
 * da secao de texto.
 * @param const_value Valor da constante.
 */
void TCASM_dump_const_reflist_dataafter(int const_value) {
  TCASM_list_t* ref_list = &TCASM_symbol->sym_union.varconst.ref_list;
  TCASM_symbol_address_varconst_reflist_t* ref;
  uint16_t opcode;
  while (ref_list->size > 0) {
    ref = (TCASM_symbol_address_varconst_reflist_t*) ref_list->first->value;
    opcode = TCASM_assembled_code[ref->instr_addr];
    if (opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_DIV && const_value == 0) {
      fprintf(stderr, "Erro linha %u: Divisao por constante inicializada com valor zero\n", ref->line);
      exit(EXIT_FAILURE);
    }
    else if ((opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_COPY && ref->instr_addr + 2 == ref->op_addr) ||
              opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_STORE ||
              opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_INPUT) {
      fprintf(stderr, "Erro linha %u: Escrita em memoria reservada para armazenamento de constante\n", ref->line);
      exit(EXIT_FAILURE);
    }
    TCASM_assembled_code[ref->op_addr] = TCASM_assembled_code_size;
    free(ref);
    TCASM_list_erase(ref_list, ref_list->first);
  }
}

/**
 * Funcao para esvaziar as referencias a um vetor, apenas quando a secao de
 * dados vem ANTES da secao de texto no codigo-fonte.
 * @param ref_list Ponteiro da lista de referencias.
 */
void TCASM_dump_array_reflist_databefore(TCASM_list_t* ref_list) {
  uint16_t* array_size = &TCASM_symbol->sym_union.array.size;
  TCASM_symbol_address_array_reflist_t* ref;
  while (ref_list->size > 0) {
    ref = (TCASM_symbol_address_array_reflist_t*) ref_list->first->value;
    if (ref->offset >= *array_size) {
      fprintf(stderr, "Erro linha %u: Acesso a posicao invalida do vetor\n", ref->line);
      exit(EXIT_FAILURE);
    }
    TCASM_assembled_code[ref->addr] = TCASM_assembled_code_size + ref->offset;
    free(ref);
    TCASM_list_erase(ref_list, ref_list->first);
  }
}

/**
 * Funcao que esvazia a lista de referencias do vetor que acabou de ser
 * definido. Usada apenas quando a secao de dados vem DEPOIS da secao de texto.
 */
void TCASM_dump_array_reflist_dataafter() {
  TCASM_list_t* ref_list = &TCASM_symbol->sym_union.array.ref_list;
  uint16_t* array_size = &TCASM_symbol->sym_union.array.size;
  TCASM_symbol_address_array_reflist_t* ref;
  while (ref_list->size > 0) {
    ref = (TCASM_symbol_address_array_reflist_t*) ref_list->first->value;
    if (ref->offset >= *array_size) {
      fprintf(stderr, "Erro linha %u: Acesso a posicao invalida do vetor\n", ref->line);
      exit(EXIT_FAILURE);
    }
    TCASM_assembled_code[ref->addr] = TCASM_assembled_code_size + ref->offset;
    free(ref);
    TCASM_list_erase(ref_list, ref_list->first);
  }
}

/**
 * Funcao para esvaziar a lista de dados (quando a secao de dados vem ANTES)
 * alocando os espacos e resolvendo referencias.
 */
void TCASM_dump_datalist() {
  while (TCASM_datalist.size != 0) {
    TCASM_datalist_node_t* data = (TCASM_datalist_node_t*) TCASM_datalist.first->value;
    switch (data->sym->type) {
      case TCASM_SYMBOL_ADDRESS_VAR:
        TCASM_assembled_code[TCASM_assembled_code_size] = 0;
        if (data->anonymous)
          free(data->sym);
        else
          TCASM_dump_varconst_reflist_databefore(&data->sym->sym_union.var.ref_list);
        TCASM_assembled_code_size++;
        break;
        
      case TCASM_SYMBOL_ADDRESS_CONST:
        TCASM_assembled_code[TCASM_assembled_code_size] = data->sym->sym_union.constant.value;
        if (data->anonymous)
          free(data->sym);
        else
          TCASM_dump_varconst_reflist_databefore(&data->sym->sym_union.constant.ref_list);
        TCASM_assembled_code_size++;
        break;
        
      case TCASM_SYMBOL_ADDRESS_ARRAY:
        TCASM_write_uint16_zeroarray(TCASM_assembled_code, TCASM_assembled_code_size, data->sym->sym_union.array.size);
        if (data->anonymous)
          free(data->sym);
        else
          TCASM_dump_array_reflist_databefore(&data->sym->sym_union.array.ref_list);
        TCASM_assembled_code_size += data->sym->sym_union.array.size;
        break;
        
      default:
        break;
    }
    free(TCASM_datalist.first->value);
    TCASM_list_erase(&TCASM_datalist, TCASM_datalist.first);
  }
}

/**
 * Funcao para procurar uma referencia que nao foi resolvida e acusar erro,
 * apenas quando a secao de dados vem ANTES da secao de texto no codigo-fonte.
 */
void TCASM_dump_reflist_databefore() {
  if (TCASM_reflist.size == 0)
    return;
  
  TCASM_hashtable_node_t* node;
  TCASM_symbol_t* sym;
  while (TCASM_reflist.size != 0) {
    node = (TCASM_hashtable_node_t*) ((TCASM_reflist_node_t*) TCASM_reflist.first->value)->sym;
    sym = (TCASM_symbol_t*) node->value;
    switch (sym->type) {
      case TCASM_SYMBOL_ADDRESS_TEXT:
        if (sym->sym_union.text.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Rotulo '%s' indefinido\n", ((TCASM_symbol_address_reflist_t*) sym->sym_union.text.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      case TCASM_SYMBOL_ADDRESS_VAR:
        if (sym->sym_union.var.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Variavel '%s' indefinida\n", ((TCASM_symbol_address_reflist_t*) sym->sym_union.var.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      case TCASM_SYMBOL_ADDRESS_CONST:
        if (sym->sym_union.constant.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Constante '%s' indefinida\n", ((TCASM_symbol_address_reflist_t*) sym->sym_union.constant.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      case TCASM_SYMBOL_ADDRESS_ARRAY:
        if (sym->sym_union.array.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Vetor '%s' indefinido\n", ((TCASM_symbol_address_array_reflist_t*) sym->sym_union.varconst.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      default:
        break;
    }
    free(TCASM_reflist.first->value);
    TCASM_list_erase(&TCASM_reflist, TCASM_reflist.first);
  }
}

/**
 * Funcao para procurar uma referencia que nao foi resolvida e acusar erro,
 * apenas quando a secao de dados vem DEPOIS da secao de texto no codigo-fonte.
 */
void TCASM_dump_reflist_dataafter() {
  if (TCASM_reflist.size == 0)
    return;
  
  TCASM_hashtable_node_t* node;
  TCASM_symbol_t* sym;
  while (TCASM_reflist.size != 0) {
    node = (TCASM_hashtable_node_t*) ((TCASM_reflist_node_t*) TCASM_reflist.first->value)->sym;
    sym = (TCASM_symbol_t*) node->value;
    switch (sym->type) {
      case TCASM_SYMBOL_ADDRESS_TEXT:
        if (sym->sym_union.text.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Rotulo '%s' indefinido\n", ((TCASM_symbol_address_reflist_t*) sym->sym_union.text.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      case TCASM_SYMBOL_ADDRESS_VAR:
        if (sym->sym_union.varconst.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Variavel '%s' indefinida\n", ((TCASM_symbol_address_varconst_reflist_t*) sym->sym_union.varconst.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      case TCASM_SYMBOL_ADDRESS_CONST:
        if (sym->sym_union.varconst.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Constante '%s' indefinida\n", ((TCASM_symbol_address_varconst_reflist_t*) sym->sym_union.varconst.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      case TCASM_SYMBOL_ADDRESS_ARRAY:
        if (sym->sym_union.array.ref_list.size != 0) {
          fprintf(stderr, "Erro linha %u: Vetor '%s' indefinido\n", ((TCASM_symbol_address_array_reflist_t*) sym->sym_union.varconst.ref_list.first->value)->line, node->key);
          exit(EXIT_FAILURE);
        }
        break;
        
      default:
        break;
    }
    free(TCASM_reflist.first->value);
    TCASM_list_erase(&TCASM_reflist, TCASM_reflist.first);
  }
}

/**
 * Funcao para criar um espaco anonimo quando a secao de dados vem ANTES da
 * secao de texto no codigo-fonte.
 */
void TCASM_create_anonymous_data_databefore() {
  if (TCASM_read_char()) {
    fseek(TCASM_fin, -1, SEEK_CUR); // devolve o ultimo char valido lido
    
    if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
      // variavel
      if (TCASM_read_lines > 0) {
        ++TCASM_data_size;
        TCASM_datalist_node_t tmp;
        tmp.anonymous = true;
        tmp.sym = (TCASM_symbol_t*) malloc(sizeof(TCASM_symbol_t));
        tmp.sym->type = TCASM_SYMBOL_ADDRESS_VAR;
        TCASM_list_insert(&TCASM_datalist, NULL, &tmp);
        return;
      }
      // vetor
      else {
        int i;
        if (fscanf(TCASM_fin, "%i", &i) == 1) {
          if (TCASM_data_size + i > 65536) {
            fprintf(stderr, "Erro linha %u: Memoria estourada\n", TCASM_statement_line);
            exit(EXIT_FAILURE);
          }
          
          if (i > 0) {
            TCASM_data_size += i;
            TCASM_datalist_node_t tmp;
            tmp.anonymous = true;
            tmp.sym = (TCASM_symbol_t*) malloc(sizeof(TCASM_symbol_t));
            tmp.sym->type = TCASM_SYMBOL_ADDRESS_ARRAY;
            tmp.sym->sym_union.array.size = i;
            TCASM_list_insert(&TCASM_datalist, NULL, &tmp);
            return;
          }
        }
        
        // if (fscanf(TCASM_fin, "%i", &i) != 1 || (fscanf(TCASM_fin, "%i", &i) == 1 && i <= 0))
        fprintf(stderr, "Erro linha %u: Tamanho invalido para vetor\n", TCASM_statement_line);
        exit(EXIT_FAILURE);
      }
    }
    // constante
    else {
      TCASM_check_same_line();
      
      // tenta ler um inteiro de 16 bits com sinal
      int i;
      if (fscanf(TCASM_fin, "%i", &i) == 1) {
        if (i < -32768 || i > 32767) {
          fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
          exit(EXIT_FAILURE);
        }
        
        ++TCASM_data_size;
        TCASM_datalist_node_t tmp;
        tmp.anonymous = true;
        tmp.sym = (TCASM_symbol_t*) malloc(sizeof(TCASM_symbol_t));
        tmp.sym->type = TCASM_SYMBOL_ADDRESS_CONST;
        tmp.sym->sym_union.constant.value = i;
        TCASM_list_insert(&TCASM_datalist, NULL, &tmp);
        return;
      }
      
      // if (fscanf(TCASM_fin, "%i", &i) != 1)
      fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
  }
  
  // if (!TCASM_read_char())
  fprintf(stderr, "Erro: Arquivo fonte sem nenhuma instrucao\n");
  exit(EXIT_FAILURE);
}

/**
 * Funcao para criar um espaco anonimo quando a secao de dados vem DEPOIS da
 * secao de texto no codigo-fonte.
 */
void TCASM_create_anonymous_data_dataafter() {
  if (TCASM_read_char()) {
    fseek(TCASM_fin, -1, SEEK_CUR); // devolve o ultimo char valido lido
    
    // variavel ou vetor
    if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
      // variavel
      if (TCASM_read_lines > 0) {
        TCASM_assembled_code[TCASM_assembled_code_size++] = 0;
        return;
      }
      // vetor
      else {
        int i;
        if (fscanf(TCASM_fin, "%i", &i) == 1) {
          if (TCASM_assembled_code_size + i > 65536) {
            fprintf(stderr, "Erro linha %u: Memoria estourada\n", TCASM_statement_line);
            exit(EXIT_FAILURE);
          }
          
          if (i > 0) {
            TCASM_write_uint16_zeroarray(TCASM_assembled_code, TCASM_assembled_code_size, i);
            TCASM_assembled_code_size += i;
            return;
          }
        }
        
        // if (fscanf(TCASM_fin, "%i", &i) != 1 || (fscanf(TCASM_fin, "%i", &i) == 1 && i <= 0))
        fprintf(stderr, "Erro linha %u: Tamanho invalido para vetor\n", TCASM_statement_line);
        exit(EXIT_FAILURE);
      }
    }
    // constante
    else {
      TCASM_check_same_line();
      
      // tenta ler um inteiro de 16 bits com sinal
      int i;
      if (fscanf(TCASM_fin, "%i", &i) == 1) {
        if (i < -32768 || i > 32767) {
          fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
          exit(EXIT_FAILURE);
        }
        
        TCASM_assembled_code[TCASM_assembled_code_size++] = i;
        return;
      }
      
      // if (fscanf(TCASM_fin, "%i", &i) != 1)
      fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
  }
  // variavel
  else if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
    TCASM_assembled_code[TCASM_assembled_code_size++] = 0;
    return;
  }
  
  // if (!TCASM_read_char() || TCASM_symbol->type != TCASM_SYMBOL_DIRECTIVE_SPACE)
  fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para decidir o proximo estado da maquina de acordo com a instrucao
 * que esta sendo montada.
 */
void TCASM_decode_instruction() {
  TCASM_opcode = TCASM_symbol->sym_union.instr.opcode;
  TCASM_assembled_code[TCASM_assembled_code_size++] = TCASM_opcode;
  
  if (TCASM_opcode >= TCASM_SYMBOL_INSTRUCTION_OPCODE_JMP && TCASM_opcode <= TCASM_SYMBOL_INSTRUCTION_OPCODE_JMPZ)
    TCASM_state = TCASM_STATE_BRANCH;
  else if (TCASM_opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_COPY)
    TCASM_state = TCASM_STATE_COPY;
  else if (TCASM_opcode != TCASM_SYMBOL_INSTRUCTION_OPCODE_STOP)
    TCASM_state = TCASM_STATE_REGULAR;
  else
    TCASM_state = TCASM_STATE_TEXT_STATEMENT;
}

/**
 * Funcao para criar a lista de referencias do simbolo que esta sendo
 * utilizado como operando. Cria uma referencia para a instrucao que esta
 * sendo montada e utiliza este simbolo.
 */
void TCASM_state_regular_create_ref_list() {
  TCASM_assembled_code[TCASM_assembled_code_size] = 0;
  
  bool read_char = TCASM_read_char();
  fseek(TCASM_fin, -1, SEEK_CUR); // devolve o ultimo char valido lido
  // variavel ou constante
  if (!read_char || TCASM_read_lines != 0 || TCASM_char == ',') {
    TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_VAR;
    
    TCASM_symbol_address_varconst_reflist_t tmp;
    tmp.instr_addr = TCASM_assembled_code_size - (TCASM_second_op ? 2 : 1);
    tmp.line = TCASM_statement_line;
    tmp.op_addr = TCASM_assembled_code_size;
    TCASM_list_init(&TCASM_symbol->sym_union.varconst.ref_list, sizeof(TCASM_symbol_address_varconst_reflist_t));
    TCASM_list_insert(&TCASM_symbol->sym_union.varconst.ref_list, NULL, &tmp);
  }
  // vetor
  else {
    TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_ARRAY;
    
    if (!TCASM_read_char()) {
      fprintf(stderr, "Erro linha %u: Indice invalido de vetor\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    TCASM_check_same_line();
    
    TCASM_symbol_address_array_reflist_t tmp;
    tmp.addr = TCASM_assembled_code_size;
    tmp.line = TCASM_statement_line;
    tmp.offset = TCASM_read_array_ref();
    TCASM_list_init(&TCASM_symbol->sym_union.array.ref_list, sizeof(TCASM_symbol_address_array_reflist_t));
    TCASM_list_insert(&TCASM_symbol->sym_union.array.ref_list, NULL, &tmp);
  }
  
  // inserindo na lista de elementos com referencias pendentes
  TCASM_reflist_node_t tmp;
  tmp.sym = TCASM_hashnode;
  TCASM_list_insert(&TCASM_reflist, NULL, &tmp);
  
  TCASM_assembled_code_size++;
}

/**
 * Funcao para adicionar uma referencia ao simbolo que acabou de ser lido
 * para ser usado como operando.
 */
void TCASM_state_regular_add_ref() {
  switch (TCASM_symbol->type) {
    case TCASM_SYMBOL_ADDRESS_VAR:
      // secao de dados ANTES usa a struct TCASM_symbol_address_var_t
      if (TCASM_data_read) {
        TCASM_symbol_address_reflist_t tmp;
        tmp.addr = TCASM_assembled_code_size;
        tmp.line = TCASM_statement_line;
        TCASM_list_insert(&TCASM_symbol->sym_union.var.ref_list, NULL, &tmp);
      }
      // secao de dados DEPOIS usa a struct TCASM_symbol_address_varconst_t
      else {
        TCASM_symbol_address_varconst_reflist_t tmp;
        tmp.instr_addr = TCASM_assembled_code_size - (TCASM_second_op ? 2 : 1);
        tmp.line = TCASM_statement_line;
        tmp.op_addr = TCASM_assembled_code_size;
        TCASM_list_insert(&TCASM_symbol->sym_union.varconst.ref_list, NULL, &tmp);
      }
      break;
      
    case TCASM_SYMBOL_ADDRESS_CONST:
      // sabendo que o tipo eh CONST, sabe-se que a secao de dados veio ANTES.
      // portanto, usa-se a struct TCASM_symbol_address_const_t
      {
        TCASM_symbol_address_reflist_t tmp;
        tmp.addr = TCASM_assembled_code_size;
        tmp.line = TCASM_statement_line;
        TCASM_list_insert(&TCASM_symbol->sym_union.constant.ref_list, NULL, &tmp);
      }
      break;
      
    case TCASM_SYMBOL_ADDRESS_ARRAY:
      // vetores possuem apenas uma struct
      {
        if (!TCASM_read_char()) {
          fprintf(stderr, "Erro linha %u: Indice invalido de vetor\n", TCASM_statement_line);
          exit(EXIT_FAILURE);
        }
        TCASM_check_same_line();
        
        TCASM_symbol_address_array_reflist_t tmp;
        tmp.addr = TCASM_assembled_code_size;
        tmp.line = TCASM_statement_line;
        tmp.offset = TCASM_read_array_ref();
        TCASM_list_insert(&TCASM_symbol->sym_union.array.ref_list, NULL, &tmp);
      }
      break;
      
    default:
      break;
  }
  
  TCASM_assembled_code_size++;
}

/**
 * Funcao para tratar o ultimo char lido no estado TCASM_STATE_SECTION.
 * Neste estado, a maquina esta esperando para ler a palavra chave SECTION.
 */
void TCASM_handle_state_section() {
  // nao reclame! eh otimizacao
  if (TCASM_char == 'S') {
    TCASM_char = fgetc(TCASM_fin);
    if (TCASM_char == 'E' || TCASM_char == 'e') {
      TCASM_char = fgetc(TCASM_fin);
      if (TCASM_char == 'C' || TCASM_char == 'c') {
        TCASM_char = fgetc(TCASM_fin);
        if (TCASM_char == 'T' || TCASM_char == 't') {
          TCASM_char = fgetc(TCASM_fin);
          if (TCASM_char == 'I' || TCASM_char == 'i') {
            TCASM_char = fgetc(TCASM_fin);
            if (TCASM_char == 'O' || TCASM_char == 'o') {
              TCASM_char = fgetc(TCASM_fin);
              if (!feof(TCASM_fin) && (TCASM_char == 'N' || TCASM_char == 'n')) {
                TCASM_state = TCASM_STATE_SECTION_TYPE;
                return;
              }
            }
          }
        }
      }
    }
  }
  
  // if (feof(TCASM_fin) || word != "SECTION")
  fprintf(stderr, "Erro linha %u: O arquivo fonte deve iniciar com uma diretiva SECTION\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado TCASM_STATE_SECTION_TYPE.
 * Neste estado, a maquina esta esperando para ler a palavra chave TEXT, ou
 * a palavra chave DATA.
 */
void TCASM_handle_state_section_type() {
  TCASM_check_same_line();
  TCASM_check_new_column();
  
  // nao reclame! eh otimizacao
  if (TCASM_char == 'T') {
    TCASM_char = fgetc(TCASM_fin);
    if (TCASM_char == 'E' || TCASM_char == 'e') {
      TCASM_char = fgetc(TCASM_fin);
      if (TCASM_char == 'X' || TCASM_char == 'x') {
        TCASM_char = fgetc(TCASM_fin);
        if (!feof(TCASM_fin) && (TCASM_char == 'T' || TCASM_char == 't')) {
          if (TCASM_text_read) {
            fprintf(stderr, "Erro linha %u: Secao de texto ja iniciada anteriormente\n", TCASM_statement_line);
            exit(EXIT_FAILURE);
          }
          
          TCASM_text_read = true;
          TCASM_state = TCASM_STATE_TEXT_STATEMENT;
          return;
        }
      }
    }
  }
  
  // nao reclame! eh otimizacao
  if (TCASM_char == 'D') {
    TCASM_char = fgetc(TCASM_fin);
    if (TCASM_char == 'A' || TCASM_char == 'a') {
      TCASM_char = fgetc(TCASM_fin);
      if (TCASM_char == 'T' || TCASM_char == 't') {
        TCASM_char = fgetc(TCASM_fin);
        if (!feof(TCASM_fin) && (TCASM_char == 'A' || TCASM_char == 'a')) {
          if (TCASM_data_read) {
            fprintf(stderr, "Erro linha %u: Secao de dados ja iniciada anteriormente\n", TCASM_statement_line);
            exit(EXIT_FAILURE);
          }
          
          TCASM_data_read = true;
          TCASM_state = !TCASM_text_read ? TCASM_STATE_DATA_STATEMENT_DATABEFORE : TCASM_STATE_DATA_STATEMENT_DATAAFTER;
          return;
        }
      }
    }
  }
  
  // if (feof(TCASM_fin) || (section_type != "TEXT" && section_type != "DATA"))
  fprintf(stderr, "Erro linha %u: Os tipos de secao permitidos sao TEXT e DATA\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado TCASM_STATE_TEXT_STATEMENT.
 * Neste estado, a maquina esta esperando para ler uma instrucao, ou
 * um identificador, ou a diretiva SECTION se a secao de dados ainda nao foi
 * declarada.
 */
void TCASM_handle_state_text_statement() {
  bool created;
  
  TCASM_check_new_line();
  
  TCASM_read_symbol();
  TCASM_symbol = (TCASM_symbol_t*) TCASM_hashtable_get(&TCASM_symbol_table, TCASM_word, &created);
  
  // criando e definindo um rotulo
  if (created) {
    TCASM_read_colon();
    TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_TEXT;
    TCASM_symbol->sym_union.text.addr = TCASM_assembled_code_size;
    TCASM_list_init(&TCASM_symbol->sym_union.text.ref_list, sizeof(TCASM_symbol_address_reflist_t)); // apenas para dizer que a lista esta vazia
    TCASM_state = TCASM_STATE_TEXT;
    return;
  }
  // definindo um rotulo
  else if (TCASM_symbol->type == TCASM_SYMBOL_ADDRESS_TEXT) {
    TCASM_read_colon();
    if (TCASM_symbol->sym_union.text.ref_list.size == 0) {
      fprintf(stderr, "Erro linha %u: Redefinindo identificador\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    TCASM_symbol->sym_union.text.addr = TCASM_assembled_code_size;
    TCASM_dump_text_reflist();
    TCASM_state = TCASM_STATE_TEXT;
    return;
  }
  // leitura de instrucao iniciada
  else if (TCASM_symbol->type >= TCASM_SYMBOL_INSTRUCTION_REGULAR && TCASM_symbol->type <= TCASM_SYMBOL_INSTRUCTION_STOP) {
    TCASM_decode_instruction();
    return;
  }
  // palavra-chave SECTION
  else if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SECTION) {
    if (TCASM_data_read) {
      fprintf(stderr, "Erro linha %u: Ambas as secoes de texto e de dados ja foram iniciadas anteriormente\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    TCASM_state = TCASM_STATE_SECTION_TYPE;
    return;
  }
  // outra palavra-chave
  else if (TCASM_symbol->type >= TCASM_SYMBOL_DIRECTIVE_SECTION_TYPE && TCASM_symbol->type <= TCASM_SYMBOL_DIRECTIVE_CONST) {
    fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  // if (TCASM_symbol->type == TCASM_SYMBOL_ADDRESS*)
  fprintf(stderr, "Erro linha %u: Redefinindo identificador\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado
 * TCASM_STATE_DATA_STATEMENT_DATABEFORE.
 * Neste estado, a maquina esta esperando para ler a palavra chave SECTION,
 * um identificador, ou um dado anonimo.
 */
void TCASM_handle_state_data_statement_databefore() {
  bool created;
  
  TCASM_check_new_line();
  
  TCASM_read_symbol();
  TCASM_symbol = (TCASM_symbol_t*) TCASM_hashtable_get(&TCASM_symbol_table, TCASM_word, &created);
  
  // memoria estourada
  if (TCASM_data_size + 1 > 65536) {
    fprintf(stderr, "Erro linha %u: Memoria estourada\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  // se criou agora nao eh palavra-chave
  if (created) {
    TCASM_read_colon();
    TCASM_state = TCASM_STATE_DATA_CREATE_DATABEFORE;
    return;
  }
  // palavra-chave SECTION
  else if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SECTION) {
    TCASM_state = TCASM_STATE_SECTION_TYPE;
    return;
  }
  // espaco anonimo
  else if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SPACE || TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_CONST) {
    TCASM_create_anonymous_data_databefore();
    return;
  }
  // outra palavra-chave
  else if (TCASM_symbol->type <= TCASM_SYMBOL_INSTRUCTION_STOP) {
    fprintf(stderr, "Erro linha %u: Tentando definir dado com palavra-chave\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  // if (TCASM_symbol->type == TCASM_SYMBOL_ADDRESS*)
  fprintf(stderr, "Erro linha %u: Redefinindo identificador\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado
 * TCASM_STATE_DATA_STATEMENT_DATAAFTER.
 * Neste estado, a maquina esta esperando para ler um identificador.
 */
void TCASM_handle_state_data_statement_dataafter() {
  bool created;
  
  TCASM_check_new_line();
  
  TCASM_read_symbol();
  TCASM_symbol = (TCASM_symbol_t*) TCASM_hashtable_get(&TCASM_symbol_table, TCASM_word, &created);
  
  // memoria estourada
  if (TCASM_assembled_code_size + 1 > 65536) {
    fprintf(stderr, "Erro linha %u: Memoria estourada\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  // se criou agora nao eh palavra-chave, mas eh declaracao nao utilizada
  if (created) {
    fprintf(stderr, "Aviso linha %u: Declaracao '%s' nao utilizada\n", TCASM_statement_line, TCASM_word);
    TCASM_read_colon();
    TCASM_state = TCASM_STATE_DATA_CREATE_DATAAFTER;
    return;
  }
  // definindo endereco de uma variavel ou constante
  else if (TCASM_symbol->type == TCASM_SYMBOL_ADDRESS_VAR) {
    if (TCASM_symbol->sym_union.varconst.ref_list.size > 0) {
      TCASM_read_colon();
      TCASM_state = TCASM_STATE_DATA_DEFINE_VARCONST;
      return;
    }
  }
  // definindo endereco de um vetor
  else if (TCASM_symbol->type == TCASM_SYMBOL_ADDRESS_ARRAY) {
    if (TCASM_symbol->sym_union.array.ref_list.size > 0) {
      TCASM_read_colon();
      TCASM_state = TCASM_STATE_DATA_DEFINE_ARRAY;
      return;
    }
  }
  // espaco anonimo
  else if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SPACE || TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_CONST) {
    TCASM_create_anonymous_data_dataafter();
    return;
  }
  // palavra-chave SECTION
  else if (TCASM_symbol->type == TCASM_SYMBOL_DIRECTIVE_SECTION) {
    fprintf(stderr, "Erro linha %u: Ambas as secoes de texto e de dados ja foram iniciadas anteriormente\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  // outra palavra-chave
  else if (TCASM_symbol->type <= TCASM_SYMBOL_INSTRUCTION_STOP) {
    fprintf(stderr, "Erro linha %u: Tentando definir dado com palavra-chave\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  // if (TCASM_symbol->type == TCASM_SYMBOL_ADDRESS_TEXT)
  fprintf(stderr, "Erro linha %u: Redefinindo identificador\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado TCASM_STATE_TEXT.
 * Neste estado, a maquina esta esperando uma instrucao.
 */
void TCASM_handle_state_text() {
  bool created;
  
  TCASM_check_same_line();
  
  TCASM_read_symbol();
  TCASM_symbol = (TCASM_symbol_t*) TCASM_hashtable_get(&TCASM_symbol_table, TCASM_word, &created);
  
  if (TCASM_symbol->type < TCASM_SYMBOL_INSTRUCTION_REGULAR || TCASM_symbol->type > TCASM_SYMBOL_INSTRUCTION_STOP) {
    fprintf(stderr, "Erro linha %u: Instrucao invalida\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  TCASM_decode_instruction();
}

/**
 * Funcao para tratar o ultimo char lido no estado
 * TCASM_STATE_DATA_CREATE_DATABEFORE.
 * Neste estado, a maquina esta esperando um dado (variavel, constante
 * ou vetor).
 */
void TCASM_handle_state_data_create_databefore() {
  TCASM_check_same_line();
  
  TCASM_symbol_type_t type = TCASM_read_data_type();
  // le o proximo char valido para poder checar se ocorreu uma nova linha ou nao
  if (TCASM_read_char()) {
    fseek(TCASM_fin, -1, SEEK_CUR); // devolve o ultimo char valido lido
    
    // variavel ou vetor
    if (type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
      // variavel
      if (TCASM_read_lines > 0) {
        TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_VAR;
        TCASM_list_init(&TCASM_symbol->sym_union.var.ref_list, sizeof(TCASM_symbol_address_reflist_t));
        ++TCASM_data_size;
        TCASM_datalist_node_t tmp;
        tmp.anonymous = false;
        tmp.sym = TCASM_symbol;
        TCASM_list_insert(&TCASM_datalist, NULL, &tmp);
        TCASM_state = TCASM_STATE_DATA_STATEMENT_DATABEFORE;
        return;
      }
      // vetor
      else {
        int i;
        if (fscanf(TCASM_fin, "%i", &i) == 1) {
          if (TCASM_data_size + i > 65536) {
            fprintf(stderr, "Erro linha %u: Memoria estourada\n", TCASM_statement_line);
            exit(EXIT_FAILURE);
          }
          
          if (i > 0) {
            TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_ARRAY;
            TCASM_symbol->sym_union.array.size = i;
            TCASM_list_init(&TCASM_symbol->sym_union.array.ref_list, sizeof(TCASM_symbol_address_array_reflist_t));
            TCASM_data_size += i;
            TCASM_datalist_node_t tmp;
            tmp.anonymous = false;
            tmp.sym = TCASM_symbol;
            TCASM_list_insert(&TCASM_datalist, NULL, &tmp);
            TCASM_state = TCASM_STATE_DATA_STATEMENT_DATABEFORE;
            return;
          }
        }
        
        // if (fscanf(TCASM_fin, "%i", &i) != 1 || i <= 0)
        fprintf(stderr, "Erro linha %u: Tamanho invalido para vetor\n", TCASM_statement_line);
        exit(EXIT_FAILURE);
      }
    }
    // constante
    else {
      TCASM_check_same_line();
      
      // tenta ler um inteiro de 16 bits com sinal
      int i;
      if (fscanf(TCASM_fin, "%i", &i) == 1) {
        if (i < -32768 || i > 32767) {
          fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
          exit(EXIT_FAILURE);
        }
        
        TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_CONST;
        TCASM_symbol->sym_union.constant.value = i;
        TCASM_list_init(&TCASM_symbol->sym_union.constant.ref_list, sizeof(TCASM_symbol_address_reflist_t));
        ++TCASM_data_size;
        TCASM_datalist_node_t tmp;
        tmp.anonymous = false;
        tmp.sym = TCASM_symbol;
        TCASM_list_insert(&TCASM_datalist, NULL, &tmp);
        TCASM_state = TCASM_STATE_DATA_STATEMENT_DATABEFORE;
        return;
      }
      
      // if (fscanf(TCASM_fin, "%i", &i) != 1)
      fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
  }
  
  // if (!TCASM_read_char())
  fprintf(stderr, "Erro: Arquivo fonte sem nenhuma instrucao\n");
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado
 * TCASM_STATE_DATA_CREATE_DATAAFTER.
 * Neste estado, a maquina esta esperando um dado (variavel, constante
 * ou vetor).
 */
void TCASM_handle_state_data_create_dataafter() {
  TCASM_check_same_line();
  
  TCASM_symbol_type_t type = TCASM_read_data_type();
  // le o proximo char valido para poder checar se ocorreu uma nova linha ou nao
  if (TCASM_read_char()) {
    fseek(TCASM_fin, -1, SEEK_CUR); // devolve o ultimo char valido lido
    
    // variavel ou vetor
    if (type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
      // variavel
      if (TCASM_read_lines > 0) {
        TCASM_assembled_code[TCASM_assembled_code_size++] = 0;
        TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
        return;
      }
      // vetor
      else {
        int i;
        if (fscanf(TCASM_fin, "%i", &i) == 1) {
          if (TCASM_assembled_code_size + i > 65536) {
            fprintf(stderr, "Erro linha %u: Memoria estourada\n", TCASM_statement_line);
            exit(EXIT_FAILURE);
          }
          
          if (i > 0) {
            TCASM_write_uint16_zeroarray(TCASM_assembled_code, TCASM_assembled_code_size, i);
            TCASM_assembled_code_size += i;
            TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
            return;
          }
        }
        
        // if (fscanf(TCASM_fin, "%i", &i) != 1 || i <= 0)
        fprintf(stderr, "Erro linha %u: Tamanho invalido para vetor\n", TCASM_statement_line);
        exit(EXIT_FAILURE);
      }
    }
    // constante
    else {
      TCASM_check_same_line();
      
      // tenta ler um inteiro de 16 bits com sinal
      int i;
      if (fscanf(TCASM_fin, "%i", &i) == 1) {
        if (i < -32768 || i > 32767) {
          fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
          exit(EXIT_FAILURE);
        }
        
        TCASM_assembled_code[TCASM_assembled_code_size++] = i;
        TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
        return;
      }
      
      // if (fscanf(TCASM_fin, "%i", &i) != 1)
      fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
  }
  // variavel
  else if (type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
    TCASM_assembled_code[TCASM_assembled_code_size++] = 0;
    TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
    return;
  }
  
  // if (!TCASM_read_char())
  fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado
 * TCASM_STATE_DATA_DEFINE_VARCONST.
 * Ocorre apenas quando a secao de dados vem depois da secao de texto.
 * Neste estado, a maquina esta esperando a definicao de uma variavel ou
 * constante.
 */
void TCASM_handle_state_data_define_varconst() {
  TCASM_check_same_line();
  
  TCASM_symbol_type_t type = TCASM_read_data_type();
  // le o proximo char valido para poder checar se ocorreu uma nova linha ou nao
  if (TCASM_read_char()) {
    fseek(TCASM_fin, -1, SEEK_CUR); // devolve o ultimo char valido lido
    
    // variavel ou vetor
    if (type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
      // variavel
      if (TCASM_read_lines > 0) {
        TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_VAR;
        TCASM_dump_var_reflist_dataafter();
        TCASM_assembled_code[TCASM_assembled_code_size++] = 0;
        TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
        return;
      }
      // vetor
      else {
        fprintf(stderr, "Erro linha %u: Definicao de vetor inesperada, ao esperar definicao de variavel ou constante\n", TCASM_statement_line);
        exit(EXIT_FAILURE);
      }
    }
    // constante
    else {
      TCASM_check_same_line();
      
      // tenta ler um inteiro de 16 bits com sinal
      int i;
      if (fscanf(TCASM_fin, "%i", &i) == 1) {
        if (i < -32768 || i > 32767) {
          fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
          exit(EXIT_FAILURE);
        }
        
        TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_CONST;
        TCASM_dump_const_reflist_dataafter(i);
        TCASM_assembled_code[TCASM_assembled_code_size++] = i;
        TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
        return;
      }
      
      // if (fscanf(TCASM_fin, "%i", &i) != 1)
      fprintf(stderr, "Erro linha %u: Constante invalida\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
  }
  // variavel
  else if (type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
    TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_VAR;
    TCASM_dump_var_reflist_dataafter();
    TCASM_assembled_code[TCASM_assembled_code_size++] = 0;
    TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
    return;
  }
  
  // if (!TCASM_read_char())
  fprintf(stderr, "Erro linha %u: Sentenca invalida", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado
 * TCASM_STATE_DATA_DEFINE_ARRAY.
 * Ocorre apenas quando a secao de dados vem depois da secao de texto.
 * Neste estado, a maquina esta esperando a definicao de um vetor.
 */
void TCASM_handle_state_data_define_array() {
  TCASM_check_same_line();
  
  TCASM_symbol_type_t type = TCASM_read_data_type();
  // le o proximo char valido para poder checar se ocorreu uma nova linha ou nao
  if (TCASM_read_char()) {
    fseek(TCASM_fin, -1, SEEK_CUR); // devolve o ultimo char valido lido
    
    // variavel ou vetor
    if (type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
      // variavel
      if (TCASM_read_lines > 0) {
        fprintf(stderr, "Erro linha %u: Definicao de variavel inesperada, ao esperar definicao de vetor\n", TCASM_statement_line);
        exit(EXIT_FAILURE);
      }
      // vetor
      else {
        int i;
        if (fscanf(TCASM_fin, "%i", &i) == 1) {
          if (TCASM_assembled_code_size + i > 65536) {
            fprintf(stderr, "Erro linha %u: Memoria estourada\n", TCASM_statement_line);
            exit(EXIT_FAILURE);
          }
          
          if (i > 0) {
            TCASM_symbol->sym_union.array.size = i;
            TCASM_dump_array_reflist_dataafter();
            TCASM_write_uint16_zeroarray(TCASM_assembled_code, TCASM_assembled_code_size, i);
            TCASM_assembled_code_size += i;
            TCASM_state = TCASM_STATE_DATA_STATEMENT_DATAAFTER;
            return;
          }
        }
        
        // if (fscanf(TCASM_fin, "%i", &i) != 1 || i <= 0)
        fprintf(stderr, "Erro linha %u: Tamanho invalido para vetor\n", TCASM_statement_line);
        exit(EXIT_FAILURE);
      }
    }
    // constante
    else {
      fprintf(stderr, "Erro linha %u: Definicao de constante inesperada, ao esperar definicao de vetor\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
  }
  // variavel
  else if (type == TCASM_SYMBOL_DIRECTIVE_SPACE) {
    fprintf(stderr, "Erro linha %u: Definicao de variavel inesperada, ao esperar definicao de vetor\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  
  // (!TCASM_read_char())
  fprintf(stderr, "Erro linha %u: Sentenca invalida\n", TCASM_statement_line);
  exit(EXIT_FAILURE);
}

/**
 * Funcao para tratar o ultimo char lido no estado TCASM_STATE_REGULAR.
 * Neste estado, a maquina espera ler o operando de uma instrucao que nao eh
 * desvio, nem STOP, nem COPY.
 */
void TCASM_handle_state_regular() {
  bool created;
  
  TCASM_check_same_line();
  
  TCASM_read_symbol();
  TCASM_hashnode = TCASM_hashtable_get_node(&TCASM_symbol_table, TCASM_word, &created);
  TCASM_symbol = (TCASM_symbol_t*) TCASM_hashnode->value;
  
  // criado agora
  if (created) {
    if (TCASM_data_read) {
      fprintf(stderr, "Erro linha %u: Dado indefinido\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    
    TCASM_state_regular_create_ref_list();
  }
  // operando invalido se nao eh endereco de dado
  else if (TCASM_symbol->type < TCASM_SYMBOL_ADDRESS_VAR) {
    fprintf(stderr, "Erro linha %u: Operando invalido\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  // tratar constante
  else if (TCASM_symbol->type == TCASM_SYMBOL_ADDRESS_CONST) {
    // escrita em memoria reservada para armazenamento de constante
    if (TCASM_opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_INPUT || TCASM_opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_STORE) {
      fprintf(stderr, "Erro linha %u: Escrita em memoria reservada para armazenamento de constante\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    
    // divisao por zero
    if (TCASM_opcode == TCASM_SYMBOL_INSTRUCTION_OPCODE_DIV && TCASM_symbol->sym_union.constant.value == 0) {
      fprintf(stderr, "Erro linha %u: Divisao por constante inicializada com valor zero\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    
    TCASM_state_regular_add_ref();
  }
  // adiciona uma referencia
  else
    TCASM_state_regular_add_ref();
  
  TCASM_state = TCASM_STATE_TEXT_STATEMENT;
}

/**
 * Funcao para tratar o ultimo char lido no estado TCASM_STATE_BRANCH.
 * Neste estado, a maquina espera ler o operando de uma instrucao de desvio.
 */
void TCASM_handle_state_branch() {
  bool created;
  
  TCASM_check_same_line();
  
  TCASM_read_symbol();
  TCASM_hashnode = TCASM_hashtable_get_node(&TCASM_symbol_table, TCASM_word, &created);
  TCASM_symbol = (TCASM_symbol_t*) TCASM_hashnode->value;
  
  // criado agora
  if (created) {
    TCASM_symbol->type = TCASM_SYMBOL_ADDRESS_TEXT;
    
    TCASM_symbol_address_reflist_t tmp0;
    tmp0.addr = TCASM_assembled_code_size++;
    tmp0.line = TCASM_statement_line;
    TCASM_list_init(&TCASM_symbol->sym_union.text.ref_list, sizeof(TCASM_symbol_address_reflist_t));
    TCASM_list_insert(&TCASM_symbol->sym_union.text.ref_list, NULL, &tmp0);
    
    // inserindo na lista de elementos com referencias pendentes
    TCASM_reflist_node_t tmp;
    tmp.sym = TCASM_hashnode;
    TCASM_list_insert(&TCASM_reflist, NULL, &tmp);
  }
  // operando invalido se nao eh endereco de texto
  else if (TCASM_symbol->type != TCASM_SYMBOL_ADDRESS_TEXT) {
    fprintf(stderr, "Erro linha %u: Operando invalido\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  // criado e definido anteriormente
  else if (TCASM_symbol->sym_union.text.ref_list.size == 0) {
    TCASM_assembled_code[TCASM_assembled_code_size++] = TCASM_symbol->sym_union.text.addr;
  }
  // criado anteriormente
  else {
    TCASM_symbol_address_reflist_t tmp;
    tmp.addr = TCASM_assembled_code_size++;
    tmp.line = TCASM_statement_line;
    TCASM_list_insert(&TCASM_symbol->sym_union.text.ref_list, NULL, &tmp);
  }
  
  TCASM_state = TCASM_STATE_TEXT_STATEMENT;
}

/**
 * Funcao para tratar o ultimo char lido no estado TCASM_STATE_COPY.
 * Neste estado, a maquina espera ler os operandos de uma instrucao COPY.
 */
void TCASM_handle_state_copy() {
  bool created;
  
  TCASM_check_same_line();
  
  TCASM_read_symbol();
  TCASM_hashnode = TCASM_hashtable_get_node(&TCASM_symbol_table, TCASM_word, &created);
  TCASM_symbol = (TCASM_symbol_t*) TCASM_hashnode->value;
  
  // criado agora
  if (created) {
    if (TCASM_data_read) {
      fprintf(stderr, "Erro linha %u: Dado indefinido\n", TCASM_statement_line);
      exit(EXIT_FAILURE);
    }
    
    TCASM_state_regular_create_ref_list();
    if (!TCASM_second_op)
      TCASM_read_comma();
  }
  // operando invalido se nao eh endereco de dado
  else if (TCASM_symbol->type < TCASM_SYMBOL_ADDRESS_VAR) {
    fprintf(stderr, "Erro linha %u: Operando invalido", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  // escrita em memoria reservada para armazenamento de constante
  else if (TCASM_second_op && TCASM_symbol->type == TCASM_SYMBOL_ADDRESS_CONST) {
    fprintf(stderr, "Erro linha %u: Escrita em memoria reservada para armazenamento de constante\n", TCASM_statement_line);
    exit(EXIT_FAILURE);
  }
  // adiciona uma referencia
  else {
    TCASM_state_regular_add_ref();
    if (!TCASM_second_op)
      TCASM_read_comma();
  }
  
  if (!TCASM_second_op)
    TCASM_second_op = true;
  else {
    TCASM_second_op = false;
    TCASM_state = TCASM_STATE_TEXT_STATEMENT;
  }
}
