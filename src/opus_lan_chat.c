#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <alsa/asoundlib.h>
#include <opus/opus.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum {
    audio_sample_rate = 16000,
    audio_channels = 1,
    audio_frame_ms = 20,
    audio_frame_samples = (audio_sample_rate / 1000) * audio_frame_ms,
    opus_max_packet = 400,
    app_invalid_fd = -1,
};

typedef enum {
    app_ok = 0,
    app_err_args,
    app_err_parse,
    app_err_range,
    app_err_socket,
    app_err_bind,
    app_err_connect,
    app_err_audio,
    app_err_opus,
    app_err_io,
} app_err_t;

#define app_assert_ptr(p) assert((p) != NULL)
#define app_guard_ptr(p, err) do { if ((p) == NULL) { return (err); } } while (0)
#define app_guard_range(v, lo, hi, err) do { if ((v) < (lo) || (v) > (hi)) { return (err); } } while (0)

typedef struct {
    int sock_fd;
    snd_pcm_t *pcm;
    OpusEncoder *enc;
    OpusDecoder *dec;
} app_ctx_t;

static void app_ctx_init(app_ctx_t *ctx) {
    app_assert_ptr(ctx);
    ctx->sock_fd = app_invalid_fd;
    ctx->pcm = NULL;
    ctx->enc = NULL;
    ctx->dec = NULL;
}

static void app_ctx_cleanup(app_ctx_t *ctx) {
    app_assert_ptr(ctx);
    if (ctx->enc != NULL) {
        opus_encoder_destroy(ctx->enc);
        ctx->enc = NULL;
    }
    if (ctx->dec != NULL) {
        opus_decoder_destroy(ctx->dec);
        ctx->dec = NULL;
    }
    if (ctx->pcm != NULL) {
        snd_pcm_close(ctx->pcm);
        ctx->pcm = NULL;
    }
    if (ctx->sock_fd >= 0) {
        close(ctx->sock_fd);
        ctx->sock_fd = app_invalid_fd;
    }
}

static app_err_t net_sock_open_udp(int *out_fd) {
    app_assert_ptr(out_fd);
    app_guard_ptr(out_fd, app_err_args);
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return app_err_socket;
    }
    *out_fd = fd;
    return app_ok;
}

static app_err_t net_sock_bind(int fd, const char *ip, uint16_t port) {
    app_assert_ptr(ip);
    app_guard_ptr(ip, app_err_args);
    app_guard_range(port, 1, UINT16_MAX, app_err_range);
    if (fd < 0) {
        return app_err_socket;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        return app_err_parse;
    }
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return app_err_bind;
    }
    return app_ok;
}

static app_err_t net_sock_connect(int fd, const char *ip, uint16_t port) {
    app_assert_ptr(ip);
    app_guard_ptr(ip, app_err_args);
    app_guard_range(port, 1, UINT16_MAX, app_err_range);
    if (fd < 0) {
        return app_err_socket;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        return app_err_parse;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return app_err_connect;
    }
    return app_ok;
}

static app_err_t audio_pcm_open(snd_pcm_t **pcm, snd_pcm_stream_t stream) {
    app_assert_ptr(pcm);
    app_guard_ptr(pcm, app_err_args);

    int rc = snd_pcm_open(pcm, "default", stream, 0);
    if (rc < 0 || *pcm == NULL) {
        return app_err_audio;
    }

    rc = snd_pcm_set_params(*pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                            audio_channels, audio_sample_rate, 1, 2 * 1000);
    if (rc < 0) {
        snd_pcm_close(*pcm);
        *pcm = NULL;
        return app_err_audio;
    }

    return app_ok;
}

static app_err_t opus_enc_open(OpusEncoder **enc) {
    app_assert_ptr(enc);
    app_guard_ptr(enc, app_err_args);
    int rc = OPUS_OK;
    *enc = opus_encoder_create(audio_sample_rate, audio_channels, OPUS_APPLICATION_VOIP, &rc);
    if (rc != OPUS_OK || *enc == NULL) {
        return app_err_opus;
    }
    if (opus_encoder_ctl(*enc, OPUS_SET_BITRATE(20000)) != OPUS_OK) { return app_err_opus; }
    if (opus_encoder_ctl(*enc, OPUS_SET_INBAND_FEC(1)) != OPUS_OK) { return app_err_opus; }
    if (opus_encoder_ctl(*enc, OPUS_SET_DTX(1)) != OPUS_OK) { return app_err_opus; }
    return app_ok;
}

static app_err_t opus_dec_open(OpusDecoder **dec) {
    app_assert_ptr(dec);
    app_guard_ptr(dec, app_err_args);
    int rc = OPUS_OK;
    *dec = opus_decoder_create(audio_sample_rate, audio_channels, &rc);
    if (rc != OPUS_OK || *dec == NULL) {
        return app_err_opus;
    }
    return app_ok;
}

static app_err_t app_run_tx(const char *dst_ip, uint16_t dst_port) {
    app_assert_ptr(dst_ip);
    app_guard_ptr(dst_ip, app_err_args);
    app_guard_range(dst_port, 1, UINT16_MAX, app_err_range);

    app_ctx_t ctx;
    app_ctx_init(&ctx);
    int16_t pcm[audio_frame_samples];
    uint8_t pkt[opus_max_packet];

    app_err_t rc = net_sock_open_udp(&ctx.sock_fd);
    if (rc != app_ok) { return rc; }
    rc = net_sock_connect(ctx.sock_fd, dst_ip, dst_port);
    if (rc != app_ok) { app_ctx_cleanup(&ctx); return rc; }
    rc = audio_pcm_open(&ctx.pcm, SND_PCM_STREAM_CAPTURE);
    if (rc != app_ok) { app_ctx_cleanup(&ctx); return rc; }
    rc = opus_enc_open(&ctx.enc);
    if (rc != app_ok) { app_ctx_cleanup(&ctx); return rc; }

    for (;;) {
        snd_pcm_sframes_t got = snd_pcm_readi(ctx.pcm, pcm, audio_frame_samples);
        if (got == -EPIPE) { snd_pcm_prepare(ctx.pcm); continue; }
        if (got < 0) { rc = app_err_io; break; }
        if (got != audio_frame_samples) { continue; }

        int n = opus_encode(ctx.enc, pcm, audio_frame_samples, pkt, (opus_int32)sizeof(pkt));
        if (n <= 0 || n > (int)sizeof(pkt)) { rc = app_err_opus; break; }

        ssize_t sent = send(ctx.sock_fd, pkt, (size_t)n, 0);
        if (sent != n) { rc = app_err_io; break; }
    }

    app_ctx_cleanup(&ctx);
    return rc;
}

static app_err_t app_run_rx(const char *bind_ip, uint16_t bind_port) {
    app_assert_ptr(bind_ip);
    app_guard_ptr(bind_ip, app_err_args);
    app_guard_range(bind_port, 1, UINT16_MAX, app_err_range);

    app_ctx_t ctx;
    app_ctx_init(&ctx);
    uint8_t pkt[opus_max_packet];
    int16_t pcm[audio_frame_samples];

    app_err_t rc = net_sock_open_udp(&ctx.sock_fd);
    if (rc != app_ok) { return rc; }
    rc = net_sock_bind(ctx.sock_fd, bind_ip, bind_port);
    if (rc != app_ok) { app_ctx_cleanup(&ctx); return rc; }
    rc = audio_pcm_open(&ctx.pcm, SND_PCM_STREAM_PLAYBACK);
    if (rc != app_ok) { app_ctx_cleanup(&ctx); return rc; }
    rc = opus_dec_open(&ctx.dec);
    if (rc != app_ok) { app_ctx_cleanup(&ctx); return rc; }

    for (;;) {
        ssize_t got = recv(ctx.sock_fd, pkt, sizeof(pkt), 0);
        if (got <= 0 || got > (ssize_t)sizeof(pkt)) { rc = app_err_io; break; }

        int n = opus_decode(ctx.dec, pkt, (opus_int32)got, pcm, audio_frame_samples, 0);
        if (n < 0 || n > audio_frame_samples) { continue; }

        snd_pcm_sframes_t wr = snd_pcm_writei(ctx.pcm, pcm, (snd_pcm_uframes_t)n);
        if (wr == -EPIPE) { snd_pcm_prepare(ctx.pcm); continue; }
        if (wr < 0) { rc = app_err_io; break; }
    }

    app_ctx_cleanup(&ctx);
    return rc;
}

static app_err_t app_parse_port(const char *text, uint16_t *out_port) {
    app_assert_ptr(text);
    app_assert_ptr(out_port);
    app_guard_ptr(text, app_err_args);
    app_guard_ptr(out_port, app_err_args);

    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return app_err_parse;
    }
    if (value == 0 || value > UINT16_MAX) {
        return app_err_range;
    }
    *out_port = (uint16_t)value;
    return app_ok;
}

static void app_usage(const char *prog) {
    app_assert_ptr(prog);
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  %s tx <dst_ip> <port>\n", prog);
    fprintf(stderr, "  %s rx <bind_ip> <port>\n", prog);
}

int main(int argc, char **argv) {
    if (argc != 4 || argv == NULL || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
        app_usage((argv && argv[0]) ? argv[0] : "opus_lan_chat");
        return app_err_args;
    }

    uint16_t port = 0;
    app_err_t parse_rc = app_parse_port(argv[3], &port);
    if (parse_rc != app_ok) {
        app_usage(argv[0]);
        return parse_rc;
    }

    if (strcmp(argv[1], "tx") == 0) { return app_run_tx(argv[2], port); }
    if (strcmp(argv[1], "rx") == 0) { return app_run_rx(argv[2], port); }

    app_usage(argv[0]);
    return app_err_args;
}
