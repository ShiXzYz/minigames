#include "game.h"
#include "constants.h"
#include <math.h>

static const float magnet_strength = 2.5f;
static const float curve_accel = 60.0f;
static const float fireball_speedup = 1.15f;
static const float split_spread = 150.0f;
static const float hold_bob_speed = 3.0f;
static const float hold_bob_range = 80.0f;

typedef struct {
    Uint64 reaction_interval_ms;
    float error_amplitude;
    bool use_prediction;
    float dead_zone;
    float speed_mult;
} AiParams;

// Easy/normal stay at full paddle speed but track sloppily; hard reads the ball's full
// trajectory (including wall bounces) but is capped below full speed so it's still
// beatable with well-angled or spin shots instead of being unbeatable by construction.
static const AiParams ai_params[AI_DIFFICULTY_COUNT] = {
    [AI_EASY]   = { 280, 42.0f, false, 15.0f, 1.00f },
    [AI_NORMAL] = { 140, 20.0f, false,  9.0f, 1.00f },
    [AI_HARD]   = { 130, 15.0f, true,   9.0f, 0.78f },
};

static const Ball *ai_select_target_ball(const GameState *game) {
    const Ball *best = NULL;
    for (int i = 0; i < MAX_BALLS; i++) {
        const Ball *b = &game->balls[i];
        if (!b->active || b->held || b->vx <= 0.0f) continue;
        if (!best || b->rect.x > best->rect.x) best = b;
    }
    return best;
}

// Reflects the ball's straight-line trajectory off the top/bottom walls to estimate
// its y position when it reaches the paddle's x, without simulating paddle bounces.
static float ai_predict_target_y(const Ball *ball, float paddle_x) {
    float ball_front_x = ball->rect.x + ball->rect.w;
    float time_to_reach = (ball->vx > 0.0001f) ? (paddle_x - ball_front_x) / ball->vx : 0.0f;
    if (time_to_reach < 0.0f) time_to_reach = 0.0f;

    float range = win_height - ball->rect.h;
    if (range <= 0.0f) return ball->rect.y + ball->rect.h / 2.0f;

    float raw_y = ball->rect.y + ball->vy * time_to_reach;
    float m = fmodf(raw_y, 2.0f * range);
    if (m < 0.0f) m += 2.0f * range;
    float predicted_y = (m <= range) ? m : (2.0f * range - m);
    return predicted_y + ball->rect.h / 2.0f;
}

static void ai_compute_input(GameState *game, const Paddle *right_paddle, Uint64 ticks, bool *want_up, bool *want_down) {
    *want_up = false;
    *want_down = false;

    const AiParams *params = &ai_params[game->ai_difficulty];

    if (ticks >= game->ai_reaction_at) {
        const Ball *target_ball = ai_select_target_ball(game);
        float base_target;
        if (target_ball) {
            base_target = params->use_prediction
                ? ai_predict_target_y(target_ball, right_paddle->rect.x)
                : (target_ball->rect.y + target_ball->rect.h / 2.0f);
        } else {
            base_target = win_height / 2.0f;
        }

        float error = ((float)SDL_rand(2001) - 1000.0f) / 1000.0f * params->error_amplitude;
        game->ai_target_y = SDL_clamp(base_target + error, 0.0f, win_height);
        game->ai_reaction_at = ticks + params->reaction_interval_ms;
    }

    float paddle_center = right_paddle->rect.y + right_paddle->rect.h / 2.0f;
    float diff = game->ai_target_y - paddle_center;

    if (diff > params->dead_zone) {
        *want_down = true;
    } else if (diff < -params->dead_zone) {
        *want_up = true;
    }
}

static bool resolve_ball_paddle_collision(Ball *ball, SDL_FRect prev_rect, Paddle *paddle, bool paddle_is_left,
                                           const PowerupState *powerups, Uint64 ticks) {
    bool is_colliding = SDL_HasRectIntersectionFloat(&ball->rect, &paddle->rect);
    bool *was_colliding = paddle_is_left ? &ball->was_colliding_left : &ball->was_colliding_right;
    bool bounced = false;

    if (is_colliding && !*was_colliding) {
        float paddle_front_x = paddle_is_left ? (paddle->rect.x + paddle->rect.w) : paddle->rect.x;
        float prev_leading_x = paddle_is_left ? prev_rect.x : (prev_rect.x + prev_rect.w);
        float cur_leading_x  = paddle_is_left ? ball->rect.x : (ball->rect.x + ball->rect.w);

        float dx = cur_leading_x - prev_leading_x;
        float t = (fabsf(dx) > 0.0001f) ? (paddle_front_x - prev_leading_x) / dx : 1.0f;
        t = SDL_clamp(t, 0.0f, 1.0f);

        float contact_y = prev_rect.y + (ball->rect.y - prev_rect.y) * t;
        bool crossed_front_face = (contact_y + ball->rect.h >= paddle->rect.y) &&
                                   (contact_y <= paddle->rect.y + paddle->rect.h);

        float paddle_center = paddle->rect.y + paddle->rect.h / 2.0f;
        float ball_center = ball->rect.y + ball->rect.h / 2.0f;
        float relative = (ball_center - paddle_center) / (paddle->rect.h / 2.0f);
        relative = SDL_clamp(relative, -1.0f, 1.0f);

        bool fireball = powerup_is_active_any(powerups, POWERUP_FIREBALL, ticks);
        float new_vy = relative * max_bounce_speed;
        if (!fireball) {
            new_vy += paddle->vel * spin_factor;
        }

        if (crossed_front_face) {
            ball->rect.x = paddle_is_left ? (paddle->rect.x + paddle->rect.w) : (paddle->rect.x - ball->rect.w);
            ball->vx = paddle_is_left ? fabsf(ball->vx) : -fabsf(ball->vx);
            ball->vy = new_vy;
            if (fireball) {
                ball->vx *= fireball_speedup;
            }
            bounced = true;
        } else {
            ball->rect.x = paddle_is_left ? (paddle->rect.x - ball->rect.w) : (paddle->rect.x + paddle->rect.w);
            ball->vy = new_vy;
        }
    }

    *was_colliding = is_colliding;
    return bounced;
}

static const Uint64 serve_delay_ms = 500;

static void enter_state(GameState *game, GameStateKind state, Uint64 ticks) {
    game->state = state;
    game->state_entered_at = ticks;
    if (state == GAME_WAITING) {
        powerup_reset_round(&game->powerups, ticks);
    }
    if (state == GAME_MENU) {
        game->is_lan_host_match = false;
        game->vs_ai = false;
    }
}

static void end_point(GameState *game, Uint64 ticks, float serve_direction) {
    reset_ball(&game->balls[0], serve_direction);
    for (int i = 1; i < MAX_BALLS; i++) {
        game->balls[i].active = false;
    }
    enter_state(game, GAME_WAITING, ticks);
}

static bool any_ball_active(const GameState *game) {
    for (int i = 0; i < MAX_BALLS; i++) {
        if (game->balls[i].active) {
            return true;
        }
    }
    return false;
}

static bool check_for_win(GameState *game, Uint64 ticks) {
    if (game->win_score < 0) return false;
    if (game->left_score < game->win_score && game->right_score < game->win_score) return false;

    for (int i = 0; i < MAX_BALLS; i++) {
        game->balls[i].active = false;
        game->balls[i].held = false;
    }
    enter_state(game, GAME_OVER, ticks);
    return true;
}

void game_update(GameState *game, Paddle *left_paddle, Paddle *right_paddle,
                  const bool *keystate, float deltaTime, Uint64 ticks) {
    if (game->state != GAME_PAUSED) {
        bool left_up = keystate[SDL_SCANCODE_W];
        bool left_down = keystate[SDL_SCANCODE_S];

        bool right_up, right_down;
        float right_speed_mult = 1.0f;
        if (game->vs_ai && (game->state == GAME_WAITING || game->state == GAME_PLAYING)) {
            ai_compute_input(game, right_paddle, ticks, &right_up, &right_down);
            right_speed_mult = ai_params[game->ai_difficulty].speed_mult;
        } else {
            right_up = keystate[SDL_SCANCODE_UP];
            right_down = keystate[SDL_SCANCODE_DOWN];
        }

        paddle_update(left_paddle, left_up, left_down, 1.0f, deltaTime, &game->powerups, true, ticks);
        paddle_update(right_paddle, right_up, right_down, right_speed_mult, deltaTime, &game->powerups, false, ticks);
    }

    bool confirm_pressed = keystate[SDL_SCANCODE_RETURN] || keystate[SDL_SCANCODE_KP_ENTER];
    bool up_pressed = keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W];
    bool down_pressed = keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S];
    bool left_pressed = keystate[SDL_SCANCODE_LEFT];
    bool right_pressed = keystate[SDL_SCANCODE_RIGHT];
    bool escape_pressed = keystate[SDL_SCANCODE_ESCAPE];

    bool confirm_just_pressed = confirm_pressed && !game->prev_confirm;
    bool up_just_pressed = up_pressed && !game->prev_up;
    bool down_just_pressed = down_pressed && !game->prev_down;
    bool left_just_pressed = left_pressed && !game->prev_left;
    bool right_just_pressed = right_pressed && !game->prev_right;
    bool escape_just_pressed = escape_pressed && !game->prev_escape;

    game->prev_confirm = confirm_pressed;
    game->prev_up = up_pressed;
    game->prev_down = down_pressed;
    game->prev_left = left_pressed;
    game->prev_right = right_pressed;
    game->prev_escape = escape_pressed;

    if (game->state == GAME_MENU) {
        if (confirm_just_pressed) {
            enter_state(game, GAME_PLAY_TYPE_SELECT, ticks);
        }
        return;
    }

    if (game->state == GAME_PLAY_TYPE_SELECT) {
        if (up_just_pressed) {
            game->play_type_index = (game->play_type_index - 1 + PLAY_TYPE_COUNT) % PLAY_TYPE_COUNT;
            game->play_type_changed_at = ticks;
        }
        if (down_just_pressed) {
            game->play_type_index = (game->play_type_index + 1) % PLAY_TYPE_COUNT;
            game->play_type_changed_at = ticks;
        }
        if (escape_just_pressed) {
            enter_state(game, GAME_MENU, ticks);
            return;
        }
        if (confirm_just_pressed) {
            if (game->play_type_index == PLAY_TYPE_LAN) {
                game->vs_ai = false;
                enter_state(game, GAME_LAN_ROLE_SELECT, ticks);
            } else {
                enter_state(game, GAME_PLAYER_COUNT_SELECT, ticks);
            }
        }
        return;
    }

    if (game->state == GAME_PLAYER_COUNT_SELECT) {
        if (up_just_pressed || down_just_pressed) {
            game->two_player_selected = !game->two_player_selected;
            game->player_count_changed_at = ticks;
        }
        if (escape_just_pressed) {
            enter_state(game, GAME_PLAY_TYPE_SELECT, ticks);
            return;
        }
        if (confirm_just_pressed) {
            if (game->two_player_selected) {
                game->vs_ai = false;
                enter_state(game, GAME_MODE_SELECT, ticks);
            } else {
                game->vs_ai = true;
                enter_state(game, GAME_DIFFICULTY_SELECT, ticks);
            }
        }
        return;
    }

    if (game->state == GAME_DIFFICULTY_SELECT) {
        if (up_just_pressed) {
            game->ai_difficulty_index = (game->ai_difficulty_index - 1 + AI_DIFFICULTY_COUNT) % AI_DIFFICULTY_COUNT;
            game->difficulty_changed_at = ticks;
        }
        if (down_just_pressed) {
            game->ai_difficulty_index = (game->ai_difficulty_index + 1) % AI_DIFFICULTY_COUNT;
            game->difficulty_changed_at = ticks;
        }
        if (escape_just_pressed) {
            enter_state(game, GAME_PLAYER_COUNT_SELECT, ticks);
            return;
        }
        if (confirm_just_pressed) {
            game->ai_difficulty = (AiDifficulty)game->ai_difficulty_index;
            enter_state(game, GAME_MODE_SELECT, ticks);
        }
        return;
    }

    if (game->state == GAME_LAN_ROLE_SELECT) {
        if (up_just_pressed || down_just_pressed) {
            game->lan_role_host_selected = !game->lan_role_host_selected;
            game->lan_role_changed_at = ticks;
        }
        if (escape_just_pressed) {
            enter_state(game, GAME_PLAY_TYPE_SELECT, ticks);
            return;
        }
        if (confirm_just_pressed) {
            enter_state(game, game->lan_role_host_selected ? GAME_LAN_HOST_WAITING : GAME_LAN_CLIENT_SEARCHING, ticks);
        }
        return;
    }

    if (game->state == GAME_LAN_HOST_WAITING || game->state == GAME_LAN_CLIENT_SEARCHING) {
        if (escape_just_pressed) {
            enter_state(game, GAME_LAN_ROLE_SELECT, ticks);
        }
        return;
    }

    if (game->state == GAME_MODE_SELECT) {
        if (up_just_pressed || down_just_pressed) {
            game->mode = (game->mode == MODE_CLASSIC) ? MODE_POWER_PLAY : MODE_CLASSIC;
            game->mode_changed_at = ticks;
        }
        if (escape_just_pressed && !game->is_lan_host_match) {
            enter_state(game, game->vs_ai ? GAME_DIFFICULTY_SELECT : GAME_PLAYER_COUNT_SELECT, ticks);
            return;
        }
        if (confirm_just_pressed) {
            enter_state(game, GAME_WIN_SCORE_SELECT, ticks);
        }
        return;
    }

    if (game->state == GAME_WIN_SCORE_SELECT) {
        if (left_just_pressed || right_just_pressed) {
            int dir = right_just_pressed ? 1 : -1;
            game->win_score_index = (game->win_score_index + dir + win_score_options_count) % win_score_options_count;
            game->win_score = win_score_options[game->win_score_index];
            game->win_score_changed_at = ticks;
        }
        if (escape_just_pressed) {
            enter_state(game, GAME_MODE_SELECT, ticks);
            return;
        }
        if (confirm_just_pressed) {
            game->left_score = 0;
            game->right_score = 0;
            powerup_reset_match(&game->powerups, ticks);
            reset_ball(&game->balls[0], (SDL_rand(2) == 0) ? 1.0f : -1.0f);
            for (int i = 1; i < MAX_BALLS; i++) {
                game->balls[i].active = false;
            }
            enter_state(game, GAME_WAITING, ticks);
        }
        return;
    }

    if (game->state == GAME_OVER) {
        if (escape_just_pressed) {
            enter_state(game, GAME_WIN_SCORE_SELECT, ticks);
            return;
        }
        if (confirm_just_pressed) {
            enter_state(game, game->is_lan_host_match ? GAME_WIN_SCORE_SELECT : GAME_MODE_SELECT, ticks);
        }
        return;
    }

    if (game->state == GAME_PAUSED) {
        if (up_just_pressed || down_just_pressed) {
            game->pause_confirm_selected = !game->pause_confirm_selected;
            game->pause_confirm_changed_at = ticks;
        }
        if (escape_just_pressed) {
            enter_state(game, game->paused_from, ticks);
            return;
        }
        if (confirm_just_pressed) {
            if (game->pause_confirm_selected) {
                enter_state(game, GAME_MENU, ticks);
            } else {
                enter_state(game, game->paused_from, ticks);
            }
        }
        return;
    }

    if (game->state == GAME_WAITING || game->state == GAME_PLAYING) {
        if (escape_just_pressed) {
            game->paused_from = game->state;
            game->pause_confirm_selected = false;
            enter_state(game, GAME_PAUSED, ticks);
            return;
        }
    }

    if (game->state == GAME_WAITING) {
        bool ready_to_serve = (ticks - game->state_entered_at) >= serve_delay_ms;
        if (ready_to_serve &&
            (confirm_pressed ||
             keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_S] ||
             keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_DOWN])) {
            enter_state(game, GAME_PLAYING, ticks);
        }
        return;
    }

    bool power_play = (game->mode == MODE_POWER_PLAY);

    if (power_play) {
        if (game->powerups.pending_split_shot) {
            game->powerups.pending_split_shot = false;
            if (!game->balls[0].held) {
                game->balls[1] = game->balls[0];
                game->balls[1].vy -= split_spread;
                game->balls[0].vy += split_spread;
            }
        }
        if (game->balls[1].active && !powerup_is_active_any(&game->powerups, POWERUP_SPLIT_SHOT, ticks)) {
            float fallback_direction = (game->balls[1].vx >= 0.0f) ? 1.0f : -1.0f;
            game->balls[1].active = false;
            if (!any_ball_active(game)) {
                end_point(game, ticks, fallback_direction);
                return;
            }
        }
    }

    for (int i = 0; i < MAX_BALLS; i++) {
        Ball *ball = &game->balls[i];
        if (!ball->active) continue;

        if (ball->held) {
            Paddle *holder = ball->held_by_left ? left_paddle : right_paddle;
            float bob = sinf((float)(ticks - ball->held_since) / 1000.0f * hold_bob_speed);
            float holder_center_y = holder->rect.y + holder->rect.h / 2.0f;

            ball->rect.x = ball->held_by_left ? (holder->rect.x + holder->rect.w + 4.0f) : (holder->rect.x - ball->rect.w - 4.0f);
            ball->rect.y = SDL_clamp(holder_center_y + bob * hold_bob_range - ball->rect.h / 2.0f, 0.0f, win_height - ball->rect.h);

            if (confirm_just_pressed) {
                float launch_dir = ball->held_by_left ? 1.0f : -1.0f;
                ball->vx = launch_dir * ball_start_speed;
                ball->vy = bob * max_bounce_speed;
                ball->held = false;
            }
            continue;
        }

        if (power_play) {
            float curve_dir = powerup_get_extra(&game->powerups, POWERUP_CURVE_BALL, ticks);
            ball->vy += curve_dir * curve_accel * deltaTime;

            if (powerup_is_active(&game->powerups, POWERUP_MAGNET, true, ticks)) {
                float target = left_paddle->rect.y + left_paddle->rect.h / 2.0f;
                float ball_center = ball->rect.y + ball->rect.h / 2.0f;
                ball->vy += (target - ball_center) * magnet_strength * deltaTime;
            }
            if (powerup_is_active(&game->powerups, POWERUP_MAGNET, false, ticks)) {
                float target = right_paddle->rect.y + right_paddle->rect.h / 2.0f;
                float ball_center = ball->rect.y + ball->rect.h / 2.0f;
                ball->vy += (target - ball_center) * magnet_strength * deltaTime;
            }
        }

        float speed_scale = 1.0f;
        if (power_play && powerup_is_active_any(&game->powerups, POWERUP_SLOW_MOTION, ticks)) {
            speed_scale = 0.5f;
        }

        SDL_FRect ball_prev_rect = ball->rect;
        ball_update(ball, deltaTime, speed_scale);

        if (ball->rect.x + ball->rect.w < 0) {
            bool scored = !(power_play && powerup_consume_shield(&game->powerups, true));
            if (scored) {
                game->right_score++;
                SDL_Log("Left: %d   Right: %d", game->left_score, game->right_score);
            }
            ball->active = false;
            ball->held = false;
            if (scored && check_for_win(game, ticks)) {
                break;
            }
            if (!any_ball_active(game)) {
                end_point(game, ticks, 1.0f);
                break;
            }
            continue;
        }

        if (ball->rect.x > win_width) {
            bool scored = !(power_play && powerup_consume_shield(&game->powerups, false));
            if (scored) {
                game->left_score++;
                SDL_Log("Left: %d   Right: %d", game->left_score, game->right_score);
            }
            ball->active = false;
            ball->held = false;
            if (scored && check_for_win(game, ticks)) {
                break;
            }
            if (!any_ball_active(game)) {
                end_point(game, ticks, -1.0f);
                break;
            }
            continue;
        }

        bool left_bounced = resolve_ball_paddle_collision(ball, ball_prev_rect, left_paddle, true, &game->powerups, ticks);
        bool right_bounced = resolve_ball_paddle_collision(ball, ball_prev_rect, right_paddle, false, &game->powerups, ticks);

        if (power_play) {
            powerup_note_touch(&game->powerups, left_bounced ? TOUCH_LEFT : (right_bounced ? TOUCH_RIGHT : TOUCH_NONE));
            powerup_update(&game->powerups, ball, ticks);

            if (game->powerups.pending_hold_shot) {
                game->powerups.pending_hold_shot = false;
                if (!ball->held) {
                    ball->held = true;
                    ball->held_by_left = game->powerups.pending_hold_shot_is_left;
                    ball->held_since = ticks;
                    ball->vx = 0.0f;
                    ball->vy = 0.0f;
                }
            }
        }
    }
}

void game_start_lan_match(GameState *game, Uint64 ticks) {
    game->is_lan_host_match = true;
    game->left_score = 0;
    game->right_score = 0;
    powerup_reset_match(&game->powerups, ticks);
    enter_state(game, GAME_MODE_SELECT, ticks);
}
