#include <stdbool.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "constants.h"
#include "ball.h"
#include "paddle.h"
#include "game.h"
#include "renderer.h"
#include "net.h"

static bool net_blink_visible(Uint64 changed_at, Uint64 ticks) {
    const Uint64 blink_duration = 480;
    const Uint64 blink_interval = 80;
    Uint64 elapsed = ticks - changed_at;
    if (elapsed >= blink_duration) return true;
    return (elapsed / blink_interval) % 2 == 0;
}

static NetSnapshot build_snapshot(const GameState *game, const Paddle *left_paddle, const Paddle *right_paddle, Uint64 ticks) {
    NetSnapshot snap = { 0 };
    snap.state = (Sint32)game->state;
    snap.win_score = game->win_score;
    snap.left_score = game->left_score;
    snap.right_score = game->right_score;
    snap.pause_confirm_selected = game->pause_confirm_selected;
    snap.win_score_underline_visible = net_blink_visible(game->win_score_changed_at, ticks);
    snap.pause_underline_visible = net_blink_visible(game->pause_confirm_changed_at, ticks);
    snap.left_paddle_rect = left_paddle->rect;
    snap.right_paddle_rect = right_paddle->rect;
    for (int i = 0; i < MAX_BALLS; i++) {
        snap.balls[i].rect = game->balls[i].rect;
        snap.balls[i].active = game->balls[i].active;
        snap.balls[i].held = game->balls[i].held;
    }
    return snap;
}

// Translates the host's precomputed blink flags into a local timestamp that makes
// underline_blink_visible() (in renderer.c, driven by the client's own clock) agree
// with the host's intent for this frame, since the two processes' SDL_GetTicks()
// epochs aren't synchronized.
static void apply_snapshot_to_game(GameState *game, Paddle *left_paddle, Paddle *right_paddle,
                                    const NetSnapshot *snap, Uint64 ticks) {
    game->state = (GameStateKind)snap->state;
    game->mode = MODE_CLASSIC;
    game->win_score = snap->win_score;
    game->left_score = snap->left_score;
    game->right_score = snap->right_score;
    game->pause_confirm_selected = snap->pause_confirm_selected;
    game->win_score_changed_at = ticks - (snap->win_score_underline_visible ? 9999 : 80);
    game->pause_confirm_changed_at = ticks - (snap->pause_underline_visible ? 9999 : 80);

    left_paddle->rect = snap->left_paddle_rect;
    right_paddle->rect = snap->right_paddle_rect;

    for (int i = 0; i < MAX_BALLS; i++) {
        game->balls[i].rect = snap->balls[i].rect;
        game->balls[i].active = snap->balls[i].active;
        game->balls[i].held = snap->balls[i].held;
    }
}

int main(int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    if (!net_startup()) {
        SDL_Log("net_startup failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_srand((Uint64)SDL_GetPerformanceCounter());

    SDL_Window *window = SDL_CreateWindow("Pong", win_width, win_height, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        net_shutdown();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        net_shutdown();
        SDL_Quit();
        return 1;
    }

    Paddle left_paddle = { { 50.0f, win_height / 2.0f - paddle_height / 2.0f, paddle_width, paddle_height }, 0.0f };
    Paddle right_paddle = { { 750.0f, win_height / 2.0f - paddle_height / 2.0f, paddle_width, paddle_height }, 0.0f };

    GameState game = { 0, 0, GAME_MENU, 0 };
    game.win_score_index = 2;
    game.win_score = win_score_options[game.win_score_index];
    game.balls[0].rect = (SDL_FRect){ win_width / 2.0f - ball_size / 2.0f, win_height / 2.0f - ball_size / 2.0f, ball_size, ball_size };
    game.balls[0].vx = ball_start_speed;
    game.balls[0].vy = -ball_start_speed;
    game.balls[0].active = true;

    NetSession net = { 0 };

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

        GameStateKind state_before = game.state;

        if (net.role == NET_ROLE_CLIENT && net.connected) {
            bool up = keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W];
            bool down = keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S];
            net_client_send_input(&net, up, down);
            if (net_client_poll_state(&net)) {
                apply_snapshot_to_game(&game, &left_paddle, &right_paddle, &net.latest_snapshot, curTime);
            }
        } else {
            const bool *use_keystate = keystate;
            bool overridden_keystate[SDL_SCANCODE_COUNT];
            if (net.role == NET_ROLE_HOST && net.connected) {
                net_host_poll_input(&net);
                memcpy(overridden_keystate, keystate, sizeof(overridden_keystate));
                overridden_keystate[SDL_SCANCODE_UP] = net.remote_up;
                overridden_keystate[SDL_SCANCODE_DOWN] = net.remote_down;
                use_keystate = overridden_keystate;
            }

            game_update(&game, &left_paddle, &right_paddle, use_keystate, deltaTime, curTime);

            if (net.role == NET_ROLE_HOST && !net.connected && game.state == GAME_LAN_HOST_WAITING) {
                if (net_host_poll_for_connection(&net)) {
                    game_start_lan_match(&game, curTime);
                }
            }
            if (net.role == NET_ROLE_CLIENT && !net.connected && game.state == GAME_LAN_CLIENT_SEARCHING) {
                net_client_poll_for_host(&net, curTime);
            }
            if (net.role == NET_ROLE_HOST && net.connected) {
                NetSnapshot snap = build_snapshot(&game, &left_paddle, &right_paddle, curTime);
                net_host_send_state(&net, &snap);
            }
        }

        if (game.state != state_before) {
            if (game.state == GAME_LAN_HOST_WAITING) {
                net_host_begin(&net);
            } else if (game.state == GAME_LAN_CLIENT_SEARCHING) {
                net_client_begin(&net, curTime);
            } else if ((state_before == GAME_LAN_HOST_WAITING || state_before == GAME_LAN_CLIENT_SEARCHING) && !net.connected) {
                net_end(&net);
            }
            if (game.state == GAME_MENU) {
                net_end(&net);
            }
        }

        render_frame(renderer, &left_paddle, &right_paddle, &game, curTime);
    }

    net_end(&net);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    net_shutdown();
    SDL_Quit();
    return 0;
}
