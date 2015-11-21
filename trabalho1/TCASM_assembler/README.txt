  
  Universidade de Brasília
  Instituto de Ciências Exatas
  Departamento de Ciência da Computação
  Disciplina: Software Básico
  Turma: B
  Professor: Diego Aranha
  Semestre: 2/2013
  Alunos:
    Lucas Carvalho - 11/0034961
    Matheus Pimenta - 09/0125789
  
  ========================
  Turing-Complete Assembly - Montador e máquina de um assembly hipotético Turing-completo.
  ========================
  
  Para compilar o montador:
  gcc -std=c99 TCASM_main.c TCASM_assembler.c TCASM_hashtable.c TCASM_list.c TCASM_symbol.c -o TCASM_assembler
  
  Forma de utilização do montador:
  ./TCASM_assembler <arquivo_entrada> <arquivo_saida>
  
