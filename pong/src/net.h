#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include "ball.h"
#include "powerup.h"

#define PONG_NET_PORT 7777

typedef enum {
    NET_ROLE_NONE,
    NET_ROLE_HOST,
    NET_ROLE_CLIENT
} NetRole;

typedef struct {
    SDL_FRect rect;
    bool active;
    bool held;
} NetBallSnapshot;

typedef struct {
    Sint32 type;
    bool affects_left;
    bool in_use;
    float extra;
    Uint32 remaining_ms;
} NetEffectSnapshot;

typedef struct {
    Sint32 state;
    Sint32 mode;
    Sint32 win_score;
    Sint32 left_score;
    Sint32 right_score;
    bool pause_confirm_selected;
    bool win_score_underline_visible;
    bool pause_underline_visible;
    bool mode_underline_visible;
    SDL_FRect left_paddle_rect;
    SDL_FRect right_paddle_rect;
    NetBallSnapshot balls[MAX_BALLS];

    int left_shields;
    int right_shields;
    SDL_FRect pickup_rect;
    bool pickup_active;
    bool has_last_activated;
    Sint32 last_activated_type;
    Uint32 last_activated_elapsed_ms;
    NetEffectSnapshot effects[MAX_ACTIVE_EFFECTS];
} NetSnapshot;

typedef struct {
    NetRole role;
    NET_DatagramSocket *socket;
    NET_Address *peer_addr;
    Uint16 peer_port;
    bool connected;
    Uint64 last_discover_sent_at;

    bool remote_up;
    bool remote_down;

    NetSnapshot latest_snapshot;
    bool has_snapshot;
} NetSession;

bool net_startup(void);
void net_shutdown(void);

void net_host_begin(NetSession *net);
void net_client_begin(NetSession *net, Uint64 ticks);
void net_end(NetSession *net);

bool net_host_poll_for_connection(NetSession *net);
bool net_client_poll_for_host(NetSession *net, Uint64 ticks);

void net_host_poll_input(NetSession *net);
void net_host_send_state(NetSession *net, const NetSnapshot *snap);

void net_client_send_input(NetSession *net, bool up, bool down);
bool net_client_poll_state(NetSession *net);
