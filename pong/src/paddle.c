#include "paddle.h"
#include "constants.h"

void paddle_update(Paddle *paddle, bool pressed_up, bool pressed_down, float speed_mult,
                    float deltaTime, const PowerupState *powerups, bool is_left, Uint64 ticks) {
    float speed = paddle_speed * speed_mult;
    if (powerup_is_active(powerups, POWERUP_SPEED_BOOST, is_left, ticks)) {
        speed *= 2.0f;
    }
    if (powerup_is_active_any(powerups, POWERUP_SLOW_MOTION, ticks)) {
        speed *= 0.5f;
    }

    float height_mult = 1.0f;
    if (powerup_is_active(powerups, POWERUP_GIANT_PADDLE, is_left, ticks)) {
        height_mult *= 2.0f;
    }
    if (powerup_is_active(powerups, POWERUP_TINY_PADDLE, is_left, ticks)) {
        height_mult *= 0.5f;
    }
    paddle->rect.h = paddle_height * height_mult;

    if (powerup_is_active(powerups, POWERUP_INVERT_CONTROLS, is_left, ticks)) {
        bool tmp = pressed_up;
        pressed_up = pressed_down;
        pressed_down = tmp;
    }

    paddle->vel = 0.0f;

    if (pressed_up) {
        paddle->rect.y -= speed * deltaTime;
        paddle->vel = -speed;
    }

    if (pressed_down) {
        paddle->rect.y += speed * deltaTime;
        paddle->vel = speed;
    }

    if (paddle->rect.y < 0) {
        paddle->rect.y = 0;
    }

    if (paddle->rect.y > win_height - paddle->rect.h) {
        paddle->rect.y = win_height - paddle->rect.h;
    }
}
