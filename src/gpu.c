
#include "include.h"

/*
GPU stuff
*/



/*
GPU State
*/

u32 pixel;
u16 scanline;
u16 old_pixel;
u16 old_scanline;

bool vBlank_NMI_enabled = false;
bool hBlank_NMI_enabled = false;
bool rendering_enabled = true;

u8 blank_color;

u8 surface_bytes[640 * 480 * 4];



/*
CPU Interface Functions
*/

u8 gpuIORead(u32 addr)
{
	switch (addr & 0x07)
	{
		case 0x4:
			return pixel & 0xff;
		
		case 0x5:
			return pixel >> 8;

		case 0x6:
			return scanline & 0xff;

		case 0x7:
			return scanline >> 8;
	}

	return 0;
}

void gpuIOWrite(u32 addr, u8 val)
{
	switch (addr & 0x07)
	{
		case 0x0:
		case 0x2:
			vBlank_NMI_enabled = val & 0x01;
			hBlank_NMI_enabled = (val & 0x02) >> 1;
			rendering_enabled = val >> 7;
			break;

		case 0x1:
		case 0x3:
			blank_color = val;
	}
}

bool getBlankingStatus()
{
	return (scanline >= 480 || pixel >= 640 || !rendering_enabled);
}


RGBColor convertPixel(u8 val)
{
	RGBColor color;
	u8 pixel_conversions[4] = {0, 85, 170, 255};

	color.red = pixel_conversions[val & 0b11];
	color.green = pixel_conversions[(val & 0b1100) >> 2];
	color.blue = pixel_conversions[(val & 0b110000) >> 4];
	return color;
}

void convertComputerPixelsToRGBArray()
{
	RGBColor current_color;
	u32 index_x;
	u32 index_y;
	
	for (u16 y = 0; y < 480; y++)
	{
		for (u16 x = 0; x < 640; x++)
		{
			current_color = convertPixel(cpu_mem[0x080000 + x + (y * 1024)]);
			index_x = x * 3;
			index_y = y * 3;
			surface_bytes[index_x + (index_y * 640)] = current_color.red;
			surface_bytes[index_x + (index_y * 640) + 1] = current_color.green;
			surface_bytes[index_x + (index_y * 640) + 2] = current_color.blue;
		}
	}
}


void gpu_render(Window window)
{
	if (rendering_enabled)
	{
		convertComputerPixelsToRGBArray();

		SDL_Surface *screen_surface = SDL_CreateRGBSurfaceFrom((void*) surface_bytes, 640, 480, 24, 640 * 3, 0x0000ff, 0x00ff00, 0xff0000, 0);
		SDL_Texture *screen_texture = SDL_CreateTextureFromSurface(window.renderer, screen_surface);
		SDL_Rect screen_rect = createRect(0, 0, 1280, 960);

		SDL_RenderClear(window.renderer);
		SDL_RenderCopy(window.renderer, screen_texture, NULL, &screen_rect);

		SDL_DestroyTexture(screen_texture);
	}
	else
	{
		RGBColor blank = convertPixel(blank_color);
		SDL_SetRenderDrawColor(window.renderer, blank.red, blank.green, blank.blue, 255);
		SDL_RenderClear(window.renderer);
	}
}


