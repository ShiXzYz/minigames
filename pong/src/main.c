#include <stdbool.h>
#include <SDL3/SDL.h>

const float win_height = 600.0f;
const float win_width = 800.0f;
const float paddle_speed = 400.0f;

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

    SDL_FRect left_paddle = { 50.0f, 250.0f, 10.0f, 75.0f };
    SDL_FRect right_paddle = { 750.0f, 250.0f, 10.0f, 75.0f };

    Uint64 prevTime = SDL_GetTicks();

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

        prevTime = curTime;

        if(state[SDL_SCANCODE_W]) {
            left_paddle.y -= paddle_speed * deltaTime;
        }

        if(state[SDL_SCANCODE_S]) {
            left_paddle.y += paddle_speed * deltaTime;
        }

        if(state[SDL_SCANCODE_UP]) {
            right_paddle.y -= paddle_speed * deltaTime;
        }

        if(state[SDL_SCANCODE_DOWN]) {
            right_paddle.y += paddle_speed * deltaTime;
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

        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_RenderFillRect(renderer, &left_paddle);
        SDL_RenderFillRect(renderer, &right_paddle);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
