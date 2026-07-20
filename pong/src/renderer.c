#include "renderer.h"
#include "constants.h"

#include <stdbool.h>
#include <stdio.h>

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

static void draw_digit(SDL_Renderer *renderer, int digit, float x, float y, float w, float h, float t) {
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

static void draw_number(SDL_Renderer *renderer, int number, float x, float y, float digit_w, float digit_h, float thickness, float spacing) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", number);

    float cursor_x = x;
    for (int i = 0; buf[i] != '\0'; i++) {
        draw_digit(renderer, buf[i] - '0', cursor_x, y, digit_w, digit_h, thickness);
        cursor_x += digit_w + spacing;
    }
}

static void draw_center_line(SDL_Renderer *renderer) {
    const float dash_width = 4.0f;
    const float dash_height = 15.0f;
    const float gap = 10.0f;
    const float x = win_width / 2.0f - dash_width / 2.0f;

    for (float y = 0.0f; y < win_height; y += dash_height + gap) {
        SDL_FRect dash = { x, y, dash_width, dash_height };
        SDL_RenderFillRect(renderer, &dash);
    }
}

void render_frame(SDL_Renderer *renderer, const Ball *ball, const Paddle *left_paddle,
                   const Paddle *right_paddle, const GameState *game) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    draw_center_line(renderer);

    SDL_RenderFillRect(renderer, &left_paddle->rect);
    SDL_RenderFillRect(renderer, &right_paddle->rect);
    SDL_RenderFillRect(renderer, &ball->rect);

    draw_number(renderer, game->left_score, win_width / 4.0f - 10.0f, 20.0f, 20.0f, 30.0f, 4.0f, 8.0f);
    draw_number(renderer, game->right_score, win_width * 3.0f / 4.0f - 10.0f, 20.0f, 20.0f, 30.0f, 4.0f, 8.0f);

    SDL_RenderPresent(renderer);
}
