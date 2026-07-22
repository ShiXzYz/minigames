#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "powerup.h"

typedef struct {
    SDL_FRect rect;
    float vel; // this frame's vertical speed, used for the spin-on-hit effect
} Paddle;

void paddle_update(Paddle *paddle, bool pressed_up, bool pressed_down, float speed_mult,
                    float deltaTime, const PowerupState *powerups, bool is_left, Uint64 ticks);
