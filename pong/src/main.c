#include <stdbool.h>
#include <SDL3/SDL.h>

#include "constants.h"
#include "ball.h"
#include "paddle.h"
#include "game.h"
#include "renderer.h"

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_srand((Uint64)SDL_GetPerformanceCounter());

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

    Paddle left_paddle = { { 50.0f, win_height / 2.0f - paddle_height / 2.0f, paddle_width, paddle_height }, 0.0f, false };
    Paddle right_paddle = { { 750.0f, win_height / 2.0f - paddle_height / 2.0f, paddle_width, paddle_height }, 0.0f, false };
    Ball ball = { { win_width / 2.0f - ball_size / 2.0f, win_height / 2.0f - ball_size / 2.0f, ball_size, ball_size }, ball_start_speed, -ball_start_speed };

    GameState game = { 0, 0, GAME_MENU, 0 };

    Uint64 prevTime = SDL_GetTicks();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        const bool *keystate = SDL_GetKeyboardState(NULL);

        Uint64 curTime = SDL_GetTicks();
        Uint64 deltaMS = curTime - prevTime;
        float deltaTime = deltaMS / 1000.0f;
        prevTime = curTime;

        game_update(&game, &ball, &left_paddle, &right_paddle, keystate, deltaTime, curTime);
        render_frame(renderer, &ball, &left_paddle, &right_paddle, &game, curTime);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
