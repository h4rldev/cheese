#ifndef CHEESE_SDL3_RENDERER_H
#define CHEESE_SDL3_RENDERER_H

/***********************************/

#include <SDL3/SDL.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Create an SDL3-backed renderer.
 * @details SDL3 owns the window and its `SDL_Renderer`, so there is no device
 * handle to take: pass the active `SDL_Renderer`. Shapes go through
 * `SDL_RenderGeometry` triangle lists (rounded rects, borders, arcs, glyph
 * quads) and images through `SDL_RenderTexture`.
 *
 * @param renderer The active SDL3 renderer.
 * @param arena The persistent arena the adapter state lives in.
 *
 * @return The renderer vtable, or a zeroed one on failure.
 */
cheese_renderer_t cheese_create_sdl3_renderer(SDL_Renderer *renderer,
                                              arena_t *arena);

#endif // !CHEESE_SDL3_RENDERER_H
