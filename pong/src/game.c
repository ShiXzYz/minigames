#include "game.h"
#include "constants.h"
#include <math.h>

static void resolve_ball_paddle_collision(Ball *ball, SDL_FRect prev_rect, Paddle *paddle, bool paddle_is_left) {
    bool is_colliding = SDL_HasRectIntersectionFloat(&ball->rect, &paddle->rect);

    if (is_colliding && !paddle->was_colliding) {
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
        float new_vy = relative * max_bounce_speed + paddle->vel * spin_factor;

        if (crossed_front_face) {
            ball->rect.x = paddle_is_left ? (paddle->rect.x + paddle->rect.w) : (paddle->rect.x - ball->rect.w);
            ball->vx = paddle_is_left ? fabsf(ball->vx) : -fabsf(ball->vx);
            ball->vy = new_vy;
        } else {
            ball->rect.x = paddle_is_left ? (paddle->rect.x - ball->rect.w) : (paddle->rect.x + paddle->rect.w);
            ball->vy = new_vy;
        }
    }

    paddle->was_colliding = is_colliding;
}

void game_update(GameState *game, Ball *ball, Paddle *left_paddle, Paddle *right_paddle,
                  const bool *keystate, float deltaTime) {
    paddle_update(left_paddle, keystate, SDL_SCANCODE_W, SDL_SCANCODE_S, deltaTime);
    paddle_update(right_paddle, keystate, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, deltaTime);

    // Ball stays parked at center until a player signals they're ready.
    if (!game->started) {
        if (keystate[SDL_SCANCODE_RETURN] || keystate[SDL_SCANCODE_KP_ENTER] ||
            keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_S] ||
            keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_DOWN]) {
            game->started = true;
        }
    }

    if (!game->started) {
        return;
    }

    SDL_FRect ball_prev_rect = ball->rect;
    ball_update(ball, deltaTime);

    // scoring
    if (ball->rect.x + ball->rect.w < 0) {
        game->right_score++;
        SDL_Log("Left: %d   Right: %d", game->left_score, game->right_score);
        reset_ball(ball, 1.0f);
        game->started = false;
    }

    if (ball->rect.x > win_width) {
        game->left_score++;
        SDL_Log("Left: %d   Right: %d", game->left_score, game->right_score);
        reset_ball(ball, -1.0f);
        game->started = false;
    }

    resolve_ball_paddle_collision(ball, ball_prev_rect, left_paddle, true);
    resolve_ball_paddle_collision(ball, ball_prev_rect, right_paddle, false);
}
