#include "net.h"

typedef enum {
    MSG_DISCOVER = 1,
    MSG_ANNOUNCE = 2,
    MSG_INPUT = 3,
    MSG_STATE = 4
} NetMsgType;

typedef struct {
    Uint8 up;
    Uint8 down;
} NetInputPayload;

static const Uint64 discover_interval_ms = 500;

static bool send_message(NetSession *net, NET_Address *addr, Uint16 port, Uint8 type, const void *payload, int payload_len) {
    Uint8 buf[1024];
    if (payload_len + 1 > (int)sizeof(buf)) return false;
    buf[0] = type;
    if (payload_len > 0) {
        SDL_memcpy(buf + 1, payload, payload_len);
    }
    return NET_SendDatagram(net->socket, addr, port, buf, payload_len + 1);
}

bool net_startup(void) {
    return NET_Init();
}

void net_shutdown(void) {
    NET_Quit();
}

void net_host_begin(NetSession *net) {
    SDL_zerop(net);
    net->role = NET_ROLE_HOST;
    net->socket = NET_CreateDatagramSocket(NULL, PONG_NET_PORT, 0);
}

void net_client_begin(NetSession *net, Uint64 ticks) {
    (void)ticks;
    SDL_zerop(net);
    net->role = NET_ROLE_CLIENT;

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true);
    net->socket = NET_CreateDatagramSocket(NULL, 0, props);
    SDL_DestroyProperties(props);
}

void net_end(NetSession *net) {
    if (net->peer_addr) {
        NET_UnrefAddress(net->peer_addr);
    }
    if (net->socket) {
        NET_DestroyDatagramSocket(net->socket);
    }
    SDL_zerop(net);
}

bool net_host_poll_for_connection(NetSession *net) {
    if (net->connected) return false;

    bool found = false;
    for (;;) {
        NET_Datagram *dgram = NULL;
        if (!NET_ReceiveDatagram(net->socket, &dgram)) break;
        if (!dgram) break;

        if (!found && dgram->buflen >= 1 && dgram->buf[0] == MSG_DISCOVER) {
            net->peer_addr = NET_RefAddress(dgram->addr);
            net->peer_port = dgram->port;
            net->connected = true;
            send_message(net, net->peer_addr, net->peer_port, MSG_ANNOUNCE, NULL, 0);
            found = true;
        }

        NET_DestroyDatagram(dgram);
    }
    return found;
}

bool net_client_poll_for_host(NetSession *net, Uint64 ticks) {
    if (net->connected) return false;

    if (ticks - net->last_discover_sent_at >= discover_interval_ms) {
        send_message(net, NULL, PONG_NET_PORT, MSG_DISCOVER, NULL, 0);
        net->last_discover_sent_at = ticks;
    }

    bool found = false;
    for (;;) {
        NET_Datagram *dgram = NULL;
        if (!NET_ReceiveDatagram(net->socket, &dgram)) break;
        if (!dgram) break;

        if (!found && dgram->buflen >= 1 && dgram->buf[0] == MSG_ANNOUNCE) {
            net->peer_addr = NET_RefAddress(dgram->addr);
            net->peer_port = dgram->port;
            net->connected = true;
            found = true;
        }

        NET_DestroyDatagram(dgram);
    }
    return found;
}

void net_host_poll_input(NetSession *net) {
    for (;;) {
        NET_Datagram *dgram = NULL;
        if (!NET_ReceiveDatagram(net->socket, &dgram)) break;
        if (!dgram) break;

        if (dgram->buflen >= (int)(1 + sizeof(NetInputPayload)) && dgram->buf[0] == MSG_INPUT &&
            net->peer_addr && NET_CompareAddresses(dgram->addr, net->peer_addr) == 0) {
            NetInputPayload payload;
            SDL_memcpy(&payload, dgram->buf + 1, sizeof(payload));
            net->remote_up = payload.up != 0;
            net->remote_down = payload.down != 0;
        }

        NET_DestroyDatagram(dgram);
    }
}

void net_host_send_state(NetSession *net, const NetSnapshot *snap) {
    if (!net->connected) return;
    send_message(net, net->peer_addr, net->peer_port, MSG_STATE, snap, sizeof(*snap));
}

void net_client_send_input(NetSession *net, bool up, bool down) {
    if (!net->connected) return;
    NetInputPayload payload = { up ? (Uint8)1 : (Uint8)0, down ? (Uint8)1 : (Uint8)0 };
    send_message(net, net->peer_addr, net->peer_port, MSG_INPUT, &payload, sizeof(payload));
}

bool net_client_poll_state(NetSession *net) {
    bool found = false;
    for (;;) {
        NET_Datagram *dgram = NULL;
        if (!NET_ReceiveDatagram(net->socket, &dgram)) break;
        if (!dgram) break;

        if (dgram->buflen >= (int)(1 + sizeof(NetSnapshot)) && dgram->buf[0] == MSG_STATE &&
            net->peer_addr && NET_CompareAddresses(dgram->addr, net->peer_addr) == 0) {
            SDL_memcpy(&net->latest_snapshot, dgram->buf + 1, sizeof(NetSnapshot));
            net->has_snapshot = true;
            found = true;
        }

        NET_DestroyDatagram(dgram);
    }
    return found;
}
