#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int16_t word;
typedef uint16_t uword;

static FILE* fin;
static FILE* fout;
static uword code;
static uword size = 0;

// reads the next word in the file and increments the size counter
inline static uword read() {
  fread(&code, sizeof(uword), 1, fin);
  ++size;
  return code;
}

// outputs file begining
inline static void init() {
  fprintf(fout, "\nglobal _start\n");
  
  fprintf(fout, "\nsection .bss\n");
  fprintf(fout, "    buf: resb 8\n");
  
  fprintf(fout, "\nsection .text\n");
  
  // outputs function to read int16 from stdin and store in bx
  fprintf(fout, "\nGetInt:\n");
  fprintf(fout, "    push ax\n");
  fprintf(fout, "    mov si, 0\n");
  fprintf(fout, "    mov edi, 0\n");
  fprintf(fout, "    mov ebx, 0\n");
  fprintf(fout, "    mov ecx, buf\n");
  fprintf(fout, "    mov edx, 1\n");
  fprintf(fout, "GetInt.repeat:\n");
  fprintf(fout, "    mov eax, 3\n");
  fprintf(fout, "    int 80h\n");
  fprintf(fout, "    mov al, [buf]\n");
  fprintf(fout, "    cmp al, 0x0A\n");
  fprintf(fout, "    jz GetInt.done\n");
  fprintf(fout, "    cmp al, 0x2D\n");
  fprintf(fout, "    jnz GetInt.check\n");
  fprintf(fout, "    mov edi, 1\n");
  fprintf(fout, "    jmp GetInt.repeat\n");
  fprintf(fout, "GetInt.check:\n");
  fprintf(fout, "    cmp al, 0x30\n");
  fprintf(fout, "    jl GetInt.repeat\n");
  fprintf(fout, "    cmp al, 0x39\n");
  fprintf(fout, "    jg GetInt.repeat\n");
  fprintf(fout, "    sub al, 0x30\n");
  fprintf(fout, "    mov ah, 0\n");
  fprintf(fout, "    imul si, 10\n");
  fprintf(fout, "    add si, ax\n");
  fprintf(fout, "    jmp GetInt.repeat\n");
  fprintf(fout, "GetInt.done:\n");
  fprintf(fout, "    mov bx, si\n");
  fprintf(fout, "    cmp edi, 0\n");
  fprintf(fout, "    jz GetInt.ret\n");
  fprintf(fout, "    neg bx\n");
  fprintf(fout, "GetInt.ret:\n");
  fprintf(fout, "    pop ax\n");
  fprintf(fout, "    ret\n");
  
  // outputs function to write int16 from bx to stdout
  fprintf(fout, "\nPutInt:\n");
  fprintf(fout, "    push ax\n");
  fprintf(fout, "    mov ax, bx\n");
  fprintf(fout, "    mov ecx, buf\n");
  fprintf(fout, "    mov byte [ecx], 0x20\n");
  fprintf(fout, "    cmp ax, 0\n");
  fprintf(fout, "    jnl PutInt.positive\n");
  fprintf(fout, "    mov byte [ecx], 0x2D\n");
  fprintf(fout, "    neg ax\n");
  fprintf(fout, "PutInt.positive:\n");
  fprintf(fout, "    mov bx, 10\n");
  fprintf(fout, "    add ecx, 7\n");
  fprintf(fout, "    mov byte [ecx], 0x0A\n");
  fprintf(fout, "    dec ecx\n");
  fprintf(fout, "PutInt.repeat:\n");
  fprintf(fout, "    mov dx, 0\n");
  fprintf(fout, "    div bx\n");
  fprintf(fout, "    add dl, 0x30\n");
  fprintf(fout, "    mov [ecx], dl\n");
  fprintf(fout, "    dec ecx\n");
  fprintf(fout, "    cmp ax, 0\n");
  fprintf(fout, "    jnz PutInt.repeat\n");
  fprintf(fout, "    mov bl, [buf]\n");
  fprintf(fout, "    mov [ecx], bl\n");
  fprintf(fout, "    cmp bl, 0x2D\n");
  fprintf(fout, "    jnz PutInt.write\n");
  fprintf(fout, "    dec ecx\n");
  fprintf(fout, "PutInt.write:\n");
  fprintf(fout, "    mov ebx, 1\n");
  fprintf(fout, "    mov edx, 1\n");
  fprintf(fout, "PutInt.write.repeat:\n");
  fprintf(fout, "    mov eax, 4\n");
  fprintf(fout, "    inc ecx\n");
  fprintf(fout, "    int 80h\n");
  fprintf(fout, "    cmp byte [ecx], 0x0A\n");
  fprintf(fout, "    jnz PutInt.write.repeat\n");
  fprintf(fout, "    pop ax\n");
  fprintf(fout, "    ret\n");
  
  fprintf(fout, "\n_start:\n");
}

int main(int argc, char** argv) {
  
  if (argc != 3) {
    fprintf(stderr, "Parameters: <input_file> <output_file>\n");
    return 0;
  }
  
  if ((fin = fopen(argv[1], "rb")) == NULL) {
    fprintf(stderr, "Impossible to open input file %s\n", argv[1]);
    return 0;
  }
  
  if ((fout = fopen(argv[2], "w")) == NULL) {
    fprintf(stderr, "Impossible to open output file %s\n", argv[2]);
    return 0;
  }
  
  // generating text section
  init();
  while (read() != 14) {
    fprintf(fout, "instr%d:\n", size - 1);
    switch (code) {
      // add    ACC <-- ACC + MEM[OP]
      case 1:
        fprintf(fout, "    add ax, [var%d]\n", read());
        break;
      
      // sub    ACC <-- ACC - MEM[OP]
      case 2:
        fprintf(fout, "    sub ax, [var%d]\n", read());
        break;
      
      // mult   ACC <-- ACC * MEM[OP]
      case 3:
        fprintf(fout, "    imul ax, [var%d]\n", read());
        break;
      
      // div    ACC <-- ACC / MEM[OP]
      case 4:
        fprintf(fout, "    cwd\n");
        fprintf(fout, "    idiv word [var%d]\n", read());
        break;
      
      // jmp    PC <-- OP
      case 5:
        fprintf(fout, "    jmp instr%d\n", read());
        break;
      
      // jmpn   Se ACC < 0, PC <-- OP
      case 6:
        fprintf(fout, "    cmp ax, 0\n");
        fprintf(fout, "    jl instr%d\n", read());
        break;
      
      // jmpp   Se ACC > 0, PC <-- OP
      case 7:
        fprintf(fout, "    cmp ax, 0\n");
        fprintf(fout, "    jg instr%d\n", read());
        break;
      
      // jmpz   Se ACC = 0, PC <-- OP
      case 8:
        fprintf(fout, "    cmp ax, 0\n");
        fprintf(fout, "    je instr%d\n", read());
        break;
      
      // copy   MEM[OP2] <-- MEM[OP1]
      case 9:
        fprintf(fout, "    mov bx, [var%d]\n", read());
        fprintf(fout, "    mov [var%d], bx\n", read());
        break;
      
      // load   ACC <-- MEM[OP]
      case 10:
        fprintf(fout, "    mov ax, [var%d]\n", read());
        break;
      
      // store  MEM[OP] <-- ACC
      case 11:
        fprintf(fout, "    mov [var%d], ax\n", read());
        break;
      
      // input  MEM[OP] <-- STDIN
      case 12:
        fprintf(fout, "    call GetInt\n");
        fprintf(fout, "    mov [var%d], bx\n", read());
        break;
      
      // output STDOUT <-- MEM[OP]
      case 13:
        fprintf(fout, "    mov bx, [var%d]\n", read());
        fprintf(fout, "    call PutInt\n");
        break;
      
      // won't be used
      default:
        break;
    }
  }
  
  // stop
  fprintf(fout, "\ninstr%d:\n", size - 1);
  fprintf(fout, "    mov eax, 1\n");
  fprintf(fout, "    mov ebx, 0\n");
  fprintf(fout, "    int 80h\n");
  
  // generating the data section
  fprintf(fout, "\nsection .data\n");
  while (fread(&code, sizeof(uword), 1, fin) > 0) {
    fprintf(fout, "    var%d: dw %d\n", size, (word)code);
    ++size;
  }
  
  fclose(fin);
  fclose(fout);
  
  return 0;
}
