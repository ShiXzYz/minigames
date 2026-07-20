#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <SDL3/SDL.h>

const float win_height = 600.0f;
const float win_width = 800.0f;
const float paddle_speed = 400.0f;

const float max_bounce_speed = 500.0f;
const float spin_factor = 0.30f;

int left_score = 0;
int right_score = 0;

void reset_ball(SDL_FRect *ball, float *vx, float *vy, float direction) {
    ball->x = win_width / 2.0f - ball->w / 2.0f;
    ball->y = win_height / 2.0f - ball->h / 2.0f;

    *vx = direction * 200.0f;
    *vy = 0.0f;
}

// Any contact with a paddle sends the ball back into play, full stop -- Pong doesn't
// distinguish a corner clip from a dead-center hit. `was_colliding` makes this fire
// once per contact instead of every frame the ball happens to still overlap the
// paddle (which previously caused rapid repeat bounces / a jittery "yank" near the
// paddle's top and bottom edges).
void resolve_ball_paddle_collision(SDL_FRect *ball, float *ball_vx, float *ball_vy,
                                    const SDL_FRect *paddle, float paddle_vel, bool paddle_is_left,
                                    bool *was_colliding) {
    bool is_colliding = SDL_HasRectIntersectionFloat(ball, paddle);

    if (is_colliding && !*was_colliding) {
        ball->x = paddle_is_left ? (paddle->x + paddle->w) : (paddle->x - ball->w);

        float paddle_center = paddle->y + paddle->h / 2.0f;
        float ball_center = ball->y + ball->h / 2.0f;
        float relative = (ball_center - paddle_center) / (paddle->h / 2.0f);

        *ball_vx = paddle_is_left ? fabsf(*ball_vx) : -fabsf(*ball_vx);
        *ball_vy = relative * max_bounce_speed + paddle_vel * spin_factor;
    }

    *was_colliding = is_colliding;
}

// Which of the 7 segments (a,b,c,d,e,f,g) are lit for each digit 0-9.
//   _a_
// f|   |b
//  |_g_|
// e|   |c
//  |_d_|
static const bool digit_segments[10][7] = {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}, // 9
};

void draw_digit(SDL_Renderer *renderer, int digit, float x, float y, float w, float h, float t) {
    if (digit < 0 || digit > 9) return;

    const bool *seg = digit_segments[digit];
    SDL_FRect rects[7] = {
        { x,         y,             w, t   }, // a - top
        { x + w - t, y,             t, h/2 }, // b - top-right
        { x + w - t, y + h/2,       t, h/2 }, // c - bottom-right
        { x,         y + h - t,     w, t   }, // d - bottom
        { x,         y + h/2,       t, h/2 }, // e - bottom-left
        { x,         y,             t, h/2 }, // f - top-left
        { x,         y + h/2 - t/2, w, t   }, // g - middle
    };

    for (int i = 0; i < 7; i++) {
        if (seg[i]) {
            SDL_RenderFillRect(renderer, &rects[i]);
        }
    }
}

void draw_number(SDL_Renderer *renderer, int number, float x, float y, float digit_w, float digit_h, float thickness, float spacing) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", number);

    float cursor_x = x;
    for (int i = 0; buf[i] != '\0'; i++) {
        draw_digit(renderer, buf[i] - '0', cursor_x, y, digit_w, digit_h, thickness);
        cursor_x += digit_w + spacing;
    }
}

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Pong", win_width, win_height, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_FRect left_paddle = { 50.0f, 250.0f, 20.0f, 100.0f };
    SDL_FRect right_paddle = { 750.0f, 250.0f, 20.0f, 100.0f };
    SDL_FRect ball = { 400.0f, 250.0f, 20.0f, 20.0f };

    float ball_vx = 100.0f;
    float ball_vy = -100.0f;

    Uint64 prevTime = SDL_GetTicks();

    float left_paddle_vel = 0.0f;
    float right_paddle_vel = 0.0f;

    bool game_started = false;

    bool left_was_colliding = false;
    bool right_was_colliding = false;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        const bool *state = SDL_GetKeyboardState(NULL);

        Uint64 curTime = SDL_GetTicks();

        Uint64 deltaMS = curTime - prevTime;
        float deltaTime = deltaMS / 1000.0f;

        left_paddle_vel = 0.0f;
        right_paddle_vel = 0.0f;

        prevTime = curTime;

        if(state[SDL_SCANCODE_W]) {
            left_paddle.y -= paddle_speed * deltaTime;
            left_paddle_vel = -paddle_speed;
        }

        if(state[SDL_SCANCODE_S]) {
            left_paddle.y += paddle_speed * deltaTime;
            left_paddle_vel = paddle_speed;
        }

        if(state[SDL_SCANCODE_UP]) {
            right_paddle.y -= paddle_speed * deltaTime;
            right_paddle_vel = -paddle_speed;
        }

        if(state[SDL_SCANCODE_DOWN]) {
            right_paddle.y += paddle_speed * deltaTime;
            right_paddle_vel = paddle_speed;
        }

        if(left_paddle.y < 0) {
            left_paddle.y = 0;
        }

        if(left_paddle.y > win_height - left_paddle.h) {
            left_paddle.y = win_height - left_paddle.h;
        }

        if(right_paddle.y < 0) {
            right_paddle.y = 0;
        }

        if(right_paddle.y > win_height - right_paddle.h) {
            right_paddle.y = win_height - right_paddle.h;
        }

        // Ball stays parked at center until a player signals they're ready.
        if (!game_started) {
            if (state[SDL_SCANCODE_RETURN] || state[SDL_SCANCODE_KP_ENTER] ||
                state[SDL_SCANCODE_W] || state[SDL_SCANCODE_S] ||
                state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_DOWN]) {
                game_started = true;
            }
        }

        if (game_started) {
            if(ball.y < 0){
                ball.y = 0;
                ball_vy = -ball_vy;
            }
            if(ball.y > win_height - ball.h) {
                ball.y = win_height - ball.h;
                ball_vy = -ball_vy;
            }

            ball.x += ball_vx * deltaTime;
            ball.y += ball_vy * deltaTime;

            // scoring
            if (ball.x + ball.w < 0) {
                right_score++;
                SDL_Log("Left: %d   Right: %d", left_score, right_score);
                reset_ball(&ball, &ball_vx, &ball_vy, 1.0f);
                game_started = false;
            }

            if (ball.x > win_width) {
                left_score++;
                SDL_Log("Left: %d   Right: %d", left_score, right_score);
                reset_ball(&ball, &ball_vx, &ball_vy, -1.0f);
                game_started = false;
            }

            resolve_ball_paddle_collision(&ball, &ball_vx, &ball_vy, &left_paddle, left_paddle_vel, true, &left_was_colliding);
            resolve_ball_paddle_collision(&ball, &ball_vx, &ball_vy, &right_paddle, right_paddle_vel, false, &right_was_colliding);
        }

        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_RenderFillRect(renderer, &left_paddle);
        SDL_RenderFillRect(renderer, &right_paddle);
        SDL_RenderFillRect(renderer, &ball);

        draw_number(renderer, left_score, win_width / 4.0f - 10.0f, 20.0f, 20.0f, 30.0f, 4.0f, 8.0f);
        draw_number(renderer, right_score, win_width * 3.0f / 4.0f - 10.0f, 20.0f, 20.0f, 30.0f, 4.0f, 8.0f);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
