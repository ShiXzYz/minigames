#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "powerup.h"

typedef struct {
    SDL_FRect rect;
    float vel;          // this frame's vertical speed, used for the spin-on-hit effect
    bool was_colliding;  // ball/paddle contact state from last frame (edge-triggers collisions)
} Paddle;

void paddle_update(Paddle *paddle, const bool *keystate, SDL_Scancode up_key, SDL_Scancode down_key,
                    float deltaTime, const PowerupState *powerups, bool is_left, Uint64 ticks);
