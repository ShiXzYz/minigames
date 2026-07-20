#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#define MAX_BALLS 2

typedef struct {
    SDL_FRect rect;
    float vx;
    float vy;
    bool active;
    bool was_colliding_left;
    bool was_colliding_right;
    bool held;
    bool held_by_left;
    Uint64 held_since;
} Ball;

void reset_ball(Ball *ball, float direction);
void ball_update(Ball *ball, float deltaTime, float speed_scale);
