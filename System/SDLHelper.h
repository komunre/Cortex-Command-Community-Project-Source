#ifndef _SDLHELPER_
#define _SDLHELPER_
/// <summary>
/// This header includes the SDL base header and adds some very useful
/// vector operators to SDL_Rect and SDL_Point
/// </summary>

#include <SDL2/SDL.h>
// #include "SDLTexture.h"

/// <summary>
/// Shift the Position of an SDL_Rect by an SDL_Point
/// </summary>
SDL_Rect operator+(const SDL_Rect &lhs, const SDL_Point &rhs);

SDL_Rect operator*(const SDL_Rect &lhs, const int rhs);

SDL_Rect operator*(const SDL_Rect &lhs, const float rhs);

SDL_Rect operator*(const SDL_Rect &lhs, const double rhs);

/// <summary>
/// Shift the Position of an SDL_Rect by -SDL_Point
/// </summary>
SDL_Rect operator-(const SDL_Rect &lhs, const SDL_Point &rhs);

/// <summary>
/// Add two SDL_Points
/// </summary>
SDL_Point operator+(const SDL_Point &lhs, const SDL_Point &rhs);

/// <summary>
/// Subtract two SDL_Points
/// </summary>
SDL_Point operator-(const SDL_Point &lhs, const SDL_Point &rhs);

/// <summary>
/// Deleter structure for the sdl pointers for use in smartpointers
/// </summary>
struct sdl_deleter {
	void operator()(SDL_Surface *p) { SDL_FreeSurface(p); }
	void operator()(SDL_PixelFormat *p) { SDL_FreeFormat(p); }
	void operator()(SDL_Texture *p) { SDL_DestroyTexture(p); }
	void operator()(SDL_Renderer *p) { SDL_DestroyRenderer(p); }
	void operator()(SDL_Window *p) { SDL_DestroyWindow(p); }
};

const SDL_BlendMode BLENDMODE_SCREEN = SDL_ComposeCustomBlendMode(
    SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR, SDL_BLENDOPERATION_ADD,
    SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
#endif
