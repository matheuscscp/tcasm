
#include <sys/stat.h>
#include <cstdint>
#include <cstdio>
#include <vector>

typedef int16_t word;
typedef uint16_t uword;

//
// Cabeçalho do ELF.
//
struct Elf32_Ehdr
{
  unsigned char e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstmdx;
};

//
// Cabeçalho da tabela de programas.
//
struct Elf32_Phdr
{
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
};

//
// Os procedimentos GetInt e PutInt necessários, usados na primeira parte do trabalho, já montados.
//
static uint8_t GetAndPutInt[] = 
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x66, 0x50, 0x66, 0xBE, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0xB9,
  0x54, 0x80, 0x04, 0x08, 0xBA, 0x01, 0x00, 0x00, 0x00, 0xB8, 0x03, 0x00, 0x00, 0x00, 0xCD, 0x80, 0xA0,
  0x54, 0x80, 0x04, 0x08, 0x3C, 0x0A, 0x74, 0x20, 0x3C, 0x2D, 0x75, 0x07, 0xBF, 0x01, 0x00, 0x00, 0x00,
  0xEB, 0xE5, 0x3C, 0x30, 0x7C, 0xE1, 0x3C, 0x39, 0x7F, 0xDD, 0x2C, 0x30, 0xB4, 0x00, 0x66, 0x6B, 0xF6,
  0x0A, 0x66, 0x01, 0xC6, 0xEB, 0xD0, 0x66, 0x89, 0xF3, 0x83, 0xFF, 0x00, 0x74, 0x03, 0x66, 0xF7, 0xDB,
  0x66, 0x58, 0xC3, 0x66, 0x50, 0x66, 0x89, 0xD8, 0xB9, 0x54, 0x80, 0x04, 0x08, 0xC6, 0x01, 0x20, 0x66,
  0x83, 0xF8, 0x00, 0x7D, 0x06, 0xC6, 0x01, 0x2D, 0x66, 0xF7, 0xD8, 0x66, 0xBB, 0x0A, 0x00, 0x83, 0xC1,
  0x07, 0xC6, 0x01, 0x0A, 0x49, 0x66, 0xBA, 0x00, 0x00, 0x66, 0xF7, 0xF3, 0x80, 0xC2, 0x30, 0x88, 0x11,
  0x49, 0x66, 0x83, 0xF8, 0x00, 0x75, 0xED, 0x8A, 0x1D, 0x54, 0x80, 0x04, 0x08, 0x88, 0x19, 0x80, 0xFB,
  0x2D, 0x75, 0x01, 0x49, 0xBB, 0x01, 0x00, 0x00, 0x00, 0xBA, 0x01, 0x00, 0x00, 0x00, 0xB8, 0x04, 0x00,
  0x00, 0x00, 0x41, 0xCD, 0x80, 0x80, 0x39, 0x0A, 0x75, 0xF3, 0x66, 0x58, 0xC3, 0x90
};

//
// Endereço virtual de carregamento.
//
static const unsigned LoadAddress = 0x08048000;

//
// Offset do ponto de entrada no programa de saída.
//
static const unsigned StartOffset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr) + 192;

//
// O arquivo de entrada.
//
static FILE *entrada;

//
// Última word lida do arquivo.
//
static uword code;

//
// Arquivo final.
//
std::vector<uint8_t> output;

//
// Lê uma palavra do arquivo de entrada.
//
inline static uword read()
{
  fread(&code, 2, 1, entrada);
  return code;
}

//
// Coloca um byte na saída.
//
inline static void putByte(uint8_t x)
{
  output.push_back(x);
}

//
// Coloca uma word na saída.
//
inline static void putWord(int x)
{
  output.push_back((uint8_t)(x & 0xFF));
  output.push_back((uint8_t)((x >> 8) & 0xFF));
  output.push_back((uint8_t)((x >> 16) & 0xFF));
  output.push_back((uint8_t)((x >> 24) & 0xFF));
}

//
// Coloca um endereço de dados no arquivo de saída.
//
inline static void putDataAddress()
{
  putWord(LoadAddress + StartOffset + (read() + 1) * 6);
}

//
// Coloca um endereço de jump no arquivo de saída.
//
inline static void putJumpAddress()
{
  int addr = read() * 6;
  addr -= output.size() + 4;
  putWord(addr);
}

//
// Escreve um arquivo ELF.
//
static void writeElf(FILE *file)
{
  // elf header
  Elf32_Ehdr ehdr =
  {
    {
      0x7F, 'E', 'L', 'F',
      1,
      1,
      1,
      0,
    },
    2,
    3,
    1,
    LoadAddress + StartOffset,
    sizeof(Elf32_Ehdr),
    0,
    0,
    sizeof(Elf32_Ehdr),
    sizeof(Elf32_Phdr),
    1,
    40,
    0,
    0
  };

  // program header table
  Elf32_Phdr phdr =
  {
    1,
    0,
    LoadAddress,
    LoadAddress,
    StartOffset + (unsigned)output.size(),
    StartOffset + (unsigned)output.size(),
    7,
    4
  };

  // escreve no arquivo
  fwrite(&ehdr, sizeof(Elf32_Ehdr), 1, file);
  fwrite(&phdr, sizeof(Elf32_Phdr), 1, file);
  fwrite(GetAndPutInt, 1, 192, file);
  fwrite(&output[0], 1, output.size(), file);
}

//
// Ponto de entrada.
//
int main(int argc, char** argv)
{
  FILE *saida;

  if (argc != 3)
  {
    fprintf(stderr, "Parameters: <input_file> <output_file>\n");
    return 0;
  }

  if ((entrada = fopen(argv[1], "rb")) == NULL)
  {
    fprintf(stderr, "Impossible to open input file %s\n", argv[1]);
    return 0;
  }

  if ((saida = fopen(argv[2], "wb")) == NULL)
  {
    fprintf(stderr, "Impossible to open output file %s\n", argv[2]);
    return 0;
  }

  // traduz as instruções
  while (read() != 14)
  {
    switch (code)
    {
      // add    ACC <-- ACC + MEM[OP]
    case 1:
      putByte(0x66);
      putByte(0x03);
      putByte(0x05);
      putDataAddress();
      break;

      // sub    ACC <-- ACC - MEM[OP]
    case 2:
      putByte(0x66);
      putByte(0x2B);
      putByte(0x05);
      putDataAddress();
      break;

      // mult   ACC <-- ACC * MEM[OP]
    case 3:
      putByte(0x66);
      putByte(0x0F);
      putByte(0xAF);
      putByte(0x05);
      putDataAddress();
      break;

      // div    ACC <-- ACC / MEM[OP]
    case 4:
      putByte(0x66);
      putByte(0x99);
      putByte(0x66);
      putByte(0xF7);
      putByte(0x3D);
      putDataAddress();
      break;

      // jmp    PC <-- OP
    case 5:
      putByte(0xE9);
      putJumpAddress();
      putByte(0x90);
      putByte(0x90);
      break;

      // jmpn   Se ACC < 0, PC <-- OP
    case 6:
      putByte(0x66);
      putByte(0x83);
      putByte(0xF8);
      putByte(0x00);

      putByte(0x0F);
      putByte(0x8C);
      putJumpAddress();
      break;

      // jmpp   Se ACC > 0, PC <-- OP
    case 7:
      putByte(0x66);
      putByte(0x83);
      putByte(0xF8);
      putByte(0x00);

      putByte(0x0F);
      putByte(0x8F);
      putJumpAddress();
      break;

      // jmpz   Se ACC = 0, PC <-- OP
    case 8:
      putByte(0x66);
      putByte(0x83);
      putByte(0xF8);
      putByte(0x00);

      putByte(0x0F);
      putByte(0x84);
      putJumpAddress();
      break;

      // copy   MEM[OP2] <-- MEM[OP1]
    case 9:
      putByte(0x66);
      putByte(0x8B);
      putByte(0x1D);
      putDataAddress();
      putByte(0x66);
      putByte(0x89);
      putByte(0x1D);
      putDataAddress();
      break;

      // load   ACC <-- MEM[OP]
    case 10:
      putByte(0x66);
      putByte(0xA1);
      putDataAddress();
      putByte(0x90);
      break;

      // store  MEM[OP] <-- ACC
    case 11:
      putByte(0x66);
      putByte(0xA3);
      putDataAddress();
      putByte(0x90);
      break;

      // input  MEM[OP] <-- STDIN
    case 12:
      putByte(0xE8);
      putWord(-(int)(output.size() + 4 + 184));

      putByte(0x66);
      putByte(0x89);
      putByte(0x1D);
      putDataAddress();
      break;

      // output STDOUT <-- MEM[OP]
    case 13:
      putByte(0x66);
      putByte(0x8B);
      putByte(0x1D);
      putDataAddress();

      putByte(0xE8);
      putWord(-(int)(output.size() + 4 + 96));
      break;
    }

    // cada palavra (16 bits) no arquivo original deve corresponder a 6 bytes em x86.
    while ((output.size() % 6) != 0)
      putByte(0x90);
  }

  // stop
  putByte(0xB8);
  putWord(0x01);
  putByte(0xBB);
  putWord(0x00);
  putByte(0xCD);
  putByte(0x80);

  // data section
  while (fread(&code, 2, 1, entrada) > 0)
  {
    putWord(code);
    putByte(0);
    putByte(0);
  }

  // termina
  fclose(entrada);
  writeElf(saida);
  fclose(saida);
  chmod(argv[2], 0775);
  return 0;
}
