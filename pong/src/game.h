#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "ball.h"
#include "paddle.h"

typedef struct {
    int left_score;
    int right_score;
    bool started;
} GameState;

// Runs one frame of simulation: paddle movement, the start gate, ball physics,
// scoring, and paddle collisions. `keystate` is the array from SDL_GetKeyboardState.
void game_update(GameState *game, Ball *ball, Paddle *left_paddle, Paddle *right_paddle,
                  const bool *keystate, float deltaTime);
