#include <stdio.h>
#include <stdlib.h>

using BYTE = unsigned char;
using WORD = unsigned short;
using u32 = unsigned int;

struct Memory{
	static constexpr u32 MAX_MEM = 1024 * 64;
	BYTE data[MAX_MEM];

	void initialize() {
		for (u32 i = 0; i < MAX_MEM; i++) {
			data[i] = 0;
		};
	};

	BYTE& operator[](u32 address) {
		return data[address];
	};
};

struct CPU {

	WORD PC; // program counter
	WORD SP; // stack pointer

	BYTE A, X, Y; // registers

	BYTE C : 1; // carry flag
	BYTE Z : 1; // zero flag
	BYTE I : 1; // interrupt disable
	BYTE D : 1; // decimal mode
	BYTE B : 1; // break command
	BYTE V : 1; // overflow flag
	BYTE N : 1; // negative flag

	void reset(Memory& mem) {
		PC = 0xFFFC;
		SP = 0x0100;
		C = Z = I = D = B = V = N = 0;
		A = X = Y = 0;
		mem.initialize();
	};

	BYTE fetch(u32& cycles, Memory& mem) {
		BYTE data = mem[PC];
		PC++;
		cycles--;
		return data;
	};

	BYTE read(u32& cycles, BYTE address, Memory& mem) {
		BYTE data = mem[address];
		cycles--;
		return data;
	};

	static constexpr BYTE
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5;

	void LDASetStatus() {
		Z = (A == 0);
		N = (A & 0b10000000) > 0;
	};

	void execute(u32 cycles, Memory& mem) {
		while (cycles > 0) {
			BYTE insion = fetch(cycles, mem);
			switch (insion) {
				case INS_LDA_IM: {
					BYTE value = fetch(cycles, mem);
					A = value;
					LDASetStatus();
				} break;
				case INS_LDA_ZP: {
					BYTE zeroPageAddress = fetch(cycles, mem);
					A = read(cycles, zeroPageAddress, mem);
					LDASetStatus();
				} break;
				default: {
					printf("Instruction not handled");
				} break;
			};
		};
	};
};

int main() {
	Memory mem;
	CPU cpu;
	cpu.reset(mem);

	mem[0xFFFC] = CPU::INS_LDA_ZP;
	mem[0xFFFD] = 0x42;
	mem[0x0042] = 0x84;

	cpu.execute(3, mem);
	return 0;
};