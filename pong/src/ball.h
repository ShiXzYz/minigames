#pragma once

#include <SDL3/SDL.h>

typedef struct {
    SDL_FRect rect;
    float vx;
    float vy;
} Ball;

void reset_ball(Ball *ball, float direction);
void ball_update(Ball *ball, float deltaTime);
