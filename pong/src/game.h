#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "ball.h"
#include "paddle.h"
#include "powerup.h"

typedef enum {
    GAME_MENU,
    GAME_PLAY_TYPE_SELECT,
    GAME_PLAYER_COUNT_SELECT,
    GAME_DIFFICULTY_SELECT,
    GAME_LAN_ROLE_SELECT,
    GAME_LAN_HOST_WAITING,
    GAME_LAN_CLIENT_SEARCHING,
    GAME_MODE_SELECT,
    GAME_WIN_SCORE_SELECT,
    GAME_WAITING,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameStateKind;

typedef enum {
    MODE_CLASSIC,
    MODE_POWER_PLAY
} GameMode;

typedef enum {
    PLAY_TYPE_LOCAL,
    PLAY_TYPE_LAN,
    PLAY_TYPE_COUNT
} PlayType;

typedef enum {
    AI_EASY,
    AI_NORMAL,
    AI_HARD,
    AI_DIFFICULTY_COUNT
} AiDifficulty;

typedef struct {
    int left_score;
    int right_score;
    GameStateKind state;
    GameStateKind paused_from;
    Uint64 state_entered_at;
    GameMode mode;
    int win_score_index;
    int win_score;
    bool prev_up;
    bool prev_down;
    bool prev_left;
    bool prev_right;
    bool prev_confirm;
    bool prev_escape;
    bool pause_confirm_selected;
    Uint64 pause_confirm_changed_at;
    Uint64 mode_changed_at;
    Uint64 win_score_changed_at;
    int play_type_index;
    Uint64 play_type_changed_at;
    bool two_player_selected;
    Uint64 player_count_changed_at;
    bool lan_role_host_selected;
    Uint64 lan_role_changed_at;
    bool is_lan_host_match;
    bool vs_ai;
    int ai_difficulty_index;
    AiDifficulty ai_difficulty;
    Uint64 difficulty_changed_at;
    float ai_target_y;
    Uint64 ai_reaction_at;
    PowerupState powerups;
    Ball balls[MAX_BALLS];
} GameState;

void game_update(GameState *game, Paddle *left_paddle, Paddle *right_paddle,
                  const bool *keystate, float deltaTime, Uint64 ticks);
void game_start_lan_match(GameState *game, Uint64 ticks);
