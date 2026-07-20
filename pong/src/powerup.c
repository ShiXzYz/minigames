#include "powerup.h"
#include "constants.h"

static const Uint64 spawn_interval_ms = 7000;
static const float pickup_box_size = 40.0f;

static Uint64 effect_duration_ms(PowerupType type) {
    return (type == POWERUP_FIREBALL) ? 8000 : 5000;
}

static int find_effect_slot(PowerupState *state, PowerupType type, bool affects_left) {
    for (int i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        if (state->effects[i].in_use && state->effects[i].type == type && state->effects[i].affects_left == affects_left) {
            return i;
        }
    }
    for (int i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        if (!state->effects[i].in_use) {
            return i;
        }
    }
    return -1;
}

void powerup_grant(PowerupState *state, PowerupType type, bool picker_is_left, Uint64 ticks) {
    if (type == POWERUP_CHAOS_BOX) {
        PowerupType reroll = (PowerupType)SDL_rand(POWERUP_COUNT - 1);
        powerup_grant(state, reroll, picker_is_left, ticks);
        return;
    }

    state->last_activated_type = type;
    state->last_activated_at = ticks;
    state->has_last_activated = true;

    if (type == POWERUP_SHIELD) {
        if (picker_is_left) {
            state->left_shields++;
        } else {
            state->right_shields++;
        }
        return;
    }

    bool affects_left = picker_is_left;
    if (type == POWERUP_TINY_PADDLE || type == POWERUP_INVERT_CONTROLS) {
        affects_left = !picker_is_left;
    }

    int slot = find_effect_slot(state, type, affects_left);
    if (slot < 0) {
        return;
    }

    state->effects[slot].type = type;
    state->effects[slot].affects_left = affects_left;
    state->effects[slot].expires_at = ticks + effect_duration_ms(type);
    state->effects[slot].in_use = true;
    state->effects[slot].extra = (type == POWERUP_CURVE_BALL) ? ((SDL_rand(2) == 0) ? 1.0f : -1.0f) : 0.0f;
}

void powerup_reset_round(PowerupState *state, Uint64 ticks) {
    for (int i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        state->effects[i].in_use = false;
    }
    state->pickup.active = false;
    state->next_spawn_at = ticks + spawn_interval_ms;
    state->last_touch = TOUCH_NONE;
}

void powerup_reset_match(PowerupState *state, Uint64 ticks) {
    state->left_shields = 0;
    state->right_shields = 0;
    powerup_reset_round(state, ticks);
}

void powerup_note_touch(PowerupState *state, TouchSide side) {
    if (side != TOUCH_NONE) {
        state->last_touch = side;
    }
}

void powerup_update(PowerupState *state, const Ball *ball, Uint64 ticks) {
    if (!state->pickup.active && ticks >= state->next_spawn_at) {
        float margin = 100.0f;
        float range_x = win_width - 2.0f * margin - pickup_box_size;
        float range_y = win_height - 2.0f * margin - pickup_box_size;
        float x = margin + (float)SDL_rand((int)range_x);
        float y = margin + (float)SDL_rand((int)range_y);
        state->pickup.rect = (SDL_FRect){ x, y, pickup_box_size, pickup_box_size };
        state->pickup.active = true;
    }

    if (state->pickup.active && SDL_HasRectIntersectionFloat(&ball->rect, &state->pickup.rect)) {
        state->pickup.active = false;
        state->next_spawn_at = ticks + spawn_interval_ms;
        if (state->last_touch != TOUCH_NONE) {
            PowerupType type = (PowerupType)SDL_rand(POWERUP_COUNT);
            powerup_grant(state, type, state->last_touch == TOUCH_LEFT, ticks);
        }
    }
}

bool powerup_is_active(const PowerupState *state, PowerupType type, bool for_left, Uint64 ticks) {
    for (int i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        const ActiveEffect *e = &state->effects[i];
        if (e->in_use && e->type == type && e->affects_left == for_left && ticks < e->expires_at) {
            return true;
        }
    }
    return false;
}

bool powerup_is_active_any(const PowerupState *state, PowerupType type, Uint64 ticks) {
    for (int i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        const ActiveEffect *e = &state->effects[i];
        if (e->in_use && e->type == type && ticks < e->expires_at) {
            return true;
        }
    }
    return false;
}

float powerup_get_extra(const PowerupState *state, PowerupType type, Uint64 ticks) {
    for (int i = 0; i < MAX_ACTIVE_EFFECTS; i++) {
        const ActiveEffect *e = &state->effects[i];
        if (e->in_use && e->type == type && ticks < e->expires_at) {
            return e->extra;
        }
    }
    return 0.0f;
}

bool powerup_consume_shield(PowerupState *state, bool for_left) {
    int *count = for_left ? &state->left_shields : &state->right_shields;
    if (*count > 0) {
        (*count)--;
        return true;
    }
    return false;
}
