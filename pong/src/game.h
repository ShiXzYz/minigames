#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "ball.h"
#include "paddle.h"
#include "powerup.h"

typedef enum {
    GAME_MENU,
    GAME_MODE_SELECT,
    GAME_WAITING,
    GAME_PLAYING
} GameStateKind;

typedef enum {
    MODE_CLASSIC,
    MODE_POWER_PLAY
} GameMode;

typedef struct {
    int left_score;
    int right_score;
    GameStateKind state;
    Uint64 state_entered_at;
    GameMode mode;
    bool prev_up;
    bool prev_down;
    bool prev_confirm;
    PowerupState powerups;
} GameState;

void game_update(GameState *game, Ball *ball, Paddle *left_paddle, Paddle *right_paddle,
                  const bool *keystate, float deltaTime, Uint64 ticks);
