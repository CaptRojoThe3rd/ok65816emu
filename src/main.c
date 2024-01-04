
#include "include.h"
#include "scancodes.h"
#include "renderer.c"
#include "cpu.c"
#include "gpu.c"
#include "via.c"



u64 getPS2Scancode(u8 sdlScancode, bool released)
{
	if (released)
	{
		switch (sdlScancode)
		{
			case SDL_SCANCODE_PRINTSCREEN:
				return 0x12f0e07cf0e0;

			case SDL_SCANCODE_PAUSE:
				return 0x77f014f0;

			default:
				return 0xf0 | (scancodeConvertTable[sdlScancode] << 8);
		}
	}

	switch (sdlScancode)
	{
		case SDL_SCANCODE_PRINTSCREEN:
			return 0x7ce012e0;

		case SDL_SCANCODE_PAUSE:
			return 0xe17714e1;

		default:
			return scancodeConvertTable[sdlScancode];
	}
}


// Main Function
int main(int argc, char *argv[])
{
	// Parse arguments
	char bios_rom_path[1024];
	char os0_rom_path[1024];
	char os1_rom_path[1024];
	bool bios_rom_specified = false;
	bool os0_rom_specified = false;
	bool os1_rom_specified = false;

	for (u8 i = 1; i < argc; i += 2)
	{
		if (strcmp(argv[i], "--bios") == 0)
		{
			strcpy(bios_rom_path, argv[i + 1]);
			bios_rom_specified = true;
		}
		if (strcmp(argv[i], "--os0") == 0)
		{
			strcpy(os0_rom_path, argv[i + 1]);
			os0_rom_specified = true;
		}
		if (strcmp(argv[i], "--os1") == 0)
		{
			strcpy(os1_rom_path, argv[i + 1]);
			os1_rom_specified = true;
		}
	}


	memset(cpu_mem, 0, 0x1000000);
	/*
	srand(time(NULL));
	for (u32 i = 0; i < 0x1000000; i++)
	{
		cpu_mem[i] = rand() ^ i;
	}
	*/


	FILE *fptr;

	// Load BIOS ROM
	//printf("BIOS ROM: %s\n", bios_rom_path);
	fptr = fopen(bios_rom_path, "rb");
	u8 bios_rom[0x2000];
	fread(bios_rom, 0x2000, 1, fptr);
	fclose(fptr);
	for (u16 i = 0; i < 0x2000; i++)
	{
		cpu_mem[i + 0xe000] = bios_rom[i];
	}

	// Load OS 0 ROM
	if (os0_rom_specified)
	{
		fptr = fopen(os0_rom_path, "rb");
		u8 os0_rom[0x20000];
		fread(os0_rom, 0x20000, 1, fptr);
		fclose(fptr);
		for (u32 i = 0; i < 0x20000; i++)
		{
			cpu_mem[i + 0xfe0000] = os0_rom[i];
		}
	}

	// Load OS 1 ROM
	if (os1_rom_specified)
	{
		fptr = fopen(os1_rom_path, "rb");
		u8 os1_rom[0x20000];
		fread(os1_rom, 0x20000, 1, fptr);
		fclose(fptr);
		for (u32 i = 0; i < 0x20000; i++)
		{
			cpu_mem[i + 0xfc0000] = os1_rom[i];
		}
	}

	if (!(os0_rom_specified || os1_rom_specified))
	{
		printf(ANSI_COLOR_YELLOW "Warning: " ANSI_COLOR_RESET "No OS ROM(s) specified! Use " ANSI_COLOR_CYAN "--os0 <file>" ANSI_COLOR_RESET " or " ANSI_COLOR_CYAN "--os1 <file>"
		ANSI_COLOR_RESET "\n");
	}
	if (!bios_rom_specified)
	{
		printf(ANSI_COLOR_RED "Error: " ANSI_COLOR_RESET "No BIOS ROM specified! Use " ANSI_COLOR_CYAN "--bios <file>\n" ANSI_COLOR_RESET);
		return 0;
	}


	/*
	SDL Init
	*/

	SDL_Init(SDL_INIT_EVERYTHING);
	Window display = createWindow("Super Cool 65816 Computer", 1280, 960, false);

	// Keyboard stuff
	bool keyboard_old[0x120];
	bool keyboard_current[0x120];
	bool keyboard_new[0x120];
	bool keyboard_newly_released[0x120];
	memset(keyboard_old, false, 0x120);
	memset(keyboard_current, false, 0x120);
	memset(keyboard_new, false, 0x120);
	memset(keyboard_newly_released, false, 0x120);
	u64 ps2_scancode_stream = 0;

	u32 frame_start_time = SDL_GetTicks();
	u32 frame_end_time = 0;
	f32 frame_delta_time;
	u16 fps;


	u8 quick_frames; // Number of frames where 83916 cycles are executed; every 3rd frame should execute 83917 cycles
	u32 cycle_count = 0;
	u32 cycle_limit;
	u8 cycles;

	// Reset vector
	PC = read16(0xfffc);
	K = 0;
	E = true;


	/*
	Main Loop
	*/

	bool running = true;
	while (running)
	{
		
		/*
		SDL Updates
		*/

		frame_start_time = SDL_GetTicks();

		// Events
		SDL_Event event;

		memcpy(keyboard_old, keyboard_current, 0x120);
		memset(keyboard_newly_released, false, 0x120);

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					running = false;
					break;

				case SDL_KEYDOWN:
					keyboard_current[event.key.keysym.scancode] = true;
					break;

				case SDL_KEYUP:
					keyboard_current[event.key.keysym.scancode] = false;
					keyboard_newly_released[event.key.keysym.scancode] = true;
					break;

				default:
					break;
			}

			switch (event.window.event)
			{
				default:
					break;
			}
		}

		memset(keyboard_new, false, 0x120);
		for (u16 i = 0; i < 0x120; i++)
		{
			keyboard_new[i] = !keyboard_old[i] && keyboard_current[i];
		}


		/*
		Execute 1 frame worth of CPU cycles.
		CPU runs at 12.5875 MHz, or 209791 and 2/3 cycles per frame.
		*/

		// Adjust cycle count variables
		if (cycle_count >= 209791) cycle_count -= (quick_frames < 2) ? 209791 : 209792;
		cycle_limit = (quick_frames < 2) ? 209791 : 209792;
		quick_frames++;
		if (quick_frames == 3) quick_frames = 0;

		// Randomize when keyboard events happen
		u16 key_scanline = rand() & 511;
		u16 key_pixel = rand() & 1023;
		if (key_pixel >= 800) key_pixel -= 800;

		
		// Check for any keyboard events
		for (u16 i = 0; i < 0x120; i++)
		{
			if (keyboard_new[i])
			{
				ps2_scancode_stream = getPS2Scancode(i, false);
				break;
			}
			if (keyboard_newly_released[i])
			{
				ps2_scancode_stream = getPS2Scancode(i, true);
				break;
			}
		}

		if (ps2_scancode_stream != 0)
		{
			ps2_input = ps2_scancode_stream & 0xff;
			ps2_scancode_stream >>= 8;
			keyboard_irq_flag = true;
		}

		// Begin execution loop
		while (cycle_count <= cycle_limit)
		{ 
			cycles = execute();
			cycle_count += cycles;
			pixel += (cycles * 2);
			pending_nmi = (old_pixel < 640 && pixel >= 640 && hBlank_NMI_enabled);

			if (pixel >= 800)
			{
				pixel -= 800;
				scanline++;
				pending_nmi = (scanline == 480 && vBlank_NMI_enabled);

				//if (scanline == 8) return 0;

				if (scanline == 525)
				{
					scanline = 0;
				}
			}
			
			old_pixel = pixel;
			old_scanline = scanline;
		}


		gpu_render(display);

		/*
		More SDL Updates
		*/

		SDL_RenderPresent(display.renderer);

		// Framerate and delta time
		frame_end_time = SDL_GetTicks();
		frame_delta_time = frame_end_time - frame_start_time;
		if (frame_delta_time < 16.666666)
		{
			frame_delta_time = 16.666666;
			SDL_Delay(16.666666 - frame_delta_time);
		}
		fps = 1000 / frame_delta_time;
		//printf("%d\n", fps);
	}

	return 0;
}
