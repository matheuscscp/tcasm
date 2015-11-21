all:
	../../TCASM_assembler/Debug/TCASM_assembler teste.s teste.bin
	../Debug/TCASM_IA-32_disassembler teste.bin teste.asm
	nasm -f elf -o teste.o teste.asm
	ld -s -m elf_i386 -o teste teste.o
	./teste
