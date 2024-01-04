
#include "include.h"


typedef struct
{
	SDL_Window *window;
	SDL_Renderer *renderer;
} Window;

typedef struct
{
	u8 red;
	u8 green;
	u8 blue;
} RGBColor;



/*
Creates a window with the specified name and size.
*/
Window createWindow(char *name, u16 size_x, u16 size_y, bool resizable)
{
	Window new_window;

	new_window.window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size_x, size_y, resizable ? SDL_WINDOW_RESIZABLE : 0);
	new_window.renderer = SDL_CreateRenderer(new_window.window, -1, SDL_RENDERER_ACCELERATED);

	return new_window;
}


/*
Draws a single pixel to the window.
*/
void drawPixel(SDL_Renderer *renderer, RGBColor color, u16 x, u16 y)
{
	SDL_SetRenderDrawColor(renderer, color.red, color.green, color.blue, 255);
	SDL_RenderDrawPoint(renderer, x, y);
}

/*
Creates a rect.
*/
SDL_Rect createRect(u16 x, u16 y, u16 w, u16 h)
{
	SDL_Rect new_rect;
	new_rect.x = x;
	new_rect.y = y;
	new_rect.w = w;
	new_rect.h = h;
	return new_rect;
}

