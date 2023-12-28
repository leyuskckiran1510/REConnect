#include "message.h"
#include "def.h"
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT_SEC 5
#define TIMEOUT_MILISEC 0

__THROW void error(const char *msg) {
#ifdef __DEBUG_MODE__
    perror(msg);
#else
    perror(msg);
    exit(EXIT_FAILURE);
#endif
}

int has_data(int sock) {
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = TIMEOUT_MILISEC;
    return select(sock + 1, &rfds, NULL, NULL, &tv);
}

int send_hello(const info to, const CLIENT _client) {

    MESSAGE m = {.header = {.type = E_HELLO, .sequence = 0},
                 .data = {.to = _client, .from = _client}};

    clock_gettime(0, (struct timespec *)&m.data.timestamp);
    sprintf(m.data.text, "HELLO FROM {%s}", _client.username);
    int _s = sendto(to.sockfd, &m, sizeof(MESSAGE), 0, &to._sockaddr,
                    sizeof(to._sockaddr));

    if (_s == -1) {
        error("Error Sending Client Hello");
        return -1;
    }

    return 0;
}
int recv_hello(const info __has_dot_sockfd, void *buffer) {

    MESSAGE *m = (MESSAGE *)buffer;
    int _s = recvfrom(__has_dot_sockfd.sockfd, m, sizeof(MESSAGE), 0, NULL, 0);
    if (_s == -1) {
        error("Error Sending Client Hello");
        return -1;
    }

    return 0;
}

int send_ack(const info to, uint32_t ack_value) {
    M_HELLO_ACK m = {
        .header.type = E_HELLO_ACK,
        .header.length = -1,
        .acknowledged = ack_value,
    };
    int _s = sendto(to.sockfd, &m, sizeof(M_HELLO_ACK), 0, &to._sockaddr,
                    sizeof(to._sockaddr));
    if (_s == -1) {
        error("Error Sending Acknowledgement");
        return -1;
    }
    return 1;
}

int recive_ack(const info __has_dot_sockfd, void *buffer) {
    M_HELLO_ACK *m = (M_HELLO_ACK *)buffer;
    // for later use if needed
    //  struct sockaddr send_from;
    //  socklen_t recived_l;
    int _s = recvfrom(__has_dot_sockfd.sockfd, &m, sizeof(MESSAGE), 0, NULL, 0);
    if (_s == -1) {
        error("Couldnot Understand Server Hello");
        return -1;
    }
    if (m->header.type == E_HELLO_ACK) {
        return m->acknowledged;
    } else {
        return 0;
    }
}

int send_heart_beat(const info to) {
    M_HEARTBEAT m = {
        .header.type = E_HEART_BEAT,
        .header.length = -1,
        .ping = 1,
    };
    int _s = sendto(to.sockfd, &m, sizeof(m), 0, &to._sockaddr,
                    sizeof(to._sockaddr));
    if (_s == -1) {
        error("Error Sending HeartBeat");
    }
    return _s < 0 ? -1 : 1;
}

int recv_heart_beat(const info __has_dot_sockfd, void *buffer) {
    M_HEARTBEAT *m = (M_HEARTBEAT *)buffer;
    int _s =
        recvfrom(__has_dot_sockfd.sockfd, &m, sizeof(M_HEARTBEAT), 0, NULL, 0);
    if (!(_s < 0)) {
        return m->pong;
    } else {
        error("Error Receving HeartBeatResponse");
    }
    return -1;
}

int recv_text(const info __has_dot_sockfd, void *buffer) {
    MESSAGE *m = (MESSAGE *)buffer;
    int _s = recv(__has_dot_sockfd.sockfd, &m, sizeof(MESSAGE), 0);
    if (!(_s < 0)) {
        return E_TEXT_MESSGAE;
    } else {
        error("Error Receving HeartBeatResponse");
    }
    return -1;
}

int send_chunks(const int sockfd, const CLIENT from, const CLIENT to,
                const readable *buffer) {
    MESSAGE m = {
        .header.type = E_CONTENT_CONTINUE,
    };
    if (buffer->type != READABLE) {
        error("Provided Endpoint is not writeable");
        return E_ERROR;
    }
    char *message = buffer->message;
    int length = -1;
    if (buffer->dtype == D_TEXT) {
        length = strlen(message);
    }

    int computed_len = MAX_CONTENT_AT_ONCE;
    int _s = 0, retry_count = 10;
    // first send the content as message_text
    while (1) {
        m.data.from = from;
        m.data.to = to;
        gettimeofday(&m.data.timestamp, NULL);
        if (buffer->dtype == D_TEXT && _s >= 0) {
            m.header.length = computed_len;
            memcpy(&m.data.text, message, computed_len);
        } else if (_s >= 0) {
            m.header.length = fread(&m.data.content, sizeof(char),
                                    MAX_CONTENT_AT_ONCE, buffer->file);
        }
        _s = sendto(sockfd, &m, sizeof(m), 0, &to.address.sockaddr_in,
                    sizeof(to.address.sockaddr_in));
        if (_s >= 0 && buffer->dtype == D_TEXT) {
            if (length == 0) {
                break;
            }
            length -= computed_len;
            message += computed_len;
            computed_len = (length / MAX_CONTENT_AT_ONCE)
                               ? MAX_CONTENT_AT_ONCE
                               : length % MAX_CONTENT_AT_ONCE;
        } else if (_s == -1) {
            error("Error While Sending Message Chunk");
            struct timespec sleep_till = {
                .tv_sec = 0,
                .tv_nsec = 5e8, // 0.5 seconds
            };
            retry_count--;
            if (!retry_count) {
                break;
            }
            nanosleep(&sleep_till, NULL);
        } else if (feof(buffer->file)) {
            break;
        }
    }
    m.header.type = E_CONTENT_CONT_END;
    m.header.length = -1;
    *(m.data.text) = '\0';

    while (sendto(sockfd, &m, sizeof(m), 0, NULL, 0) < 0) {
        error("Error Sending the End Dectector");
    }
    return E_CONTENT_CONTINUE;
}

int recv_chunks(const int sockfd, writeable buffer, CLIENT *from) {
    (void)from;
    if (buffer.type != WRITEABLE) {
        error("The Provided Buffer is not writeable");
        return E_ERROR;
    }
    MESSAGE m;
    MSG_TYPE mtyp = 0, old_mtyp = 0;
    int _s;
    size_t offset = 0, recv_len;
    while (mtyp == old_mtyp) {
        _s = recv(sockfd, &m, sizeof(m), 0);
        if(_s<0){
            continue;
        }
        if (old_mtyp == 0) {
            old_mtyp = m.header.type;
        } else {
            old_mtyp = mtyp;
        }
        mtyp = m.header.type;
        recv_len = m.header.length;
        memcpy((buffer.message + offset), m.data.text, recv_len);
        offset += recv_len;
        if ((buffer.message + offset) == NULL) {
            error("Buffer OverFlow please proved a big enoug buffer");
            return E_ERROR;
        }
    }
    return 1;
}

int send_text(const int sockfd, const char *message, CLIENT from, CLIENT to) {
    MESSAGE m = {.header.type = E_TEXT_MESSGAE,
                 .header.length = strlen(message)};
    int length = strlen(message);
    if (length > MAX_CONTENT_AT_ONCE) {
        readable _stream = {
            .dtype = D_TEXT, .type = READABLE, .message = (char *)message};
        return send_chunks(sockfd, from, to, &_stream);
    }
    memcpy(&m.data, message, length);
    int _s = sendto(sockfd, &m, sizeof(m), 0, &to.address.sockaddr_in,
                    sizeof(to.address.sockaddr_in));
    if (_s == -1) {
        error("Error Sending Text Message");
        return E_ERROR;
    }
    return E_TEXT_MESSGAE;
}

int _recv_text(const int sockfd, const char *message, CLIENT from, CLIENT to) {
    MESSAGE m = {.header.type = E_TEXT_MESSGAE,
                 .header.length = strlen(message)};
    int length = strlen(message);
    if (length > MAX_CONTENT_AT_ONCE) {
        readable _stream = {
            .dtype = D_TEXT, .type = READABLE, .message = (char *)message};
        return send_chunks(sockfd, from, to, &_stream);
    }
    memcpy(&m.data, message, length);
    int _s = sendto(sockfd, &m, sizeof(m), 0, &to.address.sockaddr_in,
                    sizeof(to.address.sockaddr_in));
    if (_s == -1) {
        error("Error Sending Text Message");
        return E_ERROR;
    }
    return E_TEXT_MESSGAE;
}


MSG_TYPE revive_any(const info from, void *buffer) {
    MSG_HEADER m;
    if (!has_data(from.sockfd)) {
        return 0;
    }
    int _s = recvfrom(from.sockfd, &m, sizeof(m), MSG_PEEK, NULL, 0);
    if (_s == -1) {
        error("Error Peeking Into Socket");
        return -1;
    }
    switch (m.type) {
    case E_ERROR: {
        assert("TODO:// HANDLE ERROR MESSAGES" && 1);
        return E_ERROR;
    }
    case E_HELLO: {
        if (recv_hello(from, buffer) >= 0) {
            return E_HELLO;
        }
        return E_ERROR;
    }
    case E_HELLO_ACK: {
        if (recive_ack(from, buffer) >= 0) {
            return E_HELLO_ACK;
        }
        return E_ERROR;
    }
    case E_HEART_BEAT: {
        if (recv_heart_beat(from, buffer) >= 0) {
            return E_HEART_BEAT;
        }
        return E_ERROR;
    }
    case E_TEXT_MESSGAE: {
        if(recv_text(from,buffer)>=0){
            return E_TEXT_MESSGAE;
        }
        return E_ERROR;
    }
    case E_CONTENT_CONTINUE: {
        assert("TODO:// HANDLE case E_CONTENT_CONTINUE:" && 1);
        return E_CONTENT_CONTINUE;
    }
    case E_CONTENT_CONT_END: {
        assert("TODO:// HANDLE case E_CONTENT_CONT_END:" && 1);
        return E_CONTENT_CONT_END;
    }
    case E_AUDIO_MESSAGE: {
        assert("TODO:// HANDLE case E_AUDIO_MESSAGE:" && 1);
        return E_AUDIO_MESSAGE;
    }
    case E_VIDEO_MESSGAE: {
        assert("TODO:// HANDLE case E_VIDEO_MESSGAE:" && 1);
        return E_VIDEO_MESSGAE;
    }
    case E_AV_MESSAGE: {
        assert("TODO:// HANDLE case E_AV_MESSAGE:" && 1);
        return E_AV_MESSAGE;
    }
    case E_RDP_MESSAGE: {
        assert("TODO:// HANDLE case E_RDP_MESSAGE:" && 1);
        return E_RDP_MESSAGE;
    } break;
    }
    return E_ERROR;
}
