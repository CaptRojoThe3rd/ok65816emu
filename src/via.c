
#include "include.h"

u8 DDRA;
u8 DDRB;

u16 T1Counter;
u16 T2Counter;
u16 T1Latch;

bool T1enabled = false;
bool T2enabled = false;

u8 ACR;


u8 viaRead(u32 addr)
{
	switch (addr & 0xf)
	{
		case 0x0:
			return 0xc0 & ~DDRB;
		
		case 0x1:
			return 0x00 & ~DDRA;

		case 0x2:
			return DDRB;

		case 0x3:
			return DDRA;

		
	}
}

void viaWrite(u32 addr, u8 val)
{
	switch (addr & 0xf)
	{
		case 0x2:
			DDRB = val;
			return;

		case 0x3:
			DDRA = val;
			return;

		case 0x4:
			T1Latch &= 0xff00;
			T1Latch |= val;
			T1Counter &= 0xff00;
			T1Counter |= val;
			return;

		case 0x5:
			T1Counter &= 0x00ff;
			T1Counter |= (val << 8);
			T1enabled = true;
			return;

		case 0x6:
			T1Latch &= 0xff00;
			T1Latch |= val;
			return;

		case 0x7:
			T1Latch &= 0x00ff;
			T1Latch |= (val << 8);
			return;

		case 0xb:
			ACR = val;
			return;

	}
}


void Clock()
{
	
}
