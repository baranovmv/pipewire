/* PipeWire */
/* SPDX-FileCopyrightText: Copyright © 2025 PipeWire contributors */
/* SPDX-License-Identifier: MIT */

#include "config.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <roc/config.h>
#include <roc/context.h>
#include <roc/endpoint.h>
#include <roc/frame.h>
#include <roc/log.h>
#include <roc/metrics.h>
#include <roc/receiver.h>
#include <roc/sender.h>

#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/utils/defs.h>

#include <pipewire/pipewire.h>
#include <pipewire/impl.h>

#include "pwtest.h"

#include "../src/modules/module-roc/common.h"

/*
 * Tier 1: Unit tests for common.h helpers
 */

PWTEST(test_spa_format_to_roc)
{
	roc_subformat out;

	pwtest_int_eq(pw_roc_spa_format_to_roc(SPA_AUDIO_FORMAT_S16, &out), 0);
	pwtest_int_eq((int)out, (int)ROC_SUBFORMAT_PCM_SINT16);

	pwtest_int_eq(pw_roc_spa_format_to_roc(SPA_AUDIO_FORMAT_S24, &out), 0);
	pwtest_int_eq((int)out, (int)ROC_SUBFORMAT_PCM_SINT24);

	pwtest_int_eq(pw_roc_spa_format_to_roc(SPA_AUDIO_FORMAT_S32, &out), 0);
	pwtest_int_eq((int)out, (int)ROC_SUBFORMAT_PCM_SINT32);

	pwtest_int_eq(pw_roc_spa_format_to_roc(SPA_AUDIO_FORMAT_F32, &out), 0);
	pwtest_int_eq((int)out, (int)ROC_SUBFORMAT_PCM_FLOAT32);

	pwtest_int_eq(pw_roc_spa_format_to_roc(SPA_AUDIO_FORMAT_F64, &out), 0);
	pwtest_int_eq((int)out, (int)ROC_SUBFORMAT_PCM_FLOAT64);

	pwtest_int_eq(pw_roc_spa_format_to_roc(SPA_AUDIO_FORMAT_U8, &out), -1);

	return PWTEST_PASS;
}

PWTEST(test_spa_format_sample_size)
{
	pwtest_int_eq((int)pw_roc_spa_format_sample_size(SPA_AUDIO_FORMAT_S16), 2);
	pwtest_int_eq((int)pw_roc_spa_format_sample_size(SPA_AUDIO_FORMAT_S24), 3);
	pwtest_int_eq((int)pw_roc_spa_format_sample_size(SPA_AUDIO_FORMAT_S32), 4);
	pwtest_int_eq((int)pw_roc_spa_format_sample_size(SPA_AUDIO_FORMAT_F32), 4);
	pwtest_int_eq((int)pw_roc_spa_format_sample_size(SPA_AUDIO_FORMAT_F64), 8);
	pwtest_int_eq((int)pw_roc_spa_format_sample_size(SPA_AUDIO_FORMAT_U8), 0);

	return PWTEST_PASS;
}

PWTEST(test_channels_to_layout)
{
	pwtest_int_eq((int)pw_roc_channels_to_layout(1), (int)ROC_CHANNEL_LAYOUT_MONO);
	pwtest_int_eq((int)pw_roc_channels_to_layout(2), (int)ROC_CHANNEL_LAYOUT_STEREO);
	pwtest_int_eq((int)pw_roc_channels_to_layout(4), (int)ROC_CHANNEL_LAYOUT_MULTITRACK);
	pwtest_int_eq((int)pw_roc_channels_to_layout(8), (int)ROC_CHANNEL_LAYOUT_MULTITRACK);

	return PWTEST_PASS;
}

PWTEST(test_parse_fec_encoding)
{
	roc_fec_encoding fec;

	pwtest_int_eq(pw_roc_parse_fec_encoding(&fec, NULL), 0);
	pwtest_int_eq((int)fec, (int)ROC_FEC_ENCODING_DEFAULT);

	pwtest_int_eq(pw_roc_parse_fec_encoding(&fec, ""), 0);
	pwtest_int_eq((int)fec, (int)ROC_FEC_ENCODING_DEFAULT);

	pwtest_int_eq(pw_roc_parse_fec_encoding(&fec, "default"), 0);
	pwtest_int_eq((int)fec, (int)ROC_FEC_ENCODING_DEFAULT);

	pwtest_int_eq(pw_roc_parse_fec_encoding(&fec, "disable"), 0);
	pwtest_int_eq((int)fec, (int)ROC_FEC_ENCODING_DISABLE);

	pwtest_int_eq(pw_roc_parse_fec_encoding(&fec, "rs8m"), 0);
	pwtest_int_eq((int)fec, (int)ROC_FEC_ENCODING_RS8M);

	pwtest_int_eq(pw_roc_parse_fec_encoding(&fec, "ldpc"), 0);
	pwtest_int_eq((int)fec, (int)ROC_FEC_ENCODING_LDPC_STAIRCASE);

	pwtest_int_eq(pw_roc_parse_fec_encoding(&fec, "invalid"), -EINVAL);

	return PWTEST_PASS;
}

PWTEST(test_parse_resampler_profile)
{
	roc_resampler_profile prof;

	pwtest_int_eq(pw_roc_parse_resampler_profile(&prof, "default"), 0);
	pwtest_int_eq((int)prof, (int)ROC_RESAMPLER_PROFILE_DEFAULT);

	pwtest_int_eq(pw_roc_parse_resampler_profile(&prof, "high"), 0);
	pwtest_int_eq((int)prof, (int)ROC_RESAMPLER_PROFILE_HIGH);

	pwtest_int_eq(pw_roc_parse_resampler_profile(&prof, "medium"), 0);
	pwtest_int_eq((int)prof, (int)ROC_RESAMPLER_PROFILE_MEDIUM);

	pwtest_int_eq(pw_roc_parse_resampler_profile(&prof, "low"), 0);
	pwtest_int_eq((int)prof, (int)ROC_RESAMPLER_PROFILE_LOW);

	pwtest_int_eq(pw_roc_parse_resampler_profile(&prof, "invalid"), -EINVAL);

	return PWTEST_PASS;
}

PWTEST(test_parse_resampler_backend)
{
	roc_resampler_backend be;

	pwtest_int_eq(pw_roc_parse_resampler_backend(&be, "default"), 0);
	pwtest_int_eq((int)be, (int)ROC_RESAMPLER_BACKEND_DEFAULT);

	pwtest_int_eq(pw_roc_parse_resampler_backend(&be, "builtin"), 0);
	pwtest_int_eq((int)be, (int)ROC_RESAMPLER_BACKEND_BUILTIN);

	pwtest_int_eq(pw_roc_parse_resampler_backend(&be, "speex"), 0);
	pwtest_int_eq((int)be, (int)ROC_RESAMPLER_BACKEND_SPEEX);

	pwtest_int_eq(pw_roc_parse_resampler_backend(&be, "speexdec"), 0);
	pwtest_int_eq((int)be, (int)ROC_RESAMPLER_BACKEND_SPEEXDEC);

	pwtest_int_eq(pw_roc_parse_resampler_backend(&be, "invalid"), -EINVAL);

	return PWTEST_PASS;
}

PWTEST(test_parse_latency_tuner_backend)
{
	roc_latency_tuner_backend be;

	pwtest_int_eq(pw_roc_parse_latency_tuner_backend(&be, "default"), 0);
	pwtest_int_eq((int)be, (int)ROC_LATENCY_TUNER_BACKEND_DEFAULT);

	pwtest_int_eq(pw_roc_parse_latency_tuner_backend(&be, "niq"), 0);
	pwtest_int_eq((int)be, (int)ROC_LATENCY_TUNER_BACKEND_NIQ);

	pwtest_int_eq(pw_roc_parse_latency_tuner_backend(&be, "invalid"), -EINVAL);

	return PWTEST_PASS;
}

PWTEST(test_parse_latency_tuner_profile)
{
	roc_latency_tuner_profile prof;

	pwtest_int_eq(pw_roc_parse_latency_tuner_profile(&prof, "default"), 0);
	pwtest_int_eq((int)prof, (int)ROC_LATENCY_TUNER_PROFILE_DEFAULT);

	pwtest_int_eq(pw_roc_parse_latency_tuner_profile(&prof, "intact"), 0);
	pwtest_int_eq((int)prof, (int)ROC_LATENCY_TUNER_PROFILE_INTACT);

	pwtest_int_eq(pw_roc_parse_latency_tuner_profile(&prof, "responsive"), 0);
	pwtest_int_eq((int)prof, (int)ROC_LATENCY_TUNER_PROFILE_RESPONSIVE);

	pwtest_int_eq(pw_roc_parse_latency_tuner_profile(&prof, "gradual"), 0);
	pwtest_int_eq((int)prof, (int)ROC_LATENCY_TUNER_PROFILE_GRADUAL);

	pwtest_int_eq(pw_roc_parse_latency_tuner_profile(&prof, "invalid"), -EINVAL);

	return PWTEST_PASS;
}

PWTEST(test_parse_log_level)
{
	roc_log_level lvl;

	pwtest_int_eq(pw_roc_parse_log_level(&lvl, "DEFAULT", ROC_LOG_INFO), 0);
	pwtest_int_eq((int)lvl, (int)ROC_LOG_INFO);

	pwtest_int_eq(pw_roc_parse_log_level(&lvl, "NONE", ROC_LOG_INFO), 0);
	pwtest_int_eq((int)lvl, (int)ROC_LOG_NONE);

	pwtest_int_eq(pw_roc_parse_log_level(&lvl, "ERROR", ROC_LOG_INFO), 0);
	pwtest_int_eq((int)lvl, (int)ROC_LOG_ERROR);

	pwtest_int_eq(pw_roc_parse_log_level(&lvl, "INFO", ROC_LOG_NONE), 0);
	pwtest_int_eq((int)lvl, (int)ROC_LOG_INFO);

	pwtest_int_eq(pw_roc_parse_log_level(&lvl, "DEBUG", ROC_LOG_NONE), 0);
	pwtest_int_eq((int)lvl, (int)ROC_LOG_DEBUG);

	pwtest_int_eq(pw_roc_parse_log_level(&lvl, "TRACE", ROC_LOG_NONE), 0);
	pwtest_int_eq((int)lvl, (int)ROC_LOG_TRACE);

	pwtest_int_eq(pw_roc_parse_log_level(&lvl, "invalid", ROC_LOG_NONE), -EINVAL);

	return PWTEST_PASS;
}

PWTEST(test_fec_encoding_to_proto)
{
	roc_protocol audio, repair;

	pw_roc_fec_encoding_to_proto(ROC_FEC_ENCODING_DISABLE, &audio, &repair);
	pwtest_int_eq((int)audio, (int)ROC_PROTO_RTP);
	pwtest_int_eq((int)repair, 0);

	pw_roc_fec_encoding_to_proto(ROC_FEC_ENCODING_RS8M, &audio, &repair);
	pwtest_int_eq((int)audio, (int)ROC_PROTO_RTP_RS8M_SOURCE);
	pwtest_int_eq((int)repair, (int)ROC_PROTO_RS8M_REPAIR);

	pw_roc_fec_encoding_to_proto(ROC_FEC_ENCODING_DEFAULT, &audio, &repair);
	pwtest_int_eq((int)audio, (int)ROC_PROTO_RTP_RS8M_SOURCE);
	pwtest_int_eq((int)repair, (int)ROC_PROTO_RS8M_REPAIR);

	pw_roc_fec_encoding_to_proto(ROC_FEC_ENCODING_LDPC_STAIRCASE, &audio, &repair);
	pwtest_int_eq((int)audio, (int)ROC_PROTO_RTP_LDPC_SOURCE);
	pwtest_int_eq((int)repair, (int)ROC_PROTO_LDPC_REPAIR);

	return PWTEST_PASS;
}

PWTEST(test_log_level_conversions)
{
	/* PW -> ROC */
	pwtest_int_eq((int)pw_roc_log_level_pw_2_roc(SPA_LOG_LEVEL_NONE), (int)ROC_LOG_NONE);
	pwtest_int_eq((int)pw_roc_log_level_pw_2_roc(SPA_LOG_LEVEL_ERROR), (int)ROC_LOG_ERROR);
	pwtest_int_eq((int)pw_roc_log_level_pw_2_roc(SPA_LOG_LEVEL_WARN), (int)ROC_LOG_ERROR);
	pwtest_int_eq((int)pw_roc_log_level_pw_2_roc(SPA_LOG_LEVEL_INFO), (int)ROC_LOG_INFO);
	pwtest_int_eq((int)pw_roc_log_level_pw_2_roc(SPA_LOG_LEVEL_DEBUG), (int)ROC_LOG_DEBUG);
	pwtest_int_eq((int)pw_roc_log_level_pw_2_roc(SPA_LOG_LEVEL_TRACE), (int)ROC_LOG_TRACE);

	/* ROC -> PW */
	pwtest_int_eq((int)pw_roc_log_level_roc_2_pw(ROC_LOG_NONE), (int)SPA_LOG_LEVEL_NONE);
	pwtest_int_eq((int)pw_roc_log_level_roc_2_pw(ROC_LOG_ERROR), (int)SPA_LOG_LEVEL_ERROR);
	pwtest_int_eq((int)pw_roc_log_level_roc_2_pw(ROC_LOG_INFO), (int)SPA_LOG_LEVEL_INFO);
	pwtest_int_eq((int)pw_roc_log_level_roc_2_pw(ROC_LOG_DEBUG), (int)SPA_LOG_LEVEL_DEBUG);
	pwtest_int_eq((int)pw_roc_log_level_roc_2_pw(ROC_LOG_TRACE), (int)SPA_LOG_LEVEL_TRACE);

	return PWTEST_PASS;
}

PWTEST(test_create_endpoint)
{
	roc_endpoint *ep = NULL;
	int ret;

	ret = pw_roc_create_endpoint(&ep, ROC_PROTO_RTP, "127.0.0.1", 10001);
	pwtest_int_eq(ret, 0);
	pwtest_ptr_notnull(ep);

	roc_endpoint_deallocate(ep);

	return PWTEST_PASS;
}

/*
 * Tier 2: roc-toolkit API integration tests
 */

PWTEST(test_context_lifecycle)
{
	roc_context *context = NULL;
	roc_context_config ctx_cfg;

	memset(&ctx_cfg, 0, sizeof(ctx_cfg));

	pwtest_int_eq(roc_context_open(&ctx_cfg, &context), 0);
	pwtest_ptr_notnull(context);

	pwtest_int_eq(roc_context_close(context), 0);

	return PWTEST_PASS;
}

PWTEST(test_custom_encoding_registration)
{
	roc_context *context = NULL;
	roc_context_config ctx_cfg;
	roc_media_encoding encoding;

	memset(&ctx_cfg, 0, sizeof(ctx_cfg));
	pwtest_int_eq(roc_context_open(&ctx_cfg, &context), 0);

	memset(&encoding, 0, sizeof(encoding));
	encoding.format = ROC_FORMAT_PCM;
	encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
	encoding.rate = 44100;
	encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;
	encoding.tracks = 0;

	pwtest_int_eq(roc_context_register_encoding(context,
		PW_ROC_CUSTOM_ENCODING_ID, &encoding), 0);

	pwtest_int_eq(roc_context_close(context), 0);

	return PWTEST_PASS;
}

PWTEST(test_encoding_formats)
{
	struct pwtest_test *t = current_test;
	int iteration = pwtest_get_iteration(t);

	static const struct {
		enum spa_audio_format spa_fmt;
		roc_subformat roc_subfmt;
		unsigned int rate;
		uint32_t channels;
		roc_channel_layout layout;
	} cases[] = {
		{ SPA_AUDIO_FORMAT_S16, ROC_SUBFORMAT_PCM_SINT16, 44100, 2, ROC_CHANNEL_LAYOUT_STEREO },
		{ SPA_AUDIO_FORMAT_S16, ROC_SUBFORMAT_PCM_SINT16, 48000, 1, ROC_CHANNEL_LAYOUT_MONO },
		{ SPA_AUDIO_FORMAT_F32, ROC_SUBFORMAT_PCM_FLOAT32, 44100, 2, ROC_CHANNEL_LAYOUT_STEREO },
		{ SPA_AUDIO_FORMAT_F32, ROC_SUBFORMAT_PCM_FLOAT32, 48000, 2, ROC_CHANNEL_LAYOUT_STEREO },
		{ SPA_AUDIO_FORMAT_S32, ROC_SUBFORMAT_PCM_SINT32, 44100, 2, ROC_CHANNEL_LAYOUT_STEREO },
	};

	roc_context *context = NULL;
	roc_context_config ctx_cfg;
	roc_media_encoding encoding;

	memset(&ctx_cfg, 0, sizeof(ctx_cfg));
	pwtest_int_eq(roc_context_open(&ctx_cfg, &context), 0);

	memset(&encoding, 0, sizeof(encoding));
	encoding.format = ROC_FORMAT_PCM;
	encoding.subformat = cases[iteration].roc_subfmt;
	encoding.rate = cases[iteration].rate;
	encoding.channels = cases[iteration].layout;
	encoding.tracks = 0;

	/* Use a different encoding ID per iteration to avoid conflicts */
	pwtest_int_eq(roc_context_register_encoding(context,
		PW_ROC_CUSTOM_ENCODING_ID + iteration, &encoding), 0);

	pwtest_int_eq(roc_context_close(context), 0);

	return PWTEST_PASS;
}

/*
 * Tier 3: Sender -> Receiver loopback tests
 */

struct loopback_config {
	enum spa_audio_format format;
	uint32_t rate;
	uint32_t channels;
	roc_fec_encoding fec;
	unsigned int num_iterations;
	int base_port;
};

struct loopback_ctx {
	roc_context *context;
	roc_sender *sender;
	roc_receiver *receiver;
	struct loopback_config config;
	void *send_buf;
	void *recv_buf;
	size_t buf_size;
	uint32_t stride;
	pthread_t recv_thread;
	int recv_error;
	unsigned int frames_received;
};

static void *receiver_thread_func(void *arg)
{
	struct loopback_ctx *ctx = arg;
	unsigned int i;
	roc_frame frame;

	memset(&frame, 0, sizeof(frame));
	frame.samples = ctx->recv_buf;
	frame.samples_size = ctx->buf_size;

	for (i = 0; i < ctx->config.num_iterations; i++) {
		if (roc_receiver_read(ctx->receiver, &frame) != 0) {
			ctx->recv_error = -1;
			return NULL;
		}
		ctx->frames_received++;
	}

	return NULL;
}

static int loopback_setup(struct loopback_ctx *ctx, const struct loopback_config *config)
{
	roc_context_config ctx_cfg;
	roc_sender_config sender_cfg;
	roc_receiver_config receiver_cfg;
	roc_media_encoding encoding;
	roc_endpoint *ep = NULL;
	roc_protocol audio_proto, repair_proto;
	uint32_t sample_size;
	int ret;

	memset(ctx, 0, sizeof(*ctx));
	ctx->config = *config;

	sample_size = pw_roc_spa_format_sample_size(config->format);
	if (sample_size == 0)
		return -EINVAL;

	ctx->stride = sample_size * config->channels;
	/* Buffer for 1024 samples per channel */
	ctx->buf_size = (size_t)ctx->stride * 1024;
	ctx->send_buf = calloc(1, ctx->buf_size);
	ctx->recv_buf = calloc(1, ctx->buf_size);
	if (!ctx->send_buf || !ctx->recv_buf)
		return -ENOMEM;

	/* Fill send buffer with a simple pattern */
	if (config->format == SPA_AUDIO_FORMAT_F32) {
		float *buf = ctx->send_buf;
		for (size_t i = 0; i < ctx->buf_size / sizeof(float); i++)
			buf[i] = 0.5f;
	} else {
		memset(ctx->send_buf, 0x42, ctx->buf_size);
	}

	/* Open context */
	memset(&ctx_cfg, 0, sizeof(ctx_cfg));
	ret = roc_context_open(&ctx_cfg, &ctx->context);
	if (ret != 0)
		return -1;

	/* Register custom encoding */
	roc_subformat subfmt;
	if (pw_roc_spa_format_to_roc(config->format, &subfmt) != 0)
		return -1;

	memset(&encoding, 0, sizeof(encoding));
	encoding.format = ROC_FORMAT_PCM;
	encoding.subformat = subfmt;
	encoding.rate = config->rate;
	encoding.channels = pw_roc_channels_to_layout(config->channels);
	if (encoding.channels == ROC_CHANNEL_LAYOUT_MULTITRACK)
		encoding.tracks = config->channels;

	ret = roc_context_register_encoding(ctx->context,
		PW_ROC_CUSTOM_ENCODING_ID, &encoding);
	if (ret != 0)
		return -1;

	/* Configure and open sender */
	memset(&sender_cfg, 0, sizeof(sender_cfg));
	sender_cfg.frame_encoding.format = ROC_FORMAT_PCM;
	sender_cfg.frame_encoding.subformat = subfmt;
	sender_cfg.frame_encoding.rate = config->rate;
	sender_cfg.frame_encoding.channels = pw_roc_channels_to_layout(config->channels);
	if (sender_cfg.frame_encoding.channels == ROC_CHANNEL_LAYOUT_MULTITRACK)
		sender_cfg.frame_encoding.tracks = config->channels;
	sender_cfg.packet_encoding = PW_ROC_CUSTOM_ENCODING_ID;
	sender_cfg.clock_source = ROC_CLOCK_SOURCE_INTERNAL;
	sender_cfg.fec_encoding = config->fec;

	ret = roc_sender_open(ctx->context, &sender_cfg, &ctx->sender);
	if (ret != 0)
		return -1;

	/* Configure and open receiver */
	memset(&receiver_cfg, 0, sizeof(receiver_cfg));
	receiver_cfg.frame_encoding.format = ROC_FORMAT_PCM;
	receiver_cfg.frame_encoding.subformat = subfmt;
	receiver_cfg.frame_encoding.rate = config->rate;
	receiver_cfg.frame_encoding.channels = pw_roc_channels_to_layout(config->channels);
	if (receiver_cfg.frame_encoding.channels == ROC_CHANNEL_LAYOUT_MULTITRACK)
		receiver_cfg.frame_encoding.tracks = config->channels;
	receiver_cfg.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

	ret = roc_receiver_open(ctx->context, &receiver_cfg, &ctx->receiver);
	if (ret != 0)
		return -1;

	pw_roc_fec_encoding_to_proto(config->fec, &audio_proto, &repair_proto);

	/* Bind receiver source endpoint */
	ret = pw_roc_create_endpoint(&ep, audio_proto, "127.0.0.1", config->base_port);
	if (ret != 0)
		return -1;
	ret = roc_receiver_bind(ctx->receiver, ROC_SLOT_DEFAULT,
		ROC_INTERFACE_AUDIO_SOURCE, ep);
	roc_endpoint_deallocate(ep);
	if (ret != 0)
		return -1;

	/* Bind receiver repair endpoint if FEC is enabled */
	if (repair_proto != 0) {
		ret = pw_roc_create_endpoint(&ep, repair_proto,
			"127.0.0.1", config->base_port + 1);
		if (ret != 0)
			return -1;
		ret = roc_receiver_bind(ctx->receiver, ROC_SLOT_DEFAULT,
			ROC_INTERFACE_AUDIO_REPAIR, ep);
		roc_endpoint_deallocate(ep);
		if (ret != 0)
			return -1;
	}

	/* Bind receiver control endpoint */
	ret = pw_roc_create_endpoint(&ep, ROC_PROTO_RTCP,
		"127.0.0.1", config->base_port + 2);
	if (ret != 0)
		return -1;
	ret = roc_receiver_bind(ctx->receiver, ROC_SLOT_DEFAULT,
		ROC_INTERFACE_AUDIO_CONTROL, ep);
	roc_endpoint_deallocate(ep);
	if (ret != 0)
		return -1;

	/* Connect sender source endpoint */
	ret = pw_roc_create_endpoint(&ep, audio_proto, "127.0.0.1", config->base_port);
	if (ret != 0)
		return -1;
	ret = roc_sender_connect(ctx->sender, ROC_SLOT_DEFAULT,
		ROC_INTERFACE_AUDIO_SOURCE, ep);
	roc_endpoint_deallocate(ep);
	if (ret != 0)
		return -1;

	/* Connect sender repair endpoint if FEC is enabled */
	if (repair_proto != 0) {
		ret = pw_roc_create_endpoint(&ep, repair_proto,
			"127.0.0.1", config->base_port + 1);
		if (ret != 0)
			return -1;
		ret = roc_sender_connect(ctx->sender, ROC_SLOT_DEFAULT,
			ROC_INTERFACE_AUDIO_REPAIR, ep);
		roc_endpoint_deallocate(ep);
		if (ret != 0)
			return -1;
	}

	/* Connect sender control endpoint */
	ret = pw_roc_create_endpoint(&ep, ROC_PROTO_RTCP,
		"127.0.0.1", config->base_port + 2);
	if (ret != 0)
		return -1;
	ret = roc_sender_connect(ctx->sender, ROC_SLOT_DEFAULT,
		ROC_INTERFACE_AUDIO_CONTROL, ep);
	roc_endpoint_deallocate(ep);
	if (ret != 0)
		return -1;

	return 0;
}

static void loopback_teardown(struct loopback_ctx *ctx)
{
	if (ctx->sender)
		roc_sender_close(ctx->sender);
	if (ctx->receiver)
		roc_receiver_close(ctx->receiver);
	if (ctx->context)
		roc_context_close(ctx->context);
	free(ctx->send_buf);
	free(ctx->recv_buf);
}

static enum pwtest_result run_loopback_test(const struct loopback_config *config)
{
	struct loopback_ctx ctx;
	roc_frame frame;
	unsigned int i;
	int ret;

	ret = loopback_setup(&ctx, config);
	if (ret != 0) {
		loopback_teardown(&ctx);
		return PWTEST_FAIL;
	}

	/* Start receiver thread */
	ret = pthread_create(&ctx.recv_thread, NULL, receiver_thread_func, &ctx);
	if (ret != 0) {
		loopback_teardown(&ctx);
		return PWTEST_SYSTEM_ERROR;
	}

	/* Brief delay to let receiver start listening */
	usleep(100000);

	/* Write frames to sender */
	memset(&frame, 0, sizeof(frame));
	frame.samples = ctx.send_buf;
	frame.samples_size = ctx.buf_size;

	for (i = 0; i < config->num_iterations; i++) {
		ret = roc_sender_write(ctx.sender, &frame);
		if (ret != 0) {
			pthread_join(ctx.recv_thread, NULL);
			loopback_teardown(&ctx);
			return PWTEST_FAIL;
		}
	}

	pthread_join(ctx.recv_thread, NULL);

	pwtest_int_eq(ctx.recv_error, 0);
	pwtest_int_ge((int)ctx.frames_received, (int)config->num_iterations);

	loopback_teardown(&ctx);

	return PWTEST_PASS;
}

PWTEST(test_loopback_f32_stereo_44100)
{
	struct loopback_config config = {
		.format = SPA_AUDIO_FORMAT_F32,
		.rate = 44100,
		.channels = 2,
		.fec = ROC_FEC_ENCODING_DISABLE,
		.num_iterations = 10,
		.base_port = 10301,
	};

	return run_loopback_test(&config);
}

PWTEST(test_loopback_s16_stereo_48000)
{
	struct loopback_config config = {
		.format = SPA_AUDIO_FORMAT_S16,
		.rate = 48000,
		.channels = 2,
		.fec = ROC_FEC_ENCODING_DISABLE,
		.num_iterations = 10,
		.base_port = 10311,
	};

	return run_loopback_test(&config);
}

PWTEST(test_loopback_f32_mono_44100)
{
	struct loopback_config config = {
		.format = SPA_AUDIO_FORMAT_F32,
		.rate = 44100,
		.channels = 1,
		.fec = ROC_FEC_ENCODING_DISABLE,
		.num_iterations = 10,
		.base_port = 10321,
	};

	return run_loopback_test(&config);
}

PWTEST(test_loopback_with_fec)
{
	struct loopback_config config = {
		.format = SPA_AUDIO_FORMAT_F32,
		.rate = 44100,
		.channels = 2,
		.fec = ROC_FEC_ENCODING_RS8M,
		.num_iterations = 10,
		.base_port = 10331,
	};

	return run_loopback_test(&config);
}

/*
 * Tier 4: PipeWire module integration tests
 *
 * Load the actual module-roc-sink and module-roc-source shared libraries
 * via pw_context_load_module(), verifying that complete module initialization
 * works correctly with various configurations. Each test starts a PipeWire
 * daemon (PWTEST_ARG_DAEMON) so the module can connect to a core.
 *
 * This exercises: argument parsing, roc context creation, custom encoding
 * registration, sender/receiver configuration, endpoint creation and
 * connection/binding, and PipeWire stream creation.
 */

struct module_test_params {
	const char *format;
	uint32_t rate;
	uint32_t channels;
	const char *fec;
	const char *resampler_backend;
	const char *resampler_profile;
	const char *lt_backend;
	const char *lt_profile;
};

static const struct module_test_params sink_params[] = {
	/* Basic formats */
	{ "F32", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	{ "S16", 48000, 2, "disable", NULL, NULL, NULL, NULL },
	{ "S32", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	{ "F64", 48000, 2, "disable", NULL, NULL, NULL, NULL },
	{ "S24", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	/* Channel variations */
	{ "F32", 44100, 1, "disable", NULL, NULL, NULL, NULL },
	{ "F32", 44100, 4, "disable", NULL, NULL, NULL, NULL },
	/* FEC encodings */
	{ "F32", 44100, 2, "rs8m", NULL, NULL, NULL, NULL },
	{ "F32", 44100, 2, "ldpc", NULL, NULL, NULL, NULL },
	/* Resampler backends and profiles */
	{ "F32", 44100, 2, "disable", "builtin", "high", NULL, NULL },
	{ "F32", 48000, 2, "disable", "speex", "medium", NULL, NULL },
	{ "F32", 44100, 2, "disable", "speexdec", "low", NULL, NULL },
	/* Latency tuner backends and profiles */
	{ "F32", 44100, 2, "disable", NULL, NULL, "niq", "responsive" },
	{ "F32", 48000, 2, "rs8m", NULL, NULL, "niq", "gradual" },
	{ "S16", 48000, 2, "disable", NULL, NULL, NULL, "intact" },
};

static const struct module_test_params source_params[] = {
	/* Basic formats */
	{ "F32", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	{ "S16", 48000, 2, "disable", NULL, NULL, NULL, NULL },
	{ "S32", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	{ "F64", 48000, 2, "disable", NULL, NULL, NULL, NULL },
	{ "S24", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	/* Channel variations */
	{ "F32", 44100, 1, "disable", NULL, NULL, NULL, NULL },
	{ "F32", 48000, 4, "disable", NULL, NULL, NULL, NULL },
	/* FEC encodings */
	{ "F32", 44100, 2, "rs8m", NULL, NULL, NULL, NULL },
	{ "F32", 44100, 2, "ldpc", NULL, NULL, NULL, NULL },
	/* Resampler backends and profiles */
	{ "F32", 44100, 2, "disable", "builtin", "high", NULL, NULL },
	{ "F32", 48000, 2, "disable", "speex", "medium", NULL, NULL },
	{ "F32", 44100, 2, "disable", "speexdec", "low", NULL, NULL },
	/* Latency tuner backends and profiles */
	{ "F32", 44100, 2, "disable", NULL, NULL, "niq", "responsive" },
	{ "F32", 48000, 2, "rs8m", NULL, NULL, "niq", "gradual" },
	{ "S16", 48000, 2, "disable", NULL, NULL, NULL, "intact" },
};

static void append_optional_args(char *buf, size_t bufsize, int *len,
		const struct module_test_params *p)
{
	if (p->fec)
		*len += snprintf(buf + *len, bufsize - *len,
			" fec.code=%s", p->fec);
	if (p->resampler_backend)
		*len += snprintf(buf + *len, bufsize - *len,
			" roc.resampler.backend=%s", p->resampler_backend);
	if (p->resampler_profile)
		*len += snprintf(buf + *len, bufsize - *len,
			" roc.resampler.profile=%s", p->resampler_profile);
	if (p->lt_backend)
		*len += snprintf(buf + *len, bufsize - *len,
			" roc.latency-tuner.backend=%s", p->lt_backend);
	if (p->lt_profile)
		*len += snprintf(buf + *len, bufsize - *len,
			" roc.latency-tuner.profile=%s", p->lt_profile);
}

static char *build_sink_args(const struct module_test_params *p)
{
	char buf[2048];
	int len = 0;

	len += snprintf(buf, sizeof(buf),
		"remote.ip=127.0.0.1 "
		"remote.source.port=10500 "
		"remote.repair.port=10501 "
		"remote.control.port=10502 "
		"sink.props = { audio.format=%s audio.rate=%u audio.channels=%u }",
		p->format, p->rate, p->channels);

	append_optional_args(buf, sizeof(buf), &len, p);
	return strdup(buf);
}

static char *build_source_args(const struct module_test_params *p)
{
	char buf[2048];
	int len = 0;

	len += snprintf(buf, sizeof(buf),
		"local.ip=127.0.0.1 "
		"local.source.port=10600 "
		"local.repair.port=10601 "
		"local.control.port=10602 "
		"sess.latency.msec=200 "
		"source.props = { audio.format=%s audio.rate=%u audio.channels=%u }",
		p->format, p->rate, p->channels);

	append_optional_args(buf, sizeof(buf), &len, p);
	return strdup(buf);
}

static enum pwtest_result run_module_load_test(const char *module_name,
		char *args)
{
	struct pw_main_loop *loop;
	struct pw_context *context;
	struct pw_impl_module *mod;

	pw_init(NULL, NULL);

	loop = pw_main_loop_new(NULL);
	if (!loop) {
		free(args);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	context = pw_context_new(pw_main_loop_get_loop(loop), NULL, 0);
	if (!context) {
		free(args);
		pw_main_loop_destroy(loop);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	mod = pw_context_load_module(context, module_name, args, NULL);
	free(args);

	pwtest_ptr_notnull(mod);

	pw_impl_module_destroy(mod);
	pw_context_destroy(context);
	pw_main_loop_destroy(loop);
	pw_deinit();

	return PWTEST_PASS;
}

PWTEST(test_module_sink_load)
{
	struct pwtest_test *t = current_test;
	int idx = pwtest_get_iteration(t);
	const struct module_test_params *p = &sink_params[idx];

	return run_module_load_test("libpipewire-module-roc-sink",
		build_sink_args(p));
}

PWTEST(test_module_source_load)
{
	struct pwtest_test *t = current_test;
	int idx = pwtest_get_iteration(t);
	const struct module_test_params *p = &source_params[idx];

	return run_module_load_test("libpipewire-module-roc-source",
		build_source_args(p));
}

PWTEST(test_module_sink_missing_ip)
{
	struct pw_main_loop *loop;
	struct pw_context *context;
	struct pw_impl_module *mod;

	pw_init(NULL, NULL);

	loop = pw_main_loop_new(NULL);
	pwtest_ptr_notnull(loop);

	context = pw_context_new(pw_main_loop_get_loop(loop), NULL, 0);
	pwtest_ptr_notnull(context);

	/* Sink requires remote.ip - omitting it should fail */
	mod = pw_context_load_module(context,
		"libpipewire-module-roc-sink",
		"fec.code=disable", NULL);
	pwtest_ptr_null(mod);

	pw_context_destroy(context);
	pw_main_loop_destroy(loop);
	pw_deinit();

	return PWTEST_PASS;
}

PWTEST(test_module_source_defaults)
{
	/* Source should load with all defaults (empty args) */
	return run_module_load_test("libpipewire-module-roc-source",
		strdup(""));
}

/*
 * Tier 4: Data flow tests
 *
 * Each loaded PipeWire module is paired with a libroc counterpart on the test
 * side.  Audio data is pushed through the module and verified via roc metrics
 * and non-zero sample detection.
 *
 * Sink test data path:
 *   Test PW source stream -> PW daemon -> module-roc-sink (roc_sender) ->
 *   UDP localhost -> Test roc_receiver -> roc_receiver_read()
 *
 * Source test data path:
 *   Test roc_sender -> roc_sender_write() -> UDP localhost ->
 *   module-roc-source (roc_receiver) -> PW daemon -> Test PW sink stream
 */

static enum spa_audio_format format_str_to_spa(const char *fmt)
{
	if (spa_streq(fmt, "F32"))
		return SPA_AUDIO_FORMAT_F32;
	if (spa_streq(fmt, "S16"))
		return SPA_AUDIO_FORMAT_S16;
	if (spa_streq(fmt, "S32"))
		return SPA_AUDIO_FORMAT_S32;
	if (spa_streq(fmt, "S24"))
		return SPA_AUDIO_FORMAT_S24;
	if (spa_streq(fmt, "F64"))
		return SPA_AUDIO_FORMAT_F64;
	return SPA_AUDIO_FORMAT_UNKNOWN;
}

static void fill_test_pattern(void *buf, size_t size, enum spa_audio_format fmt)
{
	if (fmt == SPA_AUDIO_FORMAT_F32) {
		float *p = buf;
		for (size_t i = 0; i < size / sizeof(float); i++)
			p[i] = 0.5f;
	} else if (fmt == SPA_AUDIO_FORMAT_F64) {
		double *p = buf;
		for (size_t i = 0; i < size / sizeof(double); i++)
			p[i] = 0.5;
	} else {
		memset(buf, 0x42, size);
	}
}

static int check_non_zero(const void *buf, size_t size)
{
	const unsigned char *p = buf;
	for (size_t i = 0; i < size; i++) {
		if (p[i] != 0)
			return 1;
	}
	return 0;
}

struct data_test_ctx {
	const struct module_test_params *params;
	/* PW objects */
	struct pw_main_loop *loop;
	struct pw_context *context;
	struct pw_impl_module *module;
	struct pw_stream *stream;
	struct spa_hook stream_listener;
	/* ROC counterpart (test side) */
	roc_context *roc_ctx;
	roc_sender *roc_sndr;        /* source test only */
	roc_receiver *roc_recv;      /* sink test only */
	pthread_t roc_thread;
	/* Audio params */
	uint32_t stride;
	enum spa_audio_format spa_fmt;
	/* State / results */
	enum pw_stream_state stream_state;
	unsigned int pw_frames_processed;
	unsigned int roc_frames_processed;
	int roc_errors;
	int non_zero_samples_seen;
	int done;
};

static int setup_test_roc_context(struct data_test_ctx *ctx)
{
	roc_context_config ctx_cfg;
	roc_media_encoding encoding;
	roc_subformat subfmt;

	memset(&ctx_cfg, 0, sizeof(ctx_cfg));
	if (roc_context_open(&ctx_cfg, &ctx->roc_ctx) != 0)
		return -1;

	if (pw_roc_spa_format_to_roc(ctx->spa_fmt, &subfmt) != 0)
		return -1;

	memset(&encoding, 0, sizeof(encoding));
	encoding.format = ROC_FORMAT_PCM;
	encoding.subformat = subfmt;
	encoding.rate = ctx->params->rate;
	encoding.channels = pw_roc_channels_to_layout(ctx->params->channels);
	if (encoding.channels == ROC_CHANNEL_LAYOUT_MULTITRACK)
		encoding.tracks = ctx->params->channels;

	if (roc_context_register_encoding(ctx->roc_ctx,
			PW_ROC_CUSTOM_ENCODING_ID, &encoding) != 0)
		return -1;

	return 0;
}

static int setup_test_roc_receiver(struct data_test_ctx *ctx, int base_port)
{
	roc_receiver_config cfg;
	roc_subformat subfmt = (roc_subformat)0;
	roc_endpoint *ep = NULL;
	roc_fec_encoding fec = (roc_fec_encoding)0;
	roc_protocol audio_proto, repair_proto;

	pw_roc_spa_format_to_roc(ctx->spa_fmt, &subfmt);
	pw_roc_parse_fec_encoding(&fec, ctx->params->fec);

	memset(&cfg, 0, sizeof(cfg));
	cfg.frame_encoding.format = ROC_FORMAT_PCM;
	cfg.frame_encoding.subformat = subfmt;
	cfg.frame_encoding.rate = ctx->params->rate;
	cfg.frame_encoding.channels = pw_roc_channels_to_layout(ctx->params->channels);
	if (cfg.frame_encoding.channels == ROC_CHANNEL_LAYOUT_MULTITRACK)
		cfg.frame_encoding.tracks = ctx->params->channels;
	cfg.clock_source = ROC_CLOCK_SOURCE_INTERNAL;
	cfg.target_latency = 200000000ULL; /* 200ms in nanoseconds */

	if (roc_receiver_open(ctx->roc_ctx, &cfg, &ctx->roc_recv) != 0)
		return -1;

	pw_roc_fec_encoding_to_proto(fec, &audio_proto, &repair_proto);

	/* Bind source endpoint */
	if (pw_roc_create_endpoint(&ep, audio_proto, "127.0.0.1", base_port) != 0)
		return -1;
	if (roc_receiver_bind(ctx->roc_recv, ROC_SLOT_DEFAULT,
			ROC_INTERFACE_AUDIO_SOURCE, ep) != 0) {
		roc_endpoint_deallocate(ep);
		return -1;
	}
	roc_endpoint_deallocate(ep);

	/* Bind repair endpoint if FEC enabled */
	if (repair_proto != 0) {
		if (pw_roc_create_endpoint(&ep, repair_proto,
				"127.0.0.1", base_port + 1) != 0)
			return -1;
		if (roc_receiver_bind(ctx->roc_recv, ROC_SLOT_DEFAULT,
				ROC_INTERFACE_AUDIO_REPAIR, ep) != 0) {
			roc_endpoint_deallocate(ep);
			return -1;
		}
		roc_endpoint_deallocate(ep);
	}

	/* Bind control endpoint */
	if (pw_roc_create_endpoint(&ep, ROC_PROTO_RTCP,
			"127.0.0.1", base_port + 2) != 0)
		return -1;
	if (roc_receiver_bind(ctx->roc_recv, ROC_SLOT_DEFAULT,
			ROC_INTERFACE_AUDIO_CONTROL, ep) != 0) {
		roc_endpoint_deallocate(ep);
		return -1;
	}
	roc_endpoint_deallocate(ep);

	return 0;
}

static int setup_test_roc_sender(struct data_test_ctx *ctx, int base_port)
{
	roc_sender_config cfg;
	roc_subformat subfmt = (roc_subformat)0;
	roc_endpoint *ep = NULL;
	roc_fec_encoding fec = (roc_fec_encoding)0;
	roc_protocol audio_proto, repair_proto;

	pw_roc_spa_format_to_roc(ctx->spa_fmt, &subfmt);
	pw_roc_parse_fec_encoding(&fec, ctx->params->fec);

	memset(&cfg, 0, sizeof(cfg));
	cfg.frame_encoding.format = ROC_FORMAT_PCM;
	cfg.frame_encoding.subformat = subfmt;
	cfg.frame_encoding.rate = ctx->params->rate;
	cfg.frame_encoding.channels = pw_roc_channels_to_layout(ctx->params->channels);
	if (cfg.frame_encoding.channels == ROC_CHANNEL_LAYOUT_MULTITRACK)
		cfg.frame_encoding.tracks = ctx->params->channels;
	cfg.packet_encoding = PW_ROC_CUSTOM_ENCODING_ID;
	cfg.clock_source = ROC_CLOCK_SOURCE_INTERNAL;
	cfg.fec_encoding = fec;

	if (roc_sender_open(ctx->roc_ctx, &cfg, &ctx->roc_sndr) != 0)
		return -1;

	pw_roc_fec_encoding_to_proto(fec, &audio_proto, &repair_proto);

	/* Connect source endpoint */
	if (pw_roc_create_endpoint(&ep, audio_proto, "127.0.0.1", base_port) != 0)
		return -1;
	if (roc_sender_connect(ctx->roc_sndr, ROC_SLOT_DEFAULT,
			ROC_INTERFACE_AUDIO_SOURCE, ep) != 0) {
		roc_endpoint_deallocate(ep);
		return -1;
	}
	roc_endpoint_deallocate(ep);

	/* Connect repair endpoint if FEC enabled */
	if (repair_proto != 0) {
		if (pw_roc_create_endpoint(&ep, repair_proto,
				"127.0.0.1", base_port + 1) != 0)
			return -1;
		if (roc_sender_connect(ctx->roc_sndr, ROC_SLOT_DEFAULT,
				ROC_INTERFACE_AUDIO_REPAIR, ep) != 0) {
			roc_endpoint_deallocate(ep);
			return -1;
		}
		roc_endpoint_deallocate(ep);
	}

	/* Connect control endpoint */
	if (pw_roc_create_endpoint(&ep, ROC_PROTO_RTCP,
			"127.0.0.1", base_port + 2) != 0)
		return -1;
	if (roc_sender_connect(ctx->roc_sndr, ROC_SLOT_DEFAULT,
			ROC_INTERFACE_AUDIO_CONTROL, ep) != 0) {
		roc_endpoint_deallocate(ep);
		return -1;
	}
	roc_endpoint_deallocate(ep);

	return 0;
}

/* PW stream callback: test source for sink data test (produces audio) */
static void test_source_process(void *userdata)
{
	struct data_test_ctx *ctx = userdata;
	struct pw_buffer *b;
	struct spa_buffer *buf;

	b = pw_stream_dequeue_buffer(ctx->stream);
	if (!b)
		return;

	buf = b->buffer;
	if (buf->datas[0].data == NULL) {
		pw_stream_queue_buffer(ctx->stream, b);
		return;
	}

	uint32_t n_frames = buf->datas[0].maxsize / ctx->stride;
	if (b->requested)
		n_frames = SPA_MIN(n_frames, (uint32_t)b->requested);

	fill_test_pattern(buf->datas[0].data, n_frames * ctx->stride, ctx->spa_fmt);

	buf->datas[0].chunk->offset = 0;
	buf->datas[0].chunk->stride = ctx->stride;
	buf->datas[0].chunk->size = n_frames * ctx->stride;

	pw_stream_queue_buffer(ctx->stream, b);
	ctx->pw_frames_processed++;
}

/* PW stream callback: test sink for source data test (consumes audio) */
static void test_sink_process(void *userdata)
{
	struct data_test_ctx *ctx = userdata;
	struct pw_buffer *b;
	struct spa_buffer *buf;

	b = pw_stream_dequeue_buffer(ctx->stream);
	if (!b)
		return;

	buf = b->buffer;
	if (buf->datas[0].data != NULL && buf->datas[0].chunk->size > 0) {
		if (check_non_zero(buf->datas[0].data, buf->datas[0].chunk->size))
			ctx->non_zero_samples_seen = 1;
	}

	pw_stream_queue_buffer(ctx->stream, b);
	ctx->pw_frames_processed++;
}

static void test_stream_state_changed(void *userdata, enum pw_stream_state old,
		enum pw_stream_state state, const char *error)
{
	struct data_test_ctx *ctx = userdata;
	ctx->stream_state = state;
}

static const struct pw_stream_events test_source_events = {
	PW_VERSION_STREAM_EVENTS,
	.process = test_source_process,
	.state_changed = test_stream_state_changed,
};

static const struct pw_stream_events test_sink_events = {
	PW_VERSION_STREAM_EVENTS,
	.process = test_sink_process,
	.state_changed = test_stream_state_changed,
};

/* ROC receiver thread for sink data test */
static void *roc_receiver_thread(void *arg)
{
	struct data_test_ctx *ctx = arg;
	size_t buf_size = (size_t)ctx->stride * 1024;
	void *buf = calloc(1, buf_size);
	roc_frame frame;
	unsigned int i;

	if (!buf) {
		ctx->roc_errors = 1;
		return NULL;
	}

	memset(&frame, 0, sizeof(frame));
	frame.samples = buf;
	frame.samples_size = buf_size;

	for (i = 0; i < 100 && !ctx->done; i++) {
		if (roc_receiver_read(ctx->roc_recv, &frame) != 0) {
			ctx->roc_errors++;
			break;
		}
		ctx->roc_frames_processed++;
		if (check_non_zero(buf, buf_size))
			ctx->non_zero_samples_seen = 1;
	}

	free(buf);
	return NULL;
}

/* ROC sender thread for source data test */
static void *roc_sender_thread(void *arg)
{
	struct data_test_ctx *ctx = arg;
	size_t buf_size = (size_t)ctx->stride * 1024;
	void *buf = calloc(1, buf_size);
	roc_frame frame;
	unsigned int i;

	if (!buf) {
		ctx->roc_errors = 1;
		return NULL;
	}

	fill_test_pattern(buf, buf_size, ctx->spa_fmt);

	memset(&frame, 0, sizeof(frame));
	frame.samples = buf;
	frame.samples_size = buf_size;

	for (i = 0; i < 200 && !ctx->done; i++) {
		if (roc_sender_write(ctx->roc_sndr, &frame) != 0) {
			ctx->roc_errors++;
			break;
		}
		ctx->roc_frames_processed++;
	}

	free(buf);
	return NULL;
}

static char *build_sink_data_args(const struct module_test_params *p, int base_port)
{
	char buf[2048];
	int len = 0;

	len += snprintf(buf, sizeof(buf),
		"remote.ip=127.0.0.1 "
		"remote.source.port=%d "
		"remote.repair.port=%d "
		"remote.control.port=%d "
		"sink.props = { audio.format=%s audio.rate=%u audio.channels=%u "
		"node.name=roc-sink }",
		base_port, base_port + 1, base_port + 2,
		p->format, p->rate, p->channels);

	append_optional_args(buf, sizeof(buf), &len, p);
	return strdup(buf);
}

static char *build_source_data_args(const struct module_test_params *p, int base_port)
{
	char buf[2048];
	int len = 0;

	len += snprintf(buf, sizeof(buf),
		"local.ip=127.0.0.1 "
		"local.source.port=%d "
		"local.repair.port=%d "
		"local.control.port=%d "
		"sess.latency.msec=200 "
		"source.props = { audio.format=%s audio.rate=%u audio.channels=%u "
		"node.name=roc-source }",
		base_port, base_port + 1, base_port + 2,
		p->format, p->rate, p->channels);

	append_optional_args(buf, sizeof(buf), &len, p);
	return strdup(buf);
}

static void data_test_cleanup(struct data_test_ctx *ctx)
{
	if (ctx->stream)
		pw_stream_destroy(ctx->stream);
	if (ctx->module)
		pw_impl_module_destroy(ctx->module);
	if (ctx->roc_recv)
		roc_receiver_close(ctx->roc_recv);
	if (ctx->roc_sndr)
		roc_sender_close(ctx->roc_sndr);
	if (ctx->roc_ctx)
		roc_context_close(ctx->roc_ctx);
	if (ctx->context)
		pw_context_destroy(ctx->context);
	if (ctx->loop)
		pw_main_loop_destroy(ctx->loop);
}

static double get_time_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static const struct module_test_params sink_data_params[] = {
	{ "F32", 44100, 2, "disable", NULL, NULL, NULL, NULL },   /* baseline */
	{ "S16", 48000, 2, "disable", NULL, NULL, NULL, NULL },   /* different format+rate */
	{ "F32", 44100, 1, "disable", NULL, NULL, NULL, NULL },   /* mono */
	{ "F32", 44100, 2, "rs8m",   NULL, NULL, NULL, NULL },    /* FEC rs8m */
	{ "S32", 44100, 2, "disable", NULL, NULL, NULL, NULL },   /* S32 format */
	{ "F32", 48000, 2, "disable", "builtin", "low", NULL, NULL }, /* resampler */
};

static const struct module_test_params source_data_params[] = {
	{ "F32", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	{ "S16", 48000, 2, "disable", NULL, NULL, NULL, NULL },
	{ "F32", 44100, 1, "disable", NULL, NULL, NULL, NULL },
	{ "F32", 44100, 2, "rs8m",   NULL, NULL, NULL, NULL },
	{ "S32", 44100, 2, "disable", NULL, NULL, NULL, NULL },
	{ "F32", 48000, 2, "disable", "builtin", "low", NULL, NULL },
};

static enum pwtest_result run_sink_data_test(const struct module_test_params *p,
		int base_port)
{
	struct data_test_ctx ctx;
	struct spa_audio_info_raw audio_info;
	const struct spa_pod *params[1];
	uint8_t pod_buf[1024];
	struct spa_pod_builder b;
	char *args;
	double t_start;

	memset(&ctx, 0, sizeof(ctx));
	ctx.params = p;
	ctx.spa_fmt = format_str_to_spa(p->format);
	ctx.stride = pw_roc_spa_format_sample_size(ctx.spa_fmt) * p->channels;

	pw_init(NULL, NULL);

	ctx.loop = pw_main_loop_new(NULL);
	if (!ctx.loop) {
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	ctx.context = pw_context_new(pw_main_loop_get_loop(ctx.loop), NULL, 0);
	if (!ctx.context) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	/* Set up test roc receiver first — it binds to ports */
	if (setup_test_roc_context(&ctx) != 0 ||
	    setup_test_roc_receiver(&ctx, base_port) != 0) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_FAIL;
	}

	/* Load the module — its internal sender connects to our receiver ports */
	args = build_sink_data_args(p, base_port);
	ctx.module = pw_context_load_module(ctx.context,
		"libpipewire-module-roc-sink", args, NULL);
	free(args);
	if (!ctx.module) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_FAIL;
	}

	/* Create test PW source stream to feed audio into the module */
	ctx.stream = pw_stream_new_simple(
		pw_main_loop_get_loop(ctx.loop),
		"test-source",
		pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Playback",
			PW_KEY_TARGET_OBJECT, "roc-sink",
			NULL),
		&test_source_events,
		&ctx);

	if (!ctx.stream) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	memset(&audio_info, 0, sizeof(audio_info));
	audio_info.format = ctx.spa_fmt;
	audio_info.rate = p->rate;
	audio_info.channels = p->channels;

	spa_pod_builder_init(&b, pod_buf, sizeof(pod_buf));
	params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audio_info);

	if (pw_stream_connect(ctx.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
			PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
			params, 1) < 0) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_FAIL;
	}

	/* Start roc receiver thread */
	if (pthread_create(&ctx.roc_thread, NULL, roc_receiver_thread, &ctx) != 0) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	/* Drive main loop with wall-clock timeout */
	t_start = get_time_sec();
	while (get_time_sec() - t_start < 5.0) {
		pw_loop_iterate(pw_main_loop_get_loop(ctx.loop), 100);
	}

	ctx.done = 1;
	pthread_join(ctx.roc_thread, NULL);

	/* Query metrics */
	{
		roc_receiver_metrics slot_m;
		roc_connection_metrics conn_m;
		size_t n = 1;

		memset(&slot_m, 0, sizeof(slot_m));
		memset(&conn_m, 0, sizeof(conn_m));

		roc_receiver_query(ctx.roc_recv, ROC_SLOT_DEFAULT,
			&slot_m, &conn_m, &n);

		/* Verify data flow */
		pwtest_int_gt((int)ctx.pw_frames_processed, 0);
		pwtest_int_gt((int)ctx.roc_frames_processed, 0);
		pwtest_int_eq(ctx.roc_errors, 0);
		pwtest_int_eq(ctx.non_zero_samples_seen, 1);
		pwtest_int_ge((int)slot_m.connection_count, 1);
		if (n > 0)
			pwtest_int_gt((int)conn_m.expected_packets, 0);
	}

	data_test_cleanup(&ctx);
	pw_deinit();

	return PWTEST_PASS;
}

static enum pwtest_result run_source_data_test(const struct module_test_params *p,
		int base_port)
{
	struct data_test_ctx ctx;
	struct spa_audio_info_raw audio_info;
	const struct spa_pod *params[1];
	uint8_t pod_buf[1024];
	struct spa_pod_builder b;
	char *args;
	double t_start;

	memset(&ctx, 0, sizeof(ctx));
	ctx.params = p;
	ctx.spa_fmt = format_str_to_spa(p->format);
	ctx.stride = pw_roc_spa_format_sample_size(ctx.spa_fmt) * p->channels;

	pw_init(NULL, NULL);

	ctx.loop = pw_main_loop_new(NULL);
	if (!ctx.loop) {
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	ctx.context = pw_context_new(pw_main_loop_get_loop(ctx.loop), NULL, 0);
	if (!ctx.context) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	/* Load the module first — its internal receiver binds to ports */
	args = build_source_data_args(p, base_port);
	ctx.module = pw_context_load_module(ctx.context,
		"libpipewire-module-roc-source", args, NULL);
	free(args);
	if (!ctx.module) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_FAIL;
	}

	/* Set up test roc sender — connects to module's receiver ports */
	if (setup_test_roc_context(&ctx) != 0 ||
	    setup_test_roc_sender(&ctx, base_port) != 0) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_FAIL;
	}

	/* Create test PW sink stream to receive audio from the module */
	ctx.stream = pw_stream_new_simple(
		pw_main_loop_get_loop(ctx.loop),
		"test-sink",
		pw_properties_new(
			PW_KEY_MEDIA_TYPE, "Audio",
			PW_KEY_MEDIA_CATEGORY, "Capture",
			PW_KEY_TARGET_OBJECT, "roc-source",
			NULL),
		&test_sink_events,
		&ctx);

	if (!ctx.stream) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	memset(&audio_info, 0, sizeof(audio_info));
	audio_info.format = ctx.spa_fmt;
	audio_info.rate = p->rate;
	audio_info.channels = p->channels;

	spa_pod_builder_init(&b, pod_buf, sizeof(pod_buf));
	params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audio_info);

	if (pw_stream_connect(ctx.stream, PW_DIRECTION_INPUT, PW_ID_ANY,
			PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
			params, 1) < 0) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_FAIL;
	}

	/* Start roc sender thread */
	if (pthread_create(&ctx.roc_thread, NULL, roc_sender_thread, &ctx) != 0) {
		data_test_cleanup(&ctx);
		pw_deinit();
		return PWTEST_SYSTEM_ERROR;
	}

	/* Drive main loop with wall-clock timeout (extra time for latency fill) */
	t_start = get_time_sec();
	while (get_time_sec() - t_start < 6.0) {
		pw_loop_iterate(pw_main_loop_get_loop(ctx.loop), 100);
	}

	ctx.done = 1;
	pthread_join(ctx.roc_thread, NULL);

	/* Query metrics */
	{
		roc_sender_metrics slot_m;
		roc_connection_metrics conn_m;
		size_t n = 1;

		memset(&slot_m, 0, sizeof(slot_m));
		memset(&conn_m, 0, sizeof(conn_m));

		roc_sender_query(ctx.roc_sndr, ROC_SLOT_DEFAULT,
			&slot_m, &conn_m, &n);

		/* Verify data flow */
		pwtest_int_gt((int)ctx.pw_frames_processed, 0);
		pwtest_int_gt((int)ctx.roc_frames_processed, 0);
		pwtest_int_eq(ctx.roc_errors, 0);
		pwtest_int_eq(ctx.non_zero_samples_seen, 1);
		pwtest_int_ge((int)slot_m.connection_count, 1);
		if (n > 0)
			pwtest_int_gt((int)conn_m.expected_packets, 0);
	}

	data_test_cleanup(&ctx);
	pw_deinit();

	return PWTEST_PASS;
}

PWTEST(test_module_sink_data)
{
	struct pwtest_test *t = current_test;
	int idx = pwtest_get_iteration(t);
	const struct module_test_params *p = &sink_data_params[idx];
	int base_port = 10700 + idx * 3;

	return run_sink_data_test(p, base_port);
}

PWTEST(test_module_source_data)
{
	struct pwtest_test *t = current_test;
	int idx = pwtest_get_iteration(t);
	const struct module_test_params *p = &source_data_params[idx];
	int base_port = 10800 + idx * 3;

	return run_source_data_test(p, base_port);
}

PWTEST_SUITE(roc)
{
	/* Tier 1: common.h helper unit tests */
	pwtest_add(test_spa_format_to_roc, PWTEST_NOARG);
	pwtest_add(test_spa_format_sample_size, PWTEST_NOARG);
	pwtest_add(test_channels_to_layout, PWTEST_NOARG);
	pwtest_add(test_parse_fec_encoding, PWTEST_NOARG);
	pwtest_add(test_parse_resampler_profile, PWTEST_NOARG);
	pwtest_add(test_parse_resampler_backend, PWTEST_NOARG);
	pwtest_add(test_parse_latency_tuner_backend, PWTEST_NOARG);
	pwtest_add(test_parse_latency_tuner_profile, PWTEST_NOARG);
	pwtest_add(test_parse_log_level, PWTEST_NOARG);
	pwtest_add(test_fec_encoding_to_proto, PWTEST_NOARG);
	pwtest_add(test_log_level_conversions, PWTEST_NOARG);
	pwtest_add(test_create_endpoint, PWTEST_NOARG);

	/* Tier 2: roc-toolkit API integration */
	pwtest_add(test_context_lifecycle, PWTEST_NOARG);
	pwtest_add(test_custom_encoding_registration, PWTEST_NOARG);
	pwtest_add(test_encoding_formats, PWTEST_ARG_RANGE, 0, 5);

	/* Tier 3: sender/receiver loopback */
	pwtest_add(test_loopback_f32_stereo_44100, PWTEST_NOARG);
	pwtest_add(test_loopback_s16_stereo_48000, PWTEST_NOARG);
	pwtest_add(test_loopback_f32_mono_44100, PWTEST_NOARG);
	pwtest_add(test_loopback_with_fec, PWTEST_NOARG);

	/* Tier 4: Module integration -- sink configurations */
	pwtest_add(test_module_sink_load,
		PWTEST_ARG_RANGE, 0, 15, PWTEST_ARG_DAEMON);

	/* Tier 4: Module integration -- source configurations */
	pwtest_add(test_module_source_load,
		PWTEST_ARG_RANGE, 0, 15, PWTEST_ARG_DAEMON);

	/* Tier 4: Module integration -- error cases */
	pwtest_add(test_module_sink_missing_ip, PWTEST_ARG_DAEMON);
	pwtest_add(test_module_source_defaults, PWTEST_ARG_DAEMON);

	/* Tier 4: Module data flow -- sink */
	pwtest_add(test_module_sink_data,
		PWTEST_ARG_RANGE, 0, 6, PWTEST_ARG_DAEMON);

	/* Tier 4: Module data flow -- source */
	pwtest_add(test_module_source_data,
		PWTEST_ARG_RANGE, 0, 6, PWTEST_ARG_DAEMON);

	return PWTEST_PASS;
}
