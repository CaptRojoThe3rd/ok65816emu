
#include "include.h"

/*
65c816 and other CPU related stuff
*/

const char instruction_text[256][4] = {
	"BRK", "ORA", "COP", "ORA", "TSB", "ORA", "ASL", "ORA", "PHP", "ORA", "ASL", "PHD", "TSB", "ORA", "ASL", "ORA",
	"BPL", "ORA", "ORA", "ORA", "TRB", "ORA", "ASL", "ORA", "CLC", "ORA", "INC", "TCS", "TRB", "ORA", "ASL", "ORA",
	"JSR", "AND", "JSL", "AND", "BIT", "AND", "ROL", "AND", "PLP", "AND", "ROL", "PLD", "BIT", "AND", "ROL", "AND",
	"BMI", "AND", "AND", "AND", "BIT", "AND", "ROL", "AND", "SEC", "AND", "DEC", "TSC", "BIT", "AND", "ROL", "AND",
	"RTI", "EOR", "WDM", "EOR", "MVP", "EOR", "LSR", "EOR", "PHA", "EOR", "LSR", "PHK", "JMP", "EOR", "LSR", "EOR",
	"BVC", "EOR", "EOR", "EOR", "MVN", "EOR", "LSR", "EOR", "CLI", "EOR", "PHY", "TCD", "JMP", "EOR", "LSR", "EOR",
	"RTS", "ADC", "PER", "ADC", "STZ", "ADC", "ROR", "ADC", "PLA", "ADC", "ROR", "RTL", "JMP", "ADC", "ROR", "ADC",
	"BVS", "ADC", "ADC", "ADC", "STZ", "ADC", "ROR", "ADC", "SEI", "ADC", "PLY", "TDC", "JMP", "ADC", "ROR", "ADC",
	"BRA", "STA", "BRL", "STA", "STY", "STA", "STX", "STA", "DEY", "BIT", "TXA", "PHB", "STY", "STA", "STX", "STA",
	"BCC", "STA", "STA", "STA", "STY", "STA", "STX", "STA", "TYA", "STA", "TXS", "TXY", "STZ", "STA", "STZ", "STA",
	"LDY", "LDA", "LDX", "LDA", "LDY", "LDA", "LDX", "LDA", "TAY", "LDA", "TAX", "PLB", "LDY", "LDA", "LDX", "LDA",
	"BCS", "LDA", "LDA", "LDA", "LDY", "LDA", "LDX", "LDA", "CLV", "LDA", "TSX", "TYX", "LDY", "LDA", "LDX", "LDA",
	"CPY", "CMP", "REP", "CMP", "CPY", "CMP", "DEC", "CMP", "INY", "CMP", "DEX", "WAI", "CPY", "CMP", "DEC", "CMP",
	"BNE", "CMP", "CMP", "CMP", "PEI", "CMP", "DEC", "CMP", "CLD", "CMP", "PHX", "STP", "JML", "CMP", "DEC", "CMP",
	"CPX", "SBC", "SEP", "SBC", "CPX", "SBC", "INC", "SBC", "INX", "SBC", "NOP", "XBA", "CPX", "SBC", "INC", "SBC",
	"BEQ", "SBC", "SBC", "SBC", "PEA", "SBC", "INC", "SBC", "SED", "SBC", "PLX", "XCE", "JSR", "SBC", "INC", "SBC"
};

// IO
u8 ps2_input;
bool keyboard_irq_flag = false;

// CPU Registers
u16 A;
u16 X;
u16 Y;
u16 PC;
u16 S;
u16 D;
u8 DBR;
u8 K;
u8 P;
u8 E;

#define fc 0x01
#define fz 0x02
#define fi 0x04
#define fd 0x08
#define fx 0x10
#define fb 0x10
#define fm 0x20
#define fv 0x40
#define fn 0x80

// Other CPU State Info
bool waiting = false;
bool stopped = false;
bool mvn_active = false;
bool mvp_active = false;
u8 mv_start = 0;
u8 mv_end = 0;
u8 opcode;

// Memory
u8 cpu_mem[0x1000000];
const u16 irq_vec[2] = {0xffee, 0xfffe};
const u16 res_vec[2] = {0xfffc, 0xfffc};
const u16 nmi_vec[2] = {0xffea, 0xfffa};
const u16 abr_vec[2] = {0xffe8, 0xfff8};
const u16 brk_vec[2] = {0xffe6, 0xfffe};
const u16 cop_vec[2] = {0xffe4, 0xfff4};

// Interrupts
bool pending_irq = false;
bool pending_res = false;
bool pending_nmi = false;
bool pending_abr = false;

// Addressing Modes
#define adr_a		0
#define adr_A		1
#define adr_ax		2
#define adr_ay		3
#define adr_al		4
#define adr_alx		5
#define adr_ia		6
#define adr_iax		7
#define adr_d		8
#define adr_ds		9
#define adr_dx		10
#define adr_dy		11
#define adr_id		12
#define adr_ild		13
#define adr_dsy		14
#define adr_idx		15
#define adr_idy		16
#define adr_ildy	17
#define adr_i		18
#define adr_r		19
#define adr_rl		20
#define adr_s		21
#define adr_xyc		22
#define adr_imm		23
#define adr_ial		24


/*
Memory
*/

u8 gpuIORead(u32 addr);
void gpuIOWrite(u32 addr, u8 val);
bool getBlankingStatus();
u8 viaRead(u32 addr);
void viaWrite(u32 addr, u8 val);

u8 read(u32 addr)
{
	// Low RAM
	if (0x0000 <= addr && addr <= 0x7fff)
	{
		return cpu_mem[addr + 0x100000];
	}

	// Work RAM
	if ((0x100000 <= addr && addr <= 0x3fffff) || (0xfc0000 <= addr && addr <= 0xffffff) || (0xe000 <= addr && addr <= 0xffff))
	{
		return cpu_mem[addr];
	}

	// VRAM
	if (0x080000 <= addr && addr <= 0x0fffff && getBlankingStatus())
	{
		return cpu_mem[addr];
	}

	// VIA
	if (0x9000 <= addr && addr <= 0x9fff)
	{
		return viaRead(addr);
	}

	// Keyboard
	if (0xa000 <= addr <= 0xa7ff)
	{
		return ps2_input;
	}
	if (0xa800 <= addr <= 0xafff)
	{
		return keyboard_irq_flag << 7;
	}

	// GPU
	if (0xc000 <= addr && addr <= 0xcfff)
	{
		return gpuIORead(addr);
	}
}

u8 readDBR(u16 addr)
{
	return read(addr | (DBR << 16));
}

u8 readK(u16 addr)
{
	return read(addr | (K << 16));
}

u16 read16(u32 addr)
{
	return read(addr) | (read(addr + 1) << 8);
}

u16 read16DBR(u16 addr)
{
	return readDBR(addr) | (readDBR(addr + 1) << 8);
}

u16 read16K(u16 addr)
{
	return readK(addr) | (readK(((addr + 1) & 0xffff)) << 8);
}

u32 read24(u32 addr)
{
	return read(addr) | (read(addr + 1) << 8) | (read(addr + 2) << 16);
}

u32 read24DBR(u16 addr)
{
	return readDBR(addr) | (readDBR(addr + 1) << 8) | (readDBR(addr + 2) << 16);
}

u32 read24K(u16 addr)
{
	return readK(addr) | (readK(((addr + 1) & 0xffff)) << 8) | (readK(((addr + 2) & 0xffff)) << 16);
}

void write(u32 addr, u8 val)
{
	// Low RAM
	if (0x0000 <= addr && addr <= 0x7fff)
	{
		cpu_mem[addr + 0x100000] = val;
	}

	// Work RAM
	if (0x100000 <= addr && addr <= 0x3fffff)
	{
		cpu_mem[addr] = val;
	}

	// VRAM
	if (0x080000 <= addr && addr <= 0x0fffff && getBlankingStatus())
	{
		cpu_mem[addr] = val;
	}

	// VIA
	if (0x9000 <= addr && addr <= 0x9fff)
	{
		viaWrite(addr, val);
	}

	// Keyboard
	if (0xa800 <= addr && addr <= 0xafff)
	{
		keyboard_irq_flag = false;
	}

	// GPU
	if (0xc000 <= addr && addr <= 0xcfff)
	{
		gpuIOWrite(addr, val);
	}
}

void writeDBR(u16 addr, u8 val)
{
	write(addr | (DBR << 16), val);
}

void writeK(u16 addr, u8 val)
{
	write(addr | (K << 16), val);
}

void write16(u32 addr, u16 val)
{
	write(addr, val & 0xff);
	write(addr + 1, val >> 8);
}

void write16DBR(u16 addr, u16 val)
{
	write(addr | (DBR << 16), val & 0xff);
	write(addr | (DBR << 16) + 1, val >> 8);
}

void write16K(u16 addr, u16 val)
{
	write(addr | (K << 16), val & 0xff);
	write(((addr + 1) & 0xffff) | (K << 16), val >> 8);
}

void push(u8 val)
{
	write(S, val);
	S--;
}

void push16(u16 val)
{
	write(S, val >> 8);
	S--;
	write(S, val & 0xff);
	S--;
}

u8 pull()
{
	S++;
	return read(S);
}

u16 pull16()
{
	S++;
	u16 val = read(S);
	S++;
	val |= (read(S) << 8);
}

u32 DBRaddr(u16 addr)
{
	return addr | (DBR << 16);
}

u32 Kaddr(u16 addr)
{
	return addr | (K << 16);
}


/*
CPU Flag Reads
*/

u8 FLC()
{
	return (P & fc);
}

u8 FLZ()
{
	return (P & fz) >> 1;
}

u8 FLI()
{
	return (P & fi) >> 2;
}

u8 FLD()
{
	return (P & fd) >> 3;
}

u8 FLX()
{
	return (P & fx) >> 4;
}

u8 FLM()
{
	return (P & fm) >> 5;
}

u8 FLV()
{
	return (P & fv) >> 6;
}

u8 FLN()
{
	return (P & fn) >> 7;
}

bool isEM()
{
	return E | FLM();
}

bool isEX()
{
	return E | FLX();
}

void setF(u8 flags)
{
	P |= flags;
}

void clrF(u8 flags)
{
	P &= ~flags;
}

void updateZero8(u8 val)
{
	if (val == 0)
	{
		setF(fz);
	}
	else
	{
		clrF(fz);
	}
}

void updateZero16(u16 val)
{
	if (val == 0)
	{
		setF(fz);
	}
	else
	{
		clrF(fz);
	}
}

void updateCarry8(u8 old, u8 new)
{
	if (old > new)
	{
		setF(fc);
	}
	else
	{
		clrF(fc);
	}
}

void updateCarry16(u16 old, u16 new)
{
	if (old > new)
	{
		setF(fc);
	}
	else
	{
		clrF(fc);
	}
}

void updateNegative8(u8 val)
{
	if (val & 0x80)
	{
		setF(fn);
	}
	else
	{
		clrF(fn);
	}
}

void updateNegative16(u16 val)
{
	if (val & 0x8000)
	{
		setF(fn);
	}
	else
	{
		clrF(fn);
	}
}

void updateOverflow8(u8 old, u8 new)
{
	if ((old & 0x80) != (new & 0x80))
	{
		setF(fv);
	}
	else
	{
		clrF(fv);
	}
}

void updateOverflow16(u16 old, u16 new)
{
	if ((old & 0x8000) != (new & 0x8000))
	{
		setF(fv);
	}
	else
	{
		clrF(fv);
	}
}


/*
Registers
*/

u8 AL()
{
	return A & 0xff;
}

u8 AH()
{
	return A >> 8;
}

u8 XL()
{
	return X & 0xff;
}

u8 XH()
{
	return X >> 8;
}

u8 YL()
{
	return Y & 0xff;
}

u8 YH()
{
	return Y >> 8;
}

u8 PCL()
{
	return PC & 0xff;
}

u8 PCH()
{
	return PC >> 8;
}

u16 Xindex()
{
	return isEX() ? XL() : X;
}

u16 Yindex()
{
	return isEX() ? YL() : Y;
}

void setAL(u8 val)
{
	A &= 0xff00;
	A |= val;
}

void setXL(u8 val)
{
	X &= 0xff00;
	X |= val;
}

void setYL(u8 val)
{
	Y &= 0xff00;
	Y |= val;
}


/*
Operand Fetching
*/

u16 fetchOperand_Load(AddrMode mode, u8 reg_flag)
{
	u16 val;
	u32 final_addr;
	u16 pointer_addr;

	switch (mode)
	{
		// Absolute
		case adr_a:
			final_addr = read16K(PC);
			PC += 2;

			if (reg_flag)
			{
				return readDBR(final_addr);
			}
			else
			{
				return read16DBR(final_addr);
			}

		// Absolute,X
		case adr_ax:
			final_addr = read16K(PC);
			PC += 2;

			final_addr = DBRaddr(final_addr) + Xindex();

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Absolute,Y
		case adr_ay:
			final_addr = read16K(PC);
			PC += 2;

			final_addr = DBRaddr(final_addr) + Yindex();

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Long
		case adr_al:
			final_addr = read24K(PC);
			PC += 3;

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Long,X
		case adr_alx:
			final_addr = read24K(PC);
			PC += 3;

			final_addr += Xindex();

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Direct
		case adr_d:
			final_addr = readK(PC);
			PC++;

			final_addr = (final_addr + D) & 0xffff;
			
			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Direct,S
		case adr_ds:
			final_addr = readK(PC);
			PC++;

			final_addr = (final_addr + D + S) & 0xffff;
			
			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Direct,X
		case adr_dx:
			final_addr = readK(PC);
			PC++;

			final_addr = (final_addr + D + Xindex()) & 0xffff;

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Direct,Y
		case adr_dy:
			final_addr = readK(PC);
			PC++;

			final_addr = (final_addr + D + Yindex()) & 0xffff;

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// (Direct)
		case adr_id:
			pointer_addr = readK(PC);
			PC++;

			pointer_addr += D;
			final_addr = read16(pointer_addr);

			if (reg_flag)
			{
				return readDBR(final_addr);
			}
			else
			{
				return read16DBR(final_addr);
			}

		// [Direct]
		case adr_ild:
			pointer_addr = readK(PC);
			PC++;

			pointer_addr += D;
			final_addr = read24(pointer_addr);

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// (Direct,S),Y
		case adr_dsy:
			pointer_addr = readK(PC);
			PC++;

			pointer_addr += D + S;
			final_addr = read16(pointer_addr) + Yindex() + (DBR << 16);

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// (Direct,X)
		case adr_idx:
			pointer_addr = readK(PC);
			PC++;

			pointer_addr += D + Xindex();
			final_addr = DBRaddr(read16(pointer_addr));

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// (Direct),Y
		case adr_idy:
			pointer_addr = readK(PC);
			PC++;

			pointer_addr += D;
			final_addr = DBRaddr(read16(pointer_addr));
			final_addr += Yindex();

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// [Direct],Y
		case adr_ildy:
			pointer_addr = readK(PC);
			PC++;

			pointer_addr += D;
			final_addr = read24(pointer_addr);
			final_addr += Yindex();

			if (reg_flag)
			{
				return read(final_addr);
			}
			else
			{
				return read16(final_addr);
			}

		// Immediate
		case adr_imm:
			;
			u16 val = read16K(PC);

			if (E || reg_flag)
			{
				PC++;
				return val & 0xff;
			}
			else
			{
				PC += 2;
				return val;
			}
	}
}

u32 fetchOperand_ReadModifyWrite(AddrMode mode, u8 flag)
{
	u32 final_addr;

	switch (mode)
	{
		case adr_a:
			final_addr = read16K(PC);
			PC += 2;
			return DBRaddr(final_addr);

		case adr_ax:
			final_addr = read16K(PC);
			PC += 2;
			final_addr = DBRaddr(final_addr) + Xindex();
			return final_addr;

		case adr_d:
			final_addr = readK(PC);
			PC++;
			final_addr += D;
			final_addr &= 0xffff;
			return final_addr;

		case adr_dx:
			final_addr = readK(PC);
			PC++;
			final_addr += D + Xindex();
			final_addr &= 0xffff;
			return final_addr;
	}
}

u32 fetchOperand_Jump(AddrMode mode)
{
	u32 address;
	u16 pointer;

	switch (mode)
	{
		case adr_a:
			address = read16K(PC);
			PC += 2;
			address = Kaddr(address);
			return address;

		case adr_al:
			address = read16K(PC);
			PC += 2;
			address |= (readK(PC) << 16);
			PC++;
			return address;
		
		case adr_ia:
			pointer = read16K(PC);
			PC += 2;
			address = read16DBR(pointer);
			address = Kaddr(address);
			return address;

		case adr_iax:
			pointer = read16K(PC);
			PC += 2;
			pointer += Xindex();
			address = read16DBR(pointer);
			address = Kaddr(address);
			return address;

		case adr_ial:
			pointer = read16K(PC);
			PC += 2;
			address = read24DBR(pointer);
			return address;
	}
}

u32 fetchOperand_Store(AddrMode mode, u8 flag)
{
	u32 address;
	u16 pointer;

	switch (mode)
	{
		case adr_a:
			address = read16K(PC);
			PC += 2;
			address = DBRaddr(address);
			return address;

		case adr_ax:
			address = read16K(PC);
			PC += 2;
			address = DBRaddr(address);
			address += Xindex();
			return address;

		case adr_ay:
			address = read16K(PC);
			PC += 2;
			address = DBRaddr(address);
			address += Yindex();
			return address;

		case adr_al:
			address = read24K(PC);
			PC += 3;
			return address;

		case adr_alx:
			address = read24K(PC);
			PC += 3;
			address += Xindex();
			return address;

		case adr_d:
			address = readK(PC);
			PC++;
			address += D;
			address &= 0xffff;
			return address;

		case adr_ds:
			address = readK(PC);
			PC++;
			address += D + S;
			address &= 0xffff;
			return address;

		case adr_dx:
			address = readK(PC);
			PC++;
			address += D + X;
			address &= 0xffff;
			return address;

		case adr_dy:
			address = readK(PC);
			PC++;
			address += D + Y;
			address &= 0xffff;
			return address;

		case adr_id:
			pointer = readK(PC);
			PC++;
			pointer += D;
			address = read16(pointer);
			address = DBRaddr(address);
			return address;

		case adr_ild:
			pointer = readK(PC);
			PC++;
			pointer += D;
			address = read24(pointer);
			return address;

		case adr_dsy:
			pointer = readK(PC);
			PC++;
			pointer += D + S;
			address = read16(pointer);
			address = DBRaddr(address);
			address += Y;
			return address;

		case adr_idx:
			pointer = readK(PC);
			PC++;
			pointer += D + X;
			address = read16(pointer);
			address = DBRaddr(address);
			return address;
		
		case adr_idy:
			pointer = readK(PC);
			PC++;
			pointer += D;
			address = read16(pointer);
			address = DBRaddr(address);
			address += Y;
			return address;

		case adr_ildy:
			pointer = readK(PC);
			PC++;
			pointer += D;
			address = read24(pointer);
			address += Y;
			return address;
	}
}


/*
Cycles
*/

CycleCount getCycles_Load_Store(AddrMode mode, u8 flag)
{
	switch (mode)
	{
		case adr_a:
			return 5 - flag;

		case adr_ax:
			return 6 - flag - FLX();

		case adr_ay:
			return 6 - flag - FLX();

		case adr_al:
			return 6 - flag;

		case adr_alx:
			return 6 - flag;

		case adr_d:
			return 4 - flag;

		case adr_ds:
			return 5 - flag;
		
		case adr_dx:
			return 5 - flag;

		case adr_id:
			return 6 - flag;

		case adr_ild:
			return 7 - flag;

		case adr_dsy:
			return 8 - flag;

		case adr_idx:
			return 7 - flag;

		case adr_idy:
			return 7 - flag - FLX();

		case adr_ildy:
			return 7 - flag;

		case adr_imm:
			return 3 - flag;
	}
}

CycleCount getCycles_ReadModifyWrite(AddrMode mode, u8 flag)
{
	switch (mode)
	{
		case adr_a:
			return 8 - (flag * 2);

		case adr_ax:
			return 9 - (flag * 2);
		
		case adr_d:
			return 7 - (flag * 2);

		case adr_dx:
			return 8 - (flag * 2);

		case adr_A:
			return 2;
	}
}

CycleCount getCycles_Jump(AddrMode mode)
{
	switch (mode)
	{
		case adr_a:
			return 3;
		
		case adr_al:
			return 4;
		
		case adr_ia:
			return 5;

		case adr_iax:
		case adr_ial:
			return 6;
	}
}

CycleCount getCycles_SubroutineJump(AddrMode mode)
{
	switch (mode)
	{
		case adr_a:
			return 6;

		case adr_al:
		case adr_iax:
			return 8;
	}
}


/*
CPU Instructions
*/

CycleCount ADC(AddrMode mode)
{
	u16 addend = fetchOperand_Load(mode, FLM());

	if (isEM())
	{
		u8 old_al = AL();
		u8 al = old_al + addend + FLC();

		updateCarry8(old_al, al);
		updateZero8(al);
		updateNegative8(al);
		updateOverflow8(old_al, al);

		setAL(al);
	}
	else
	{
		u16 old_a = A;
		A += addend + FLC();

		updateCarry16(old_a, A);
		updateZero16(A);
		updateNegative16(A);
		updateOverflow16(old_a, A);
	}

	return getCycles_Load_Store(mode, FLM());
}

CycleCount AND(AddrMode mode)
{
	u16 andval = fetchOperand_Load(mode, FLM());

	if (isEM())
	{
		setAL(AL() & andval); 

		updateZero8(AL());
		updateNegative8(AL());
	}
	else
	{
		A &= andval; 

		updateZero16(A);
		updateNegative16(A);
	}

	return getCycles_Load_Store(mode, FLM());
}

CycleCount ASL(AddrMode mode)
{
	if (mode == adr_A)
	{
		if (isEM())
		{
			if (AL() & 0x80) setF(fc);
			else clrF(fc);
			setAL(AL() << 1);
			updateZero8(AL());
			updateNegative8(AL());
		}
		else
		{
			if (A & 0x8000) setF(fc);
			else clrF(fc);
			A <<= 1;
			updateZero16(A);
			updateNegative16(A);
		}

		return 2;
	}
	
	u32 val_addr = fetchOperand_ReadModifyWrite(mode, FLM());
	if (isEM())
	{
		u8 val = read(val_addr);
		if (val & 0x80) setF(fc);
		else clrF(fc);
		val <<= 1;
		updateZero8(val);
		updateNegative8(val);
		write(val_addr, val);
	}
	else
	{
		u16 val = read16(val_addr);
		if (val & 0x8000) setF(fc);
		else clrF(fc);
		val <<= 1;
		updateZero16(val);
		updateNegative16(val);
		write16(val_addr, val);
	}

	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount BCC(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (!FLC())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount BCS(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (FLC())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount BEQ(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (FLZ())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount BIT(AddrMode mode)
{
	if (isEM())
	{
		u8 val = fetchOperand_Load(mode, FLM());
		P &= 0x3f;
		P |= (val & 0xc0);
		u8 al_copy = AL() & val;

		updateZero8(al_copy);
		updateNegative8(al_copy);

		return getCycles_Load_Store(mode, FLM());
	}
	
	u16 val = fetchOperand_Load(mode, FLM());
	P &= 0x3f;
	P |= ((val & 0xc000) >> 8);
	u16 a_copy = A & val;

	updateZero16(a_copy);
	updateNegative16(a_copy);

	return getCycles_Load_Store(mode, FLM());
}

CycleCount BMI(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (FLN())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount BNE(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (!FLZ())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount BPL(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (!FLN())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount BRA(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	PC += offset;
	return 3;
}

CycleCount BRK(AddrMode mode)
{
	if (!E) push(K);
	push16(PC);
	push(P);

	K = 0;
	PC = brk_vec[E];
	PC = read16(PC);

	printf("BRK\n");

	return 8 - E;
}

CycleCount BRL(AddrMode mode)
{
	i16 offset = read16K(PC);
	PC += 2;

	PC += offset;
	return 3;
}

CycleCount BVC(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (!FLV())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount BVS(AddrMode mode)
{
	i8 offset = readK(PC);
	PC++;

	if (FLV())
	{
		PC += offset;
		return 3;
	}

	return 2;
}

CycleCount CLC(AddrMode mode)
{
	clrF(fc);
	return 2;
}

CycleCount CLD(AddrMode mode)
{
	clrF(fd);
	return 2;
}

CycleCount CLI(AddrMode mode)
{
	clrF(fi);
	return 2;
}

CycleCount CLV(AddrMode mode)
{
	clrF(fv);
	return 2;
}

CycleCount CMP(AddrMode mode)
{

	if (isEM())
	{
		u8 cmp_val = fetchOperand_Load(mode, FLM());

		if (AL() == cmp_val) setF(fz);
		else clrF(fz);
		if (AL() >= cmp_val) setF(fc);
		else clrF(fc);
		if (AL() >= 0x80) setF(fn);
		else clrF(fn);

		return getCycles_Load_Store(mode, FLM());
	}

	u16 cmp_val = fetchOperand_Load(mode, FLM());

	if (A == cmp_val) setF(fz);
	else clrF(fz);
	if (A >= cmp_val) setF(fc);
	else clrF(fc);
	if (A >= 0x80) setF(fn);
	else clrF(fn);

	return getCycles_Load_Store(mode, FLM());
}

CycleCount COP(AddrMode mode)
{
	if (!E) push(K);
	push16(PC);
	push(P);

	K = 0;
	PC = cop_vec[E];
	PC = read16(PC);

	return 8 - E;
}

CycleCount CPX(AddrMode mode)
{
	if (isEX())
	{
		u8 cmp_val = fetchOperand_Load(mode, FLX());

		if (XL() == cmp_val) setF(fz);
		else clrF(fz);
		if (XL() >= cmp_val) setF(fc);
		else clrF(fc);
		if (XL() >= 0x80) setF(fn);
		else clrF(fn);

		return getCycles_Load_Store(mode, FLX());
	}

	u16 cmp_val = fetchOperand_Load(mode, FLX());

	if (X == cmp_val) setF(fz);
	else clrF(fz);
	if (X >= cmp_val) setF(fc);
	else clrF(fc);
	if (X >= 0x80) setF(fn);
	else clrF(fn);

	return getCycles_Load_Store(mode, FLX());
}

CycleCount CPY(AddrMode mode)
{
	if (isEX())
	{
		u8 cmp_val = fetchOperand_Load(mode, FLX());
		
		if (YL() == cmp_val) setF(fz);
		else clrF(fz);
		if (YL() >= cmp_val) setF(fc);
		else clrF(fc);
		if (YL() >= 0x80) setF(fn);
		else clrF(fn);

		return getCycles_Load_Store(mode, FLX());
	}

	u16 cmp_val = fetchOperand_Load(mode, FLX());

	if (Y == cmp_val) setF(fz);
	else clrF(fz);
	if (Y >= cmp_val) setF(fc);
	else clrF(fc);
	if (Y >= 0x8000) setF(fn);
	else clrF(fn);

	return getCycles_Load_Store(mode, FLX());
}

CycleCount DEC(AddrMode mode)
{
	if (mode == adr_A)
	{
		if (isEM())
		{
			setAL(AL() - 1);
			updateZero8(AL());
			updateNegative8(AL());
		}
		else
		{
			A--;
			updateZero16(A);
			updateNegative16(A);
		}

		return 2;
	}

	u32 val_addr = fetchOperand_ReadModifyWrite(mode, FLM());

	if (isEM())
	{
		u8 val = read(val_addr);
		val--;
		write(val_addr, val);

		updateZero8(val);
		updateNegative8(val);

		return getCycles_ReadModifyWrite(mode, FLM());
	}

	u16 val = read16(val_addr);
	val--;
	write16(val_addr, val);

	updateZero16(val);
	updateNegative16(val);

	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount DEX(AddrMode mode)
{
	if (isEX())
	{
		setXL(XL() - 1);
		updateZero8(XL());
		updateNegative8(XL());
		return 2;
	}

	X--;
	updateZero16(X);
	updateNegative16(X);
	return 2;
}

CycleCount DEY(AddrMode mode)
{
	if (isEX())
	{
		setYL(YL() - 1);
		updateZero8(YL());
		updateNegative8(YL());
		return 2;
	}

	Y--;
	updateZero16(Y);
	updateNegative16(Y);
	return 2;
}

CycleCount EOR(AddrMode mode)
{
	if (isEM())
	{
		u8 val = fetchOperand_Load(mode, FLM());
		setAL(AL() ^ val);
		updateZero8(AL());
		updateNegative8(AL());
		return getCycles_Load_Store(mode, FLM());
	}

	u16 val = fetchOperand_Load(mode, FLM());
	A ^= val;
	updateZero16(A);
	updateNegative16(A);
	return getCycles_Load_Store(mode, FLM());
}

CycleCount INC(AddrMode mode)
{
	if (mode == adr_A)
	{
		if (isEM())
		{
			setAL(AL() + 1);
			updateZero8(AL());
			updateNegative8(AL());
		}
		else
		{
			A++;
			updateZero16(A);
			updateNegative16(A);
		}

		return 2;
	}

	u32 val_addr = fetchOperand_ReadModifyWrite(mode, FLM());

	if (isEM())
	{
		u8 val = read(val_addr);
		val++;
		write(val_addr, val);

		updateZero8(val);
		updateNegative8(val);

		return getCycles_ReadModifyWrite(mode, FLM());
	}

	u16 val = read16(val_addr);
	val++;
	write16(val_addr, val);

	updateZero16(val);
	updateNegative16(val);

	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount INX(AddrMode mode)
{
	if (isEX())
	{
		setXL(XL() + 1);
		updateZero8(XL());
		updateNegative8(XL());
		return 2;
	}

	X++;
	updateZero16(X);
	updateNegative16(X);
	return 2;
}

CycleCount INY(AddrMode mode)
{
	if (isEX())
	{
		setYL(YL() + 1);
		updateZero8(YL());
		updateNegative8(YL());
		return 2;
	}

	Y++;
	updateZero16(Y);
	updateNegative16(Y);
	return 2;
}

CycleCount JMP(AddrMode mode)
{
	u32 addr = fetchOperand_Jump(mode);
	PC = addr & 0xffff;
	K = addr >> 16;
	return getCycles_Jump(mode);
}

CycleCount JML(AddrMode mode)
{
	return JMP(mode);
}

CycleCount JSR(AddrMode mode)
{
	push16(PC + 1);
	JMP(mode);
	return getCycles_SubroutineJump(mode);
}

CycleCount JSL(AddrMode mode)
{
	push(K);
	push16(PC + 2);
	JMP(mode);
	return getCycles_SubroutineJump(mode);
}

CycleCount LDA(AddrMode mode)
{
	if (isEM())
	{
		u8 val = fetchOperand_Load(mode, FLM());
		setAL(val);
		updateZero8(AL());
		updateNegative8(AL());
		return getCycles_Load_Store(mode, FLM());
	}
	A = fetchOperand_Load(mode, FLM()); 
	updateZero16(A);
	updateNegative16(A);
	return getCycles_Load_Store(mode, FLM());
}

CycleCount LDX(AddrMode mode)
{
	if (isEX())
	{
		u8 val = fetchOperand_Load(mode, FLX());
		setXL(val);
		updateZero8(XL());
		updateNegative8(XL());
		return getCycles_Load_Store(mode, FLX());
	}
	X = fetchOperand_Load(mode, FLX());
	updateZero16(X);
	updateNegative16(X);
	return getCycles_Load_Store(mode, FLX());
}

CycleCount LDY(AddrMode mode)
{
	if (isEX())
	{
		u8 val = fetchOperand_Load(mode, FLX());
		setYL(val);
		updateZero8(YL());
		updateNegative8(YL());
		return getCycles_Load_Store(mode, FLX());
	}
	Y = fetchOperand_Load(mode, FLX());
	updateZero16(Y);
	updateNegative16(Y);
	return getCycles_Load_Store(mode, FLX());
}

CycleCount LSR(AddrMode mode)
{
	clrF(fn);

	if (mode == adr_A)
	{
		if (isEM())
		{
			if (AL() & 0x01) setF(fc);
			else clrF(fc);
			setAL(AL() >> 1);
			updateZero8(AL());
		}
		else
		{
			if (A & 0x0001) setF(fc);
			else clrF(fc);
			A >>= 1;
			updateZero16(A);
		}

		return 2;
	}
	
	u32 val_addr = fetchOperand_ReadModifyWrite(mode, FLM());
	if (isEM())
	{
		u8 val = read(val_addr);
		if (val & 0x01) setF(fc);
		else clrF(fc);
		val >>= 1;
		updateZero8(val);
		write(val_addr, val);
	}
	else
	{
		u16 val = read16(val_addr);
		if (val & 0x0001) setF(fc);
		else clrF(fc);
		val >>= 1;
		updateZero16(val);
		write16(val_addr, val);
	}

	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount MVN(AddrMode mode)
{
	mvn_active = true;
	mv_end = readK(PC);
	DBR = mv_end;
	PC++;
	mv_start = readK(PC);
	PC++;
}

CycleCount MVP(AddrMode mode)
{
	mvp_active = true;
	mv_end = readK(PC);
	DBR = mv_end;
	PC++;
	mv_start = readK(PC);
	PC++;
}

CycleCount NOP(AddrMode mode)
{
	//printf("NOP\n");
	//printf("%04x\n", A);
	//printf("%04x\n", X);
	//printf("%04x\n", Y);
	return 2;
}

CycleCount ORA(AddrMode mode)
{
	if (isEM())
	{
		u8 val = fetchOperand_Load(mode, FLM());
		setAL(AL() | val);
		updateZero8(AL());
		updateNegative8(AL());
		return getCycles_Load_Store(mode, FLM());
	}
	A |= fetchOperand_Load(mode, fm);
	updateZero16(A);
	updateNegative16(A);
	return getCycles_Load_Store(mode, FLM());
}

CycleCount PEA(AddrMode mode)
{
	u16 val = read16K(PC);
	PC += 2;
	push16(val);
	return 5;
}

CycleCount PEI(AddrMode mode)
{
	u16 addr = read16K(PC);
	PC += 2;
	addr += D;
	u16 val = read16(addr);
	push16(val);
	return 6;
}

CycleCount PER(AddrMode mode)
{
	i16 offset = read16K(PC);
	PC += 2;
	u16 val = PC + offset;
	push16(val);
	return 6;
}

CycleCount PHA(AddrMode mode)
{
	if (isEM())
	{
		push(AL());
		return 3;
	}
	push16(A);
	return 4;
}

CycleCount PHB(AddrMode mode)
{
	push(DBR);
	return 3;
}

CycleCount PHD(AddrMode mode)
{
	push16(D);
	return 4;
}

CycleCount PHK(AddrMode mode)
{
	push(K);
	return 3;
}

CycleCount PHP(AddrMode mode)
{
	push(P);
	return 3;
}

CycleCount PHX(AddrMode mode)
{
	if (isEX())
	{
		push(XL());
		return 3;
	}
	push16(X);
	return 4;
}

CycleCount PHY(AddrMode mode)
{
	if (isEX())
	{
		push(YL());
		return 3;
	}
	push16(Y);
	return 4;
}

CycleCount PLA(AddrMode mode)
{
	if (isEM())
	{
		setAL(pull());
		return 3;
	}
	A = pull16();
	return 4;
}

CycleCount PLB(AddrMode mode)
{
	DBR = pull();
	return 3;
}

CycleCount PLD(AddrMode mode)
{
	D = pull16();
	return 4;
}

CycleCount PLP(AddrMode mode)
{
	P = pull();
	return 3;
}

CycleCount PLX(AddrMode mode)
{
	if (isEX())
	{
		setXL(pull());
		return 3;
	}
	X = pull16();
	return 4;
}

CycleCount PLY(AddrMode mode)
{
	if (isEX())
	{
		setYL(pull());
		return 3;
	}
	Y = pull16();
	return 4;
}

CycleCount REP(AddrMode mode)
{
	P &= ~(read(PC));
	PC++;
	return 3;
}

CycleCount ROL(AddrMode mode)
{
	if (mode == adr_A)
	{
		if (isEM())
		{
			u8 temp_c = FLC();
			if (AL() & 0x80) setF(fc);
			else clrF(fc);
			setAL(AL() << 1);
			setAL(AL() | temp_c);
			updateZero8(AL());
			updateNegative8(AL());
		}
		else
		{
			u8 temp_c = FLC();
			if (A & 0x8000) setF(fc);
			else clrF(fc);
			A <<= 1;
			A |= temp_c;
			updateZero16(A);
			updateNegative16(A);
		}

		return 2;
	}
	
	u32 val_addr = fetchOperand_ReadModifyWrite(mode, FLM());
	if (isEM())
	{
		u8 temp_c = FLC();
		u8 val = read(val_addr);
		if (val & 0x80) setF(fc);
		else clrF(fc);
		val <<= 1;
		val |= temp_c;
		updateZero8(val);
		updateNegative8(val);
		write(val_addr, val);
	}
	else
	{
		u8 temp_c = FLC();
		u16 val = read16(val_addr);
		if (val & 0x8000) setF(fc);
		else clrF(fc);
		val <<= 1;
		val |= temp_c;
		updateZero16(val);
		updateNegative16(val);
		write16(val_addr, val);
	}

	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount ROR(AddrMode mode)
{
	if (mode == adr_A)
	{
		if (isEM())
		{
			u8 temp_c = FLC();
			if (AL() & 0x01) setF(fc);
			else clrF(fc);
			setAL(AL() >> 1);
			setAL(AL() | (temp_c << 7));
			updateZero8(AL());
			updateNegative8(AL());
		}
		else
		{
			u8 temp_c = FLC();
			if (A & 0x0001) setF(fc);
			else clrF(fc);
			A >>= 1;
			A |= (temp_c << 15);
			updateZero16(A);
			updateNegative16(A);
		}

		return 2;
	}
	
	u32 val_addr = fetchOperand_ReadModifyWrite(mode, FLM());
	if (isEM())
	{
		u8 temp_c = FLC();
		u8 val = read(val_addr);
		if (val & 0x01) setF(fc);
		else clrF(fc);
		val >>= 1;
		val |= (temp_c << 7);
		updateZero8(val);
		updateNegative8(val);
		write(val_addr, val);
	}
	else
	{
		u8 temp_c = FLC();
		u16 val = read16(val_addr);
		if (val & 0x0001) setF(fc);
		else clrF(fc);
		val >>= 1;
		val |= (temp_c << 15);
		updateZero16(val);
		updateNegative16(val);
		write16(val_addr, val);
	}

	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount RTI(AddrMode mode)
{
	P = pull();
	PC = pull16();
	if (!E) K = pull();
}

CycleCount RTL(AddrMode mode)
{
	PC = pull16();
	K = pull();
	PC++;
	return 6;
}

CycleCount RTS(AddrMode mode)
{
	PC = pull16();
	PC++;
	return 6;
}

CycleCount SBC(AddrMode mode)
{
	u16 addend = fetchOperand_Load(mode, FLM());
	addend ^= 0xffff;

	if (isEM())
	{
		addend &= 0xff;
		u8 old_al = AL();
		u8 al = old_al + addend + FLC();

		updateCarry8(old_al, al);
		updateZero8(al);
		updateNegative8(al);
		updateOverflow8(old_al, al);

		setAL(al);
	}
	else
	{
		u16 old_a = A;
		A += addend + FLC();

		updateCarry16(old_a, A);
		updateZero16(A);
		updateNegative16(A);
		updateOverflow16(old_a, A);
	}

	return getCycles_Load_Store(mode, FLM());
}

CycleCount SEP(AddrMode mode)
{
	P |= read(PC);
	PC++;
	return 3;
}

CycleCount SEC(AddrMode mode)
{
	setF(fc);
	return 2;
}

CycleCount SED(AddrMode mode)
{
	setF(fd);
	return 2;
}

CycleCount SEI(AddrMode mode)
{
	setF(fi);
	return 2;
}

CycleCount STA(AddrMode mode)
{
	u32 address = fetchOperand_Store(mode, FLM());

	if (isEM())
	{
		write(address, AL());
	}
	else
	{
		write16(address, A);
	}

	return getCycles_Load_Store(mode, FLM());
}

CycleCount STP(AddrMode mode)
{
	stopped = true;
	return 1;
}

CycleCount STX(AddrMode mode)
{
	u32 address = fetchOperand_Store(mode, FLX());

	if (isEX())
	{
		write(address, XL());
	}
	else
	{
		write16(address, X);
	}

	return getCycles_Load_Store(mode, FLX());
}

CycleCount STY(AddrMode mode)
{
	u32 address = fetchOperand_Store(mode, FLX());

	if (isEX())
	{
		write(address, YL());
	}
	else
	{
		write16(address, Y);
	}

	return getCycles_Load_Store(mode, FLX());
}

CycleCount STZ(AddrMode mode)
{
	u32 address = fetchOperand_Store(mode, FLM());

	if (isEM())
	{
		write(address, 0);
	}
	else
	{
		write16(address, 0);
	}

	return getCycles_Load_Store(mode, FLM());
}

CycleCount TAX(AddrMode mode)
{
	if (isEX())
	{
		setXL(AL());
		return 2;
	}
	X = A;
	return 2;
}

CycleCount TAY(AddrMode mode)
{
	if (isEX())
	{
		setYL(AL());
		return 2;
	}
	Y = A;
	return 2;
}

CycleCount TCD(AddrMode mode)
{
	D = A;
	return 2;
}

CycleCount TCS(AddrMode mode)
{
	S = A;
	return 2;
}

CycleCount TDC(AddrMode mode)
{
	A = D;
	return 2;
}

CycleCount TRB(AddrMode mode)
{
	// TODO: implement this instruction
	u32 address = fetchOperand_ReadModifyWrite(mode, FLM());
	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount TSB(AddrMode mode)
{
	// TODO: implement this instruction
	u32 address = fetchOperand_ReadModifyWrite(mode, FLM());
	return getCycles_ReadModifyWrite(mode, FLM());
}

CycleCount TSC(AddrMode mode)
{
	A = S;
	return 2;
}

CycleCount TSX(AddrMode mode)
{
	if (isEX())
	{
		setXL(S & 0xff);
		return 2;
	}
	X = S;
	return 2;
}

CycleCount TXA(AddrMode mode)
{
	if (isEM())
	{
		setAL(XL());
		return 2;
	}
	A = X;
	return 2;
}

CycleCount TXS(AddrMode mode)
{
	if (E)
	{
		S = XL() | 0x100;
		return 2;
	}
	S = X;
	return 2;
}

CycleCount TXY(AddrMode mode)
{
	if (isEX())
	{
		setYL(XL());
		return 2;
	}
	Y = X;
	return 2;
}

CycleCount TYA(AddrMode mode)
{
	if (isEM())
	{
		setAL(YL());
		return 2;
	}
	A = Y;
	return 2;
}

CycleCount TYX(AddrMode mode)
{
	if (isEX())
	{
		setXL(YL());
		return 2;
	}
	X = Y;
	return 2;
}

CycleCount WAI(AddrMode mode)
{
	waiting = true;
	return 2;
}

CycleCount WDM(AddrMode mode)
{
	PC++;
	return 2;
}

CycleCount XBA(AddrMode mode)
{
	u8 temp = AH();
	A <<= 8;
	A |= temp;
	return 3;
}

CycleCount XCE(AddrMode mode)
{
	u8 temp_e = E;
	E = FLC();
	setF(temp_e ? fc : 0);
	return 2;
}



// Opcode -> Instruction
CycleCount (*instructions[256]) (AddrMode) = {
	BRK, ORA, COP, ORA, TSB, ORA, ASL, ORA, PHP, ORA, ASL, PHD, TSB, ORA, ASL, ORA,
	BPL, ORA, ORA, ORA, TRB, ORA, ASL, ORA, CLC, ORA, INC, TCS, TRB, ORA, ASL, ORA,
	JSR, AND, JSL, AND, BIT, AND, ROL, AND, PLP, AND, ROL, PLD, BIT, AND, ROL, AND,
	BMI, AND, AND, AND, BIT, AND, ROL, AND, SEC, AND, DEC, TSC, BIT, AND, ROL, AND,
	RTI, EOR, WDM, EOR, MVP, EOR, LSR, EOR, PHA, EOR, LSR, PHK, JMP, EOR, LSR, EOR,
	BVC, EOR, EOR, EOR, MVN, EOR, LSR, EOR, CLI, EOR, PHY, TCD, JMP, EOR, LSR, EOR,
	RTS, ADC, PER, ADC, STZ, ADC, ROR, ADC, PLA, ADC, ROR, RTL, JMP, ADC, ROR, ADC,
	BVS, ADC, ADC, ADC, STZ, ADC, ROR, ADC, SEI, ADC, PLY, TDC, JMP, ADC, ROR, ADC,
	BRA, STA, BRL, STA, STY, STA, STX, STA, DEY, BIT, TXA, PHB, STY, STA, STX, STA,
	BCC, STA, STA, STA, STY, STA, STX, STA, TYA, STA, TXS, TXY, STZ, STA, STZ, STA,
	LDY, LDA, LDX, LDA, LDY, LDA, LDX, LDA, TAY, LDA, TAX, PLB, LDY, LDA, LDX, LDA,
	BCS, LDA, LDA, LDA, LDY, LDA, LDX, LDA, CLV, LDA, TSX, TYX, LDY, LDA, LDX, LDA,
	CPY, CMP, REP, CMP, CPY, CMP, DEC, CMP, INY, CMP, DEX, WAI, CPY, CMP, DEC, CMP,
	BNE, CMP, CMP, CMP, PEI, CMP, DEC, CMP, CLD, CMP, PHX, STP, JML, CMP, DEC, CMP,
	CPX, SBC, SEP, SBC, CPX, SBC, INC, SBC, INX, SBC, NOP, XBA, CPX, SBC, INC, SBC,
	BEQ, SBC, SBC, SBC, PEA, SBC, INC, SBC, SED, SBC, PLX, XCE, JSR, SBC, INC, SBC
};



const AddrMode addr_modes[] = {
	adr_s, adr_idx, adr_s, adr_ds, adr_d, adr_d, adr_d, adr_ild, adr_s, adr_imm, adr_A, adr_s, adr_a, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_d, adr_dx, adr_dx, adr_ildy, adr_i, adr_ay, adr_A, adr_i, adr_a, adr_ax, adr_ax, adr_alx,
	adr_a, adr_idx, adr_al, adr_ds, adr_d, adr_d, adr_d, adr_ild, adr_s, adr_imm, adr_A, adr_s, adr_a, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_dx, adr_dx, adr_dx, adr_ildy, adr_i, adr_ay, adr_A, adr_i, adr_ax, adr_ax, adr_ax, adr_alx,
	adr_s, adr_idx, adr_i, adr_ds, adr_xyc, adr_d, adr_d, adr_ild, adr_s, adr_imm, adr_A, adr_s, adr_a, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_xyc, adr_dx, adr_dx, adr_ildy, adr_i, adr_ay, adr_s, adr_i, adr_al, adr_ax, adr_ax, adr_alx,
	adr_s, adr_idx, adr_s, adr_ds, adr_d, adr_d, adr_d, adr_ild, adr_s, adr_imm, adr_A, adr_s, adr_ia, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_dx, adr_dx, adr_dx, adr_ildy, adr_i, adr_ay, adr_s, adr_i, adr_iax, adr_ax, adr_ax, adr_alx,
	adr_r, adr_idx, adr_rl, adr_ds, adr_d, adr_d, adr_d, adr_ild, adr_i, adr_imm, adr_i, adr_s, adr_a, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_dx, adr_dx, adr_dy, adr_ildy, adr_i, adr_ay, adr_i, adr_i, adr_a, adr_ax, adr_ax, adr_alx,
	adr_imm, adr_idx, adr_imm, adr_ds, adr_d, adr_d, adr_d, adr_ild, adr_i, adr_imm, adr_i, adr_s, adr_a, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_dx, adr_dx, adr_dy, adr_ildy, adr_i, adr_ay, adr_i, adr_i, adr_ax, adr_ax, adr_ay, adr_alx,
	adr_imm, adr_idx, adr_imm, adr_ds, adr_d, adr_d, adr_d, adr_ild, adr_i, adr_imm, adr_i, adr_i, adr_a, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_s, adr_dx, adr_dx, adr_ildy, adr_i, adr_ay, adr_s, adr_i, adr_ial, adr_ax, adr_ax, adr_alx,
	adr_imm, adr_idx, adr_imm, adr_ds, adr_d, adr_d, adr_d, adr_ild, adr_i, adr_imm, adr_i, adr_i, adr_a, adr_a, adr_a, adr_al,
	adr_r, adr_idy, adr_id, adr_dsy, adr_s, adr_dx, adr_dx, adr_ildy, adr_i, adr_ay, adr_s, adr_i, adr_iax, adr_ax, adr_ax, adr_alx
};



CycleCount execute()
{
	// STP / WAI
	if (stopped) return 1;
	if (waiting)
	{
		if (pending_abr || pending_irq || pending_nmi)
		{
			waiting = false;
		}
		return 1;
	}

	// Interrupts

	
	if (pending_abr)
	{
		pending_abr = false;
		if (!E) push(K);
		push16(PC);
		push(P);
		K = 0;
		PC = abr_vec[E];
		PC = read16(PC);
		return 6 - E;
	}

	if (pending_nmi)
	{
		pending_nmi = false;
		if (!E) push(K);
		push16(PC);
		push(P);
		K = 0;
		PC = nmi_vec[E];
		PC = read16(PC);
		return 6 - E;
	}

	if (pending_irq && FLI() == 0)
	{
		pending_irq = false;
		if (!E) push(K);
		push16(PC);
		push(P);
		K = 0;
		PC = irq_vec[E];
		PC = read16(PC);
		return 6 - E;
	}
	

	// MVN / MVP
	if (mvn_active)
	{
		u32 start_addr = X | (mv_start << 16);
		u32 end_addr = Y | (mv_end << 16);
		u8 val = read(start_addr);
		write(end_addr, val);
		X++;
		Y++;
		A--;
		mvn_active = !(A == 0xffff);
		return 7;
	}
	if (mvp_active)
	{
		u32 start_addr = X | (mv_start << 16);
		u32 end_addr = Y | (mv_end << 16);
		u8 val = read(start_addr);
		write(end_addr, val);
		X--;
		Y--;
		A--;
		mvp_active = !(A == 0xffff);
		return 7;
	}

	// Normal code execution
	opcode = readK(PC);
	PC++;

	AddrMode mode = addr_modes[opcode];
	u8 cycles = instructions[opcode](mode);

	//printf("%s ", instruction_text[opcode]);
	//printf("%d\n", mode);

	return cycles;
}

