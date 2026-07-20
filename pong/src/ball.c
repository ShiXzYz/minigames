#include "ball.h"
#include "constants.h"

void reset_ball(Ball *ball, float direction) {
    ball->rect.x = win_width / 2.0f - ball->rect.w / 2.0f;
    ball->rect.y = win_height / 2.0f - ball->rect.h / 2.0f;

    ball->vx = direction * ball_start_speed;
    ball->vy = 0.0f;
}

void ball_update(Ball *ball, float deltaTime) {
    if (ball->rect.y < 0) {
        ball->rect.y = 0;
        ball->vy = -ball->vy;
    }
    if (ball->rect.y > win_height - ball->rect.h) {
        ball->rect.y = win_height - ball->rect.h;
        ball->vy = -ball->vy;
    }

    ball->rect.x += ball->vx * deltaTime;
    ball->rect.y += ball->vy * deltaTime;
}
