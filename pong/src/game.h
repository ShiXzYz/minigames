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
    GAME_PLAYING,
    GAME_PAUSED
} GameStateKind;

typedef enum {
    MODE_CLASSIC,
    MODE_POWER_PLAY
} GameMode;

typedef struct {
    int left_score;
    int right_score;
    GameStateKind state;
    GameStateKind paused_from;
    Uint64 state_entered_at;
    GameMode mode;
    bool prev_up;
    bool prev_down;
    bool prev_confirm;
    bool prev_escape;
    bool pause_confirm_selected;
    PowerupState powerups;
    Ball balls[MAX_BALLS];
} GameState;

void game_update(GameState *game, Paddle *left_paddle, Paddle *right_paddle,
                  const bool *keystate, float deltaTime, Uint64 ticks);
