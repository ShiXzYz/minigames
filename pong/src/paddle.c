#include "paddle.h"
#include "constants.h"

void paddle_update(Paddle *paddle, const bool *keystate, SDL_Scancode up_key, SDL_Scancode down_key, float deltaTime) {
    paddle->vel = 0.0f;

    if (keystate[up_key]) {
        paddle->rect.y -= paddle_speed * deltaTime;
        paddle->vel = -paddle_speed;
    }

    if (keystate[down_key]) {
        paddle->rect.y += paddle_speed * deltaTime;
        paddle->vel = paddle_speed;
    }

    if (paddle->rect.y < 0) {
        paddle->rect.y = 0;
    }

    if (paddle->rect.y > win_height - paddle->rect.h) {
        paddle->rect.y = win_height - paddle->rect.h;
    }
}
