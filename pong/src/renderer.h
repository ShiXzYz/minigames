#pragma once

#include <SDL3/SDL.h>

#include "ball.h"
#include "paddle.h"
#include "game.h"

void render_frame(SDL_Renderer *renderer, const Ball *ball, const Paddle *left_paddle,
                   const Paddle *right_paddle, const GameState *game, Uint64 ticks);
