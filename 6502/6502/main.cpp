#include <stdio.h>
#include <stdlib.h>

using BYTE = unsigned char;
using WORD = unsigned short;
using u32 = unsigned int; // probably need s32

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

	void writeByte(BYTE value, u32& cycles, WORD address, Memory& memory) {
		memory[address] = value;
		cycles--;
	};

	void writeWord(u32& cycles, WORD value, WORD address, Memory& memory) {
		memory[address] = value & 0xFF;
		memory[address + 1] = (value >> 8);
		cycles -= 2;
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

		INS_STA_ZP = 0x85,
		INS_STA_ZPX = 0x95,
		INS_STA_ABS = 0x8D,
		INS_STA_ABSX = 0x9D,
		INS_STA_ABSY = 0x99,
		INS_STA_INDX = 0x81,
		INS_STA_INDY = 0x91,

		INS_STX_ZP = 0x86,
		INS_STX_ABS = 0x9E,

		INS_STY_ZP = 0x84,
		INS_STY_ZPX = 0x94,
		INS_STY_ABS = 0x8C,

		INS_JSR = 0x20;

	void loadRegisterSetStatus(BYTE reg) {
		Z = (reg == 0);
		N = (reg & 0b10000000) > 0;
	};

	WORD addressZeroPage(u32& cycles, Memory& memory) {
		BYTE zeroPageAddress = fetch(cycles, memory);
		return zeroPageAddress;
	};

	WORD zeroPageAddressX(u32& cycles, Memory& memory) {
		BYTE zeroPageAddress = fetch(cycles, memory);
		zeroPageAddress += X;
		cycles--;
		return zeroPageAddress;
	};
	
	WORD zeroPageAddressY(u32& cycles, Memory& memory) {
		BYTE zeroPageAddress = fetch(cycles, memory);
		zeroPageAddress += Y;
		cycles--;
		return zeroPageAddress;
	};
	
	WORD addressAbsolute(u32& cycles, Memory& memory) {
		WORD absAddress = fetchWord(cycles, memory);
		return absAddress;
	};

	WORD addressAbsoluteX(u32& cycles, Memory& memory) {
		WORD absAddress = fetchWord(cycles, memory);
		WORD absAddressX = absAddress + X;
		if (absAddressX - absAddress >= 0xFF) {
			cycles--;
		};
		return absAddressX;
	};
	
	WORD addressAbsoluteX_5(u32& cycles, Memory& memory) {
		WORD absAddress = fetchWord(cycles, memory);
		WORD absAddressX = absAddress + X;
		cycles--;
		return absAddressX;
	};

	WORD addressAbsoluteY(u32& cycles, Memory& memory) {
		WORD absAddress = fetchWord(cycles, memory);
		WORD absAddressY = absAddress + Y;
		if (absAddressY - absAddress >= 0xFF) {
			cycles--;
		};
		return absAddressY;
	};

	WORD addressAbsoluteY_5(u32& cycles, Memory& memory) {
		WORD absAddress = fetchWord(cycles, memory);
		WORD absAddressY = absAddress + Y;
		cycles--;
		return absAddressY;
	};

	WORD addressIndirectX(u32& cycles, Memory& memory) {
		BYTE zeroPageAddress = fetch(cycles, memory);
		zeroPageAddress += X;
		cycles--;
		WORD effectiveAddress = readWord(cycles, zeroPageAddress, memory);
		return effectiveAddress;
	};

	WORD addressIndirectY(u32& cycles, Memory& memory) {
		BYTE zeroPageAddress = fetch(cycles, memory);
		WORD effectiveAddress = readWord(cycles, zeroPageAddress, memory);
		WORD effectiveAddressY = effectiveAddress + Y;
		if (effectiveAddressY - effectiveAddress >= 0xFF) {
			cycles--;
		};
		return effectiveAddressY;
	};
	
	WORD addressIndirectY_6(u32& cycles, Memory& memory) {
		BYTE zeroPageAddress = fetch(cycles, memory);
		WORD effectiveAddress = readWord(cycles, zeroPageAddress, memory);
		WORD effectiveAddressY = effectiveAddress + Y;
		cycles--;
		return effectiveAddressY;
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
					loadRegisterSetStatus(X);
				} break;
				case INS_LDX_ZPY: {
					WORD zeroPageAddress = zeroPageAddressY(cycles, mem);
					X = readByteFromZeroPage(cycles, zeroPageAddress, mem);
					loadRegisterSetStatus(X);
				} break;
				case INS_LDY_ZP: {
					WORD zeroPageAddress = addressZeroPage(cycles, mem);
					Y = readByteFromZeroPage(cycles, zeroPageAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_ZPX: {
					WORD zeroPageAddressX = addressZeroPage(cycles, mem);
					A = readByteFromZeroPage(cycles, zeroPageAddressX, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDY_ZPX: {
					WORD zeroPageAddressX = addressZeroPage(cycles, mem);
					Y = readByteFromZeroPage(cycles, zeroPageAddressX, mem);
					loadRegisterSetStatus(Y);
				} break;
				case INS_LDA_ABS: {
					WORD absAddress = addressAbsolute(cycles, mem);
					A = readByte(cycles, absAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDX_ABS: {
					WORD absAddress = addressAbsolute(cycles, mem);
					X = readByte(cycles, absAddress, mem);
					loadRegisterSetStatus(X);
				} break;
				case INS_LDY_ABS: {
					WORD absAddress = addressAbsolute(cycles, mem);
					Y = readByte(cycles, absAddress, mem);
					loadRegisterSetStatus(Y);
				} break;
				case INS_LDA_ABSX: {
					WORD absAddressX = addressAbsoluteX(cycles, mem);
					A = readByte(cycles, absAddressX, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDY_ABSX: {
					WORD absAddressX = addressAbsoluteX(cycles, mem);
					Y = readByte(cycles, absAddressX, mem);
					loadRegisterSetStatus(Y);
				} break;
				case INS_LDA_ABSY: {
					WORD absAddressY = addressAbsoluteY(cycles, mem);
					A = readByte(cycles, absAddressY, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDX_ABSY: {
					WORD absAddressY = addressAbsoluteY(cycles, mem);
					X = readByte(cycles, absAddressY, mem);
					loadRegisterSetStatus(X);
				} break;
				case INS_LDA_INDX: {
					WORD effectiveAddress = addressIndirectX(cycles, mem);
					A = readByte(cycles, effectiveAddress, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_LDA_INDY: {
					WORD effectiveAddressY = addressIndirectY(cycles, mem);
					A = readByte(cycles, effectiveAddressY, mem);
					loadRegisterSetStatus(A);
				} break;
				case INS_STA_ZP: {
					WORD zeroPageAddress = addressZeroPage(cycles, mem);
					writeByte(A, cycles, zeroPageAddress, mem);
				} break;
				case INS_STX_ZP: {
					WORD zeroPageAddress = addressZeroPage(cycles, mem);
					writeByte(X, cycles, zeroPageAddress, mem);
				} break;
				case INS_STY_ZP: {
					WORD zeroPageAddress = addressZeroPage(cycles, mem);
					writeByte(Y, cycles, zeroPageAddress, mem);
				} break;
				case INS_STA_ABS: {
					WORD absAddress = addressAbsolute(cycles, mem);
					writeByte(A, cycles, absAddress, mem);
				} break;
				case INS_STX_ABS: {
					WORD absAddress = addressAbsolute(cycles, mem);
					writeByte(X, cycles, absAddress, mem);
				} break;
				case INS_STY_ABS: {
					WORD absAddress = addressAbsolute(cycles, mem);
					writeByte(Y, cycles, absAddress, mem);
				} break;
				case INS_STA_ZPX: {
					WORD zeroPageAddress = zeroPageAddressX(cycles, mem);
					writeByte(A, cycles, zeroPageAddress, mem);
				} break;
				case INS_STY_ZPX: {
					WORD zeroPageAddress = zeroPageAddressX(cycles, mem);
					writeByte(Y, cycles, zeroPageAddress, mem);
				} break;
				case INS_STA_ABSX: {
					WORD address = addressAbsoluteX_5(cycles, mem);
					writeByte(A, cycles, address, mem);
				} break;
				case INS_STA_ABSY: {
					WORD address = addressAbsoluteY_5(cycles, mem);
					writeByte(A, cycles, address, mem);
				} break;
				case INS_STA_INDX: {
					WORD effectiveAddress = addressIndirectX(cycles, mem);
					writeByte(A, cycles, effectiveAddress, mem);
				} break;
				case INS_STA_INDY: {
					WORD effectiveAddressY = addressIndirectY_6(cycles, mem);
					writeByte(A, cycles, effectiveAddressY, mem);
				} break;
				case INS_JSR: {
					WORD subAddress = fetchWord(cycles, mem);
					writeWord(cycles, PC - 1, SP, mem);
					SP += 2;
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