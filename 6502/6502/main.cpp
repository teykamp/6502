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

	void writeWord(u32& cycles, WORD value, u32 address) {
		data[address] = value & 0xFF;
		data[address + 1] = (value >> 8);
		cycles -= 2;
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

	WORD fetchWord(u32& cycles, Memory& mem) {
		BYTE data = mem[PC];
		PC++;

		data |= (mem[PC] << 8);
		PC++;
		cycles -= 2;
		return data;
	};

	BYTE readByteFromZeroPage(u32& cycles, BYTE address, Memory& mem) {
		BYTE data = mem[address];
		cycles--;
		return data;
	};
	
	BYTE readByte(u32& cycles, WORD address, Memory& mem) {
		BYTE data = mem[address];
		cycles--;
		return data;
	};
	
	WORD readWord(u32& cycles, WORD address, Memory& mem) {
		BYTE loByte = readByte(cycles, address, mem);
		BYTE hiBYte = readByte(cycles, address + 1, mem);
		return loByte | (hiBYte << 8);
	};

	static constexpr BYTE
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5,
		INS_LDA_ZPX = 0xB5,
		INS_LDA_ABS = 0xAD,
		INS_LDA_ABSX = 0xBD,
		INS_LDA_ABSY = 0xB9,
		INS_LDA_INDX = 0xA1,
		INS_LDA_INDY = 0xB1,

		INS_LDX_IM = 0xA2,
		INS_LDX_ZP = 0xA6,
		INS_LDX_ZPY = 0xB6,
		INS_LDX_ABS = 0xAE,
		INS_LDX_ABSY = 0xBE,

		INS_LDY_IM = 0xA0,
		INS_LDY_ZP = 0xA4,
		INS_LDY_ZPX = 0xB4,
		INS_LDY_ABS = 0xAC,
		INS_LDY_ABSX = 0xBC,

		INS_JSR = 0x20;

	void loadRegisterSetStatus(BYTE reg) {
		Z = (reg == 0);
		N = (reg & 0b10000000) > 0;
	};

	WORD addressZeroPage(u32& cycles, Memory& memory) {
		BYTE zeroPageAddress = fetch(cycles, memory);
		return zeroPageAddress;
	};

	void execute(u32 cycles, Memory& mem) {
		while (cycles > 0) {
			BYTE insion = fetch(cycles, mem);
			switch (insion) {
				case INS_LDA_IM: {
					A = fetch(cycles, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDX_IM: {
					X = fetch(cycles, mem);
					loadRegisterSetStatus(X);
				} break;
				case INS_LDY_IM: {
					Y = fetch(cycles, mem);
					loadRegisterSetStatus(Y);
				} break;
				case INS_LDA_ZP: {
					WORD zeroPageAddress = addressZeroPage(cycles, mem);
					A = readByteFromZeroPage(cycles, zeroPageAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDX_ZP: {
					WORD zeroPageAddress = addressZeroPage(cycles, mem);
					X = readByteFromZeroPage(cycles, zeroPageAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDY_ZP: {
					WORD zeroPageAddress = addressZeroPage(cycles, mem);
					Y = readByteFromZeroPage(cycles, zeroPageAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_ZPX: {
					BYTE zeroPageAddress = fetch(cycles, mem);
					zeroPageAddress += A;
					cycles--;
					A = readByteFromZeroPage(cycles, zeroPageAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_ABS: {
					WORD absAddress = fetchWord(cycles, mem);
					A = readByte(cycles, absAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_ABSX: {
					WORD absAddress = fetchWord(cycles, mem);
					WORD absAddressX = absAddress + X;
					A = readByte(cycles, absAddressX, mem);
					if (absAddressX - absAddress >= 0xFF) {
						cycles--;
					};
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_ABSY: {
					WORD absAddress = fetchWord(cycles, mem);
					WORD absAddressX = absAddress + Y;
					A = readByte(cycles, absAddressX, mem);
					if (absAddressX - absAddress >= 0xFF) {
						cycles--;
					};
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_INDX: {
					BYTE zeroPageAddress = fetch(cycles, mem);
					zeroPageAddress += X;
					cycles--;
					WORD effectiveAddress = readWord(cycles, zeroPageAddress, mem);
					A = readByte(cycles, effectiveAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_INDY: {
					BYTE zeroPageAddress = fetch(cycles, mem);
					WORD effectiveAddress = readWord(cycles, zeroPageAddress, mem);
					WORD effectiveAddressY = effectiveAddress + Y;
					A = readByte(cycles, effectiveAddressY, mem);
					if (effectiveAddressY - effectiveAddress >= 0xFF) {
						cycles--;
					};
					loadRegisterSetStatus(A);
				} break;
				case INS_JSR: {
					WORD subAddress = fetchWord(cycles, mem);
					mem.writeWord(cycles, PC - 1, SP);
					SP++;
					PC = subAddress;
					cycles--;
				} break;
				default: {
					printf("Instruction not handled: ", insion);
				} break;
			};
		};
	};
};

int main() {
	Memory mem;
	CPU cpu;
	cpu.reset(mem);

	mem[0xFFFC] = CPU::INS_JSR;
	mem[0xFFFD] = 0x42;
	mem[0xFFFE] = 0x42;
	mem[0x4242] = CPU::INS_LDA_IM;
	mem[0x4243] = 0x84;

	cpu.execute(9, mem);
	return 0;
};