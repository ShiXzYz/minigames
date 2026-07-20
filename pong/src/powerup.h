#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "ball.h"

typedef enum {
    POWERUP_SPEED_BOOST,
    POWERUP_SLOW_MOTION,
    POWERUP_GIANT_PADDLE,
    POWERUP_TINY_PADDLE,
    POWERUP_FIREBALL,
    POWERUP_CURVE_BALL,
    POWERUP_MAGNET,
    POWERUP_INVERT_CONTROLS,
    POWERUP_INVISIBLE_BALL,
    POWERUP_SHIELD,
    POWERUP_SPLIT_SHOT,
    POWERUP_HOLD_SHOT,
    POWERUP_CHAOS_BOX,
    POWERUP_COUNT
} PowerupType;

typedef enum {
    TOUCH_NONE,
    TOUCH_LEFT,
    TOUCH_RIGHT
} TouchSide;

#define MAX_ACTIVE_EFFECTS 8

typedef struct {
    PowerupType type;
    bool affects_left;
    Uint64 expires_at;
    bool in_use;
    float extra;
} ActiveEffect;

typedef struct {
    SDL_FRect rect;
    bool active;
} PowerupPickup;

typedef struct {
    ActiveEffect effects[MAX_ACTIVE_EFFECTS];
    int left_shields;
    int right_shields;
    PowerupPickup pickup;
    Uint64 next_spawn_at;
    TouchSide last_touch;
    PowerupType last_activated_type;
    Uint64 last_activated_at;
    bool has_last_activated;
    bool pending_hold_shot;
    bool pending_hold_shot_is_left;
    bool pending_split_shot;
} PowerupState;

void powerup_reset_match(PowerupState *state, Uint64 ticks);
void powerup_reset_round(PowerupState *state, Uint64 ticks);
void powerup_update(PowerupState *state, const Ball *ball, Uint64 ticks);
void powerup_note_touch(PowerupState *state, TouchSide side);
void powerup_grant(PowerupState *state, PowerupType type, bool picker_is_left, Uint64 ticks);

bool powerup_is_active(const PowerupState *state, PowerupType type, bool for_left, Uint64 ticks);
bool powerup_is_active_any(const PowerupState *state, PowerupType type, Uint64 ticks);
float powerup_get_extra(const PowerupState *state, PowerupType type, Uint64 ticks);
bool powerup_consume_shield(PowerupState *state, bool for_left);
