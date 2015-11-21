
#include <cstdint>
#include <iostream>
#include <string>
#include <limits>

using namespace std;

typedef int16_t word;
typedef uint16_t uword;

static uword data[0x10000];
static uword pc = 0;
static word acc = 0;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 Reads the next word in the file.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
inline static uword read()
{
  return data[pc++];
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 Treats the next word as an address then return its value.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
inline static word &mem()
{
  return ((word*)data)[read()];
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 Application's entry point.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("Invalid syntax.\n");
    return 1;
  }

  std::string path = argv[1];

  for (int i = 2; i < argc; ++i)
  {
    path += " ";
    path += argv[i];
  }

  FILE *file = fopen(path.c_str(), "rb");
  fread(data, 2, 0xFFFF, file);
  fclose(file);

  while (data[pc] != 14)
  {
    switch (read())
    {
    case 1:
      acc += mem();
      break;

    case 2:
      acc -= mem();
      break;

    case 3:
      acc *= mem();
      break;

    case 4:
      {
        word aux = mem();

        if (aux == 0)
        {
          printf("Division by zero.\n");
          return 1;
        }

        acc /= aux;
      }
      break;

    case 5:
      pc = read();
      break;

    case 6:
      pc = acc < 0 ? read() : pc + 1;
      break;

    case 7:
      pc = acc > 0 ? read() : pc + 1;
      break;

    case 8:
      pc = acc == 0 ? read() : pc + 1;
      break;

    case 9:
      {
        word &aux = mem();
        mem() = aux;
      }
      break;

    case 10:
      acc = mem();
      break;

    case 11:
      mem() = acc;
      break;

    case 12:
      {
        int i;

        if (scanf("%i", &i) != 1 || i < -32768 || i > 32767)
        {
          printf("Invalid input.\n");
          return 1;
        }

        mem() = (word)i;
      }
      break;

    case 13:
      printf("%d\n", mem());
      break;

    default:
      printf("Unknown instruction code.\n");
      return 1;
    }
  }

  return 0;
}
