#include <stdio.h>
#include <stdlib.h>

#include "TCASM_assembler.h"

int main(int argc, char* argv[]) {
  
  if (argc != 3) {
    fprintf(stderr, "Erro: Argumentos incorretos. Forma de utilizacao: ./TCASM <arquivo_entrada> <arquivo_saida>\n");
    exit(EXIT_FAILURE);
  }
  
  TCASM_assemble(argv[1], argv[2]);
  
  return 0;
}
