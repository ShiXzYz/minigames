#include "renderer.h"
#include "constants.h"

#include <stdbool.h>
#include <stdio.h>
#include <math.h>

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

// 5x7 bitmap font, A-Z only. Each row's bit4 = leftmost column.
static const int font5x7[26][7] = {
    { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 }, // A
    { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 }, // B
    { 0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111 }, // C
    { 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 }, // D
    { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 }, // E
    { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 }, // F
    { 0b01111, 0b10000, 0b10000, 0b10011, 0b10001, 0b10001, 0b01111 }, // G
    { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 }, // H
    { 0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 }, // I
    { 0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 }, // J
    { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 }, // K
    { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 }, // L
    { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 }, // M
    { 0b10001, 0b11001, 0b10101, 0b10101, 0b10011, 0b10001, 0b10001 }, // N
    { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 }, // O
    { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 }, // P
    { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 }, // Q
    { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 }, // R
    { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 }, // S
    { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 }, // T
    { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 }, // U
    { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 }, // V
    { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010 }, // W
    { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 }, // X
    { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 }, // Y
    { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 }, // Z
};

static const int question_mark_glyph[7] = {
    0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100,
};

static void draw_char(SDL_Renderer *renderer, char c, float x, float y, float pixel) {
    const int *rows;
    if (c >= 'A' && c <= 'Z') {
        rows = font5x7[c - 'A'];
    } else if (c == '?') {
        rows = question_mark_glyph;
    } else {
        return;
    }

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (rows[row] & (1 << (4 - col))) {
                SDL_FRect px = { x + col * pixel, y + row * pixel, pixel, pixel };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

static float text_width(const char *text, float pixel, float spacing) {
    int len = 0;
    while (text[len] != '\0') len++;
    return len > 0 ? len * (5.0f * pixel + spacing) - spacing : 0.0f;
}

static void draw_text(SDL_Renderer *renderer, const char *text, float x, float y, float pixel, float spacing) {
    float cursor_x = x;
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(renderer, text[i], cursor_x, y, pixel);
        cursor_x += 5.0f * pixel + spacing;
    }
}

static void draw_text_centered(SDL_Renderer *renderer, const char *text, float y, float pixel, float spacing) {
    float x = win_width / 2.0f - text_width(text, pixel, spacing) / 2.0f;
    draw_text(renderer, text, x, y, pixel, spacing);
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

static void draw_hollow_rect(SDL_Renderer *renderer, SDL_FRect rect, float thickness) {
    SDL_FRect top = { rect.x, rect.y, rect.w, thickness };
    SDL_FRect bottom = { rect.x, rect.y + rect.h - thickness, rect.w, thickness };
    SDL_FRect left = { rect.x, rect.y, thickness, rect.h };
    SDL_FRect right = { rect.x + rect.w - thickness, rect.y, thickness, rect.h };
    SDL_RenderFillRect(renderer, &top);
    SDL_RenderFillRect(renderer, &bottom);
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
}

static void set_paddle_color(SDL_Renderer *renderer, const GameState *game, bool is_left, Uint64 ticks) {
    const PowerupState *pu = &game->powerups;
    if (powerup_is_active(pu, POWERUP_GIANT_PADDLE, is_left, ticks)) {
        SDL_SetRenderDrawColor(renderer, 120, 255, 120, 255);
    } else if (powerup_is_active(pu, POWERUP_TINY_PADDLE, is_left, ticks)) {
        SDL_SetRenderDrawColor(renderer, 255, 120, 120, 255);
    } else if (powerup_is_active(pu, POWERUP_SPEED_BOOST, is_left, ticks)) {
        SDL_SetRenderDrawColor(renderer, 100, 220, 255, 255);
    } else if (powerup_is_active(pu, POWERUP_INVERT_CONTROLS, is_left, ticks)) {
        SDL_SetRenderDrawColor(renderer, 200, 120, 255, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
}

static void set_ball_color(SDL_Renderer *renderer, const GameState *game, const Ball *ball, Uint64 ticks) {
    const PowerupState *pu = &game->powerups;
    if (ball->held) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 150, 255);
    } else if (powerup_is_active_any(pu, POWERUP_SPLIT_SHOT, ticks)) {
        SDL_SetRenderDrawColor(renderer, 180, 255, 80, 255);
    } else if (powerup_is_active_any(pu, POWERUP_FIREBALL, ticks)) {
        SDL_SetRenderDrawColor(renderer, 255, 120, 0, 255);
    } else if (powerup_is_active_any(pu, POWERUP_SLOW_MOTION, ticks)) {
        SDL_SetRenderDrawColor(renderer, 120, 180, 255, 255);
    } else if (powerup_is_active(pu, POWERUP_MAGNET, true, ticks) || powerup_is_active(pu, POWERUP_MAGNET, false, ticks)) {
        SDL_SetRenderDrawColor(renderer, 255, 80, 220, 255);
    } else if (powerup_is_active_any(pu, POWERUP_CURVE_BALL, ticks)) {
        SDL_SetRenderDrawColor(renderer, 80, 220, 200, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
}

static const char *powerup_display_name(PowerupType type) {
    switch (type) {
        case POWERUP_SPEED_BOOST: return "SPEED BOOST";
        case POWERUP_SLOW_MOTION: return "SLOW MOTION";
        case POWERUP_GIANT_PADDLE: return "GIANT PADDLE";
        case POWERUP_TINY_PADDLE: return "TINY PADDLE";
        case POWERUP_FIREBALL: return "FIREBALL";
        case POWERUP_CURVE_BALL: return "CURVE BALL";
        case POWERUP_MAGNET: return "MAGNET";
        case POWERUP_INVERT_CONTROLS: return "INVERT CONTROLS";
        case POWERUP_INVISIBLE_BALL: return "INVISIBLE BALL";
        case POWERUP_SHIELD: return "SHIELD";
        case POWERUP_SPLIT_SHOT: return "SPLIT SHOT";
        case POWERUP_HOLD_SHOT: return "HOLD AND SHOOT";
        case POWERUP_CHAOS_BOX: return "CHAOS BOX";
        default: return "";
    }
}

static void draw_powerup_announcement(SDL_Renderer *renderer, const GameState *game, Uint64 ticks) {
    const PowerupState *pu = &game->powerups;
    if (!pu->has_last_activated) return;

    const float total = 1800.0f;
    const float enter = 150.0f;
    float elapsed = (float)(ticks - pu->last_activated_at);
    if (elapsed >= total) return;

    float y_offset;
    if (elapsed < enter) {
        y_offset = (1.0f - elapsed / enter) * -20.0f;
    } else if (elapsed > total - enter) {
        y_offset = ((elapsed - (total - enter)) / enter) * -20.0f;
    } else {
        y_offset = sinf((elapsed - enter) / 1000.0f * 6.0f) * 3.0f;
    }

    SDL_SetRenderDrawColor(renderer, 255, 240, 120, 255);
    draw_text_centered(renderer, powerup_display_name(pu->last_activated_type), 20.0f + y_offset, 4.0f, 3.0f);
}

static bool underline_blink_visible(Uint64 changed_at, Uint64 ticks) {
    const Uint64 blink_duration = 480;
    const Uint64 blink_interval = 80;
    Uint64 elapsed = ticks - changed_at;
    if (elapsed >= blink_duration) return true;
    return (elapsed / blink_interval) % 2 == 0;
}

static const char *win_score_label(int win_score) {
    switch (win_score) {
        case 5: return "FIVE";
        case 7: return "SEVEN";
        case 11: return "ELEVEN";
        case 21: return "TWENTY ONE";
        default: return "UNLIMITED";
    }
}

void render_frame(SDL_Renderer *renderer, const Paddle *left_paddle,
                   const Paddle *right_paddle, const GameState *game, Uint64 ticks) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    if (game->state == GAME_MENU) {
        draw_text_centered(renderer, "PONG", 200.0f, 8.0f, 4.0f);
        if ((ticks / 500) % 2 == 0) {
            draw_text_centered(renderer, "PRESS ENTER TO START", 400.0f, 3.0f, 2.0f);
        }
        SDL_RenderPresent(renderer);
        return;
    }

    if (game->state == GAME_PAUSED) {
        draw_text_centered(renderer, "PAUSED", 150.0f, 7.0f, 4.0f);
        draw_text_centered(renderer, "RETURN TO MENU", 280.0f, 4.0f, 3.0f);

        const float yes_y = 360.0f;
        const float no_y = 420.0f;
        const float option_pixel = 4.0f;
        const float option_spacing = 3.0f;

        draw_text_centered(renderer, "YES", yes_y, option_pixel, option_spacing);
        draw_text_centered(renderer, "NO", no_y, option_pixel, option_spacing);

        const char *selected_label = game->pause_confirm_selected ? "YES" : "NO";
        float selected_y = game->pause_confirm_selected ? yes_y : no_y;
        float underline_w = text_width(selected_label, option_pixel, option_spacing);
        SDL_FRect underline = { win_width / 2.0f - underline_w / 2.0f, selected_y + 7.0f * option_pixel + 6.0f, underline_w, 3.0f };
        if (underline_blink_visible(game->pause_confirm_changed_at, ticks)) {
            SDL_RenderFillRect(renderer, &underline);
        }

        draw_text_centered(renderer, "UP DOWN ENTER", 500.0f, 2.0f, 2.0f);

        SDL_RenderPresent(renderer);
        return;
    }

    if (game->state == GAME_MODE_SELECT) {
        draw_text_centered(renderer, "SELECT MODE", 100.0f, 6.0f, 3.0f);

        const float classic_y = 250.0f;
        const float power_play_y = 340.0f;
        const float option_pixel = 5.0f;
        const float option_spacing = 3.0f;

        draw_text_centered(renderer, "CLASSIC", classic_y, option_pixel, option_spacing);
        draw_text_centered(renderer, "POWER PLAY", power_play_y, option_pixel, option_spacing);

        const char *selected_label = (game->mode == MODE_CLASSIC) ? "CLASSIC" : "POWER PLAY";
        float selected_y = (game->mode == MODE_CLASSIC) ? classic_y : power_play_y;
        float underline_w = text_width(selected_label, option_pixel, option_spacing);
        SDL_FRect underline = { win_width / 2.0f - underline_w / 2.0f, selected_y + 7.0f * option_pixel + 6.0f, underline_w, 3.0f };
        if (underline_blink_visible(game->mode_changed_at, ticks)) {
            SDL_RenderFillRect(renderer, &underline);
        }

        draw_text_centered(renderer, "UP DOWN ENTER", 480.0f, 2.0f, 2.0f);

        SDL_RenderPresent(renderer);
        return;
    }

    if (game->state == GAME_WIN_SCORE_SELECT) {
        draw_text_centered(renderer, "POINTS TO WIN", 100.0f, 6.0f, 3.0f);

        const float win_score_value_y = 280.0f;
        const float win_score_pixel = 5.0f;
        const float win_score_spacing = 3.0f;

        const char *win_score_text = win_score_label(game->win_score);
        draw_text_centered(renderer, win_score_text, win_score_value_y, win_score_pixel, win_score_spacing);

        float win_score_underline_w = text_width(win_score_text, win_score_pixel, win_score_spacing);
        SDL_FRect win_score_underline = { win_width / 2.0f - win_score_underline_w / 2.0f, win_score_value_y + 7.0f * win_score_pixel + 6.0f, win_score_underline_w, 3.0f };
        if (underline_blink_visible(game->win_score_changed_at, ticks)) {
            SDL_RenderFillRect(renderer, &win_score_underline);
        }

        draw_text_centered(renderer, "LEFT RIGHT ENTER", 480.0f, 2.0f, 2.0f);

        SDL_RenderPresent(renderer);
        return;
    }

    if (game->state == GAME_OVER) {
        const char *winner_label = (game->left_score > game->right_score) ? "LEFT WINS" : "RIGHT WINS";

        draw_text_centered(renderer, "GAME OVER", 180.0f, 7.0f, 4.0f);
        draw_text_centered(renderer, winner_label, 300.0f, 5.0f, 3.0f);

        draw_number(renderer, game->left_score, win_width / 4.0f - 10.0f, 380.0f, 20.0f, 30.0f, 4.0f, 8.0f);
        draw_number(renderer, game->right_score, win_width * 3.0f / 4.0f - 10.0f, 380.0f, 20.0f, 30.0f, 4.0f, 8.0f);

        if ((ticks / 500) % 2 == 0) {
            draw_text_centered(renderer, "PRESS ENTER", 480.0f, 3.0f, 2.0f);
        }

        SDL_RenderPresent(renderer);
        return;
    }

    draw_center_line(renderer);

    bool power_play = (game->mode == MODE_POWER_PLAY);

    if (power_play && game->powerups.pickup.active) {
        SDL_FRect pickup_rect = game->powerups.pickup.rect;
        SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
        draw_hollow_rect(renderer, pickup_rect, 3.0f);

        float qm_pixel = 3.0f;
        float qm_w = 5.0f * qm_pixel;
        float qm_h = 7.0f * qm_pixel;
        draw_char(renderer, '?', pickup_rect.x + pickup_rect.w / 2.0f - qm_w / 2.0f,
                  pickup_rect.y + pickup_rect.h / 2.0f - qm_h / 2.0f, qm_pixel);
    }

    if (power_play) {
        set_paddle_color(renderer, game, true, ticks);
    }
    SDL_RenderFillRect(renderer, &left_paddle->rect);
    if (power_play && game->powerups.left_shields > 0) {
        SDL_SetRenderDrawColor(renderer, 255, 230, 120, 255);
        SDL_FRect outline = { left_paddle->rect.x - 4.0f, left_paddle->rect.y - 4.0f, left_paddle->rect.w + 8.0f, left_paddle->rect.h + 8.0f };
        draw_hollow_rect(renderer, outline, 2.0f);
    }

    if (power_play) {
        set_paddle_color(renderer, game, false, ticks);
    }
    SDL_RenderFillRect(renderer, &right_paddle->rect);
    if (power_play && game->powerups.right_shields > 0) {
        SDL_SetRenderDrawColor(renderer, 255, 230, 120, 255);
        SDL_FRect outline = { right_paddle->rect.x - 4.0f, right_paddle->rect.y - 4.0f, right_paddle->rect.w + 8.0f, right_paddle->rect.h + 8.0f };
        draw_hollow_rect(renderer, outline, 2.0f);
    }

    bool ball_invisible = power_play && powerup_is_active_any(&game->powerups, POWERUP_INVISIBLE_BALL, ticks);
    for (int i = 0; i < MAX_BALLS; i++) {
        const Ball *ball = &game->balls[i];
        if (!ball->active) continue;

        if (ball_invisible && !ball->held) {
            if ((ticks / 150) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
                SDL_RenderFillRect(renderer, &ball->rect);
            }
        } else {
            if (power_play) {
                set_ball_color(renderer, game, ball, ticks);
            }
            SDL_RenderFillRect(renderer, &ball->rect);
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    draw_number(renderer, game->left_score, win_width / 4.0f - 10.0f, 20.0f, 20.0f, 30.0f, 4.0f, 8.0f);
    draw_number(renderer, game->right_score, win_width * 3.0f / 4.0f - 10.0f, 20.0f, 20.0f, 30.0f, 4.0f, 8.0f);

    if (power_play) {
        draw_powerup_announcement(renderer, game, ticks);
    }

    SDL_RenderPresent(renderer);
}
