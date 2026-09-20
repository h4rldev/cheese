#ifndef CHEESE_RAYLIB_RENDERER_H
#define CHEESE_RAYLIB_RENDERER_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <cheese/types.h>

/***********************************/

/**
 * @brief Create a raylib-backed renderer.
 * @details raylib owns its own window and GPU context, so there is no device
 * handle to pass: the renderer draws into whatever context raylib has active
 * (`InitWindow` must already have been called). Textures are uploaded through
 * `LoadTextureFromImage` and tracked in a small id registry.
 *
 * @param arena The persistent arena the adapter state lives in.
 *
 * @return The renderer vtable, or a zeroed one on failure.
 */
cheese_renderer_t cheese_create_raylib_renderer(arena_t *arena);

#endif // !CHEESE_RAYLIB_RENDERER_H
