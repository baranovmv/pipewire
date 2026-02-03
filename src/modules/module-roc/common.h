#ifndef MODULE_ROC_COMMON_H
#define MODULE_ROC_COMMON_H

#include <roc/config.h>
#include <roc/endpoint.h>
#include <roc/log.h>

#include <spa/utils/string.h>
#include <spa/support/log.h>
#include <spa/param/audio/raw.h>

struct pw_properties;

#define PW_ROC_DEFAULT_IP "0.0.0.0"
#define PW_ROC_DEFAULT_SOURCE_PORT 10001
#define PW_ROC_DEFAULT_REPAIR_PORT 10002
#define PW_ROC_DEFAULT_CONTROL_PORT 10003
#define PW_ROC_DEFAULT_SESS_LATENCY 200
#define PW_ROC_DEFAULT_RATE 44100
#define PW_ROC_DEFAULT_CONTROL_PROTO ROC_PROTO_RTCP

#define DEFAULT_FORMAT "F32"
#define DEFAULT_POSITION "[ FL FR ]"

#define PW_ROC_CUSTOM_ENCODING_ID 100

void pw_roc_log_handler(const roc_log_message* message, void* argument);
void pw_roc_parse_audio_info(const struct pw_properties *props, struct spa_audio_info_raw *info);

/** Map SPA audio format to roc_subformat. Returns -1 for unsupported formats. */
static inline int pw_roc_spa_format_to_roc(enum spa_audio_format fmt, roc_subformat *out)
{
	switch (fmt) {
	case SPA_AUDIO_FORMAT_S16:
		*out = ROC_SUBFORMAT_PCM_SINT16;
		return 0;
	case SPA_AUDIO_FORMAT_S24:
		*out = ROC_SUBFORMAT_PCM_SINT24;
		return 0;
	case SPA_AUDIO_FORMAT_S32:
		*out = ROC_SUBFORMAT_PCM_SINT32;
		return 0;
	case SPA_AUDIO_FORMAT_F32:
		*out = ROC_SUBFORMAT_PCM_FLOAT32;
		return 0;
	case SPA_AUDIO_FORMAT_F64:
		*out = ROC_SUBFORMAT_PCM_FLOAT64;
		return 0;
	default:
		return -1;
	}
}

/** Return byte size per sample for the given SPA audio format. Returns 0 for unsupported. */
static inline uint32_t pw_roc_spa_format_sample_size(enum spa_audio_format fmt)
{
	switch (fmt) {
	case SPA_AUDIO_FORMAT_S16:
		return 2;
	case SPA_AUDIO_FORMAT_S24:
		return 3;
	case SPA_AUDIO_FORMAT_S32:
	case SPA_AUDIO_FORMAT_F32:
		return 4;
	case SPA_AUDIO_FORMAT_F64:
		return 8;
	default:
		return 0;
	}
}

/** Map channel count to roc_channel_layout. */
static inline roc_channel_layout pw_roc_channels_to_layout(uint32_t channels)
{
	switch (channels) {
	case 1:
		return ROC_CHANNEL_LAYOUT_MONO;
	case 2:
		return ROC_CHANNEL_LAYOUT_STEREO;
	default:
		return ROC_CHANNEL_LAYOUT_MULTITRACK;
	}
}

static inline int pw_roc_parse_fec_encoding(roc_fec_encoding *out, const char *str)
{
	if (!str || !*str || spa_streq(str, "default"))
		*out = ROC_FEC_ENCODING_DEFAULT;
	else if (spa_streq(str, "disable"))
		*out = ROC_FEC_ENCODING_DISABLE;
	else if (spa_streq(str, "rs8m"))
		*out = ROC_FEC_ENCODING_RS8M;
	else if (spa_streq(str, "ldpc"))
		*out = ROC_FEC_ENCODING_LDPC_STAIRCASE;
	else
		return -EINVAL;
	return 0;
}

static inline int pw_roc_parse_resampler_profile(roc_resampler_profile *out, const char *str)
{
	if (!str || !*str || spa_streq(str, "default"))
		*out = ROC_RESAMPLER_PROFILE_DEFAULT;
	else if (spa_streq(str, "high"))
		*out = ROC_RESAMPLER_PROFILE_HIGH;
	else if (spa_streq(str, "medium"))
		*out = ROC_RESAMPLER_PROFILE_MEDIUM;
	else if (spa_streq(str, "low"))
		*out = ROC_RESAMPLER_PROFILE_LOW;
	else
		return -EINVAL;
	return 0;
}

static inline int pw_roc_parse_resampler_backend(roc_resampler_backend *out, const char *str)
{
	if (!str || !*str || spa_streq(str, "default"))
		*out = ROC_RESAMPLER_BACKEND_DEFAULT;
	else if (spa_streq(str, "builtin"))
		*out = ROC_RESAMPLER_BACKEND_BUILTIN;
	else if (spa_streq(str, "speex"))
		*out = ROC_RESAMPLER_BACKEND_SPEEX;
	else if (spa_streq(str, "speexdec"))
		*out = ROC_RESAMPLER_BACKEND_SPEEXDEC;
	else
		return -EINVAL;
	return 0;
}

static inline int pw_roc_parse_latency_tuner_backend(roc_latency_tuner_backend *out, const char *str)
{
	if (!str || !*str || spa_streq(str, "default"))
		*out = ROC_LATENCY_TUNER_BACKEND_DEFAULT;
	else if (spa_streq(str, "niq"))
		*out = ROC_LATENCY_TUNER_BACKEND_NIQ;
	else
		return -EINVAL;
	return 0;
}

static inline int pw_roc_parse_latency_tuner_profile(roc_latency_tuner_profile *out, const char *str)
{
	if (!str || !*str || spa_streq(str, "default"))
		*out = ROC_LATENCY_TUNER_PROFILE_DEFAULT;
	else if (spa_streq(str, "intact"))
		*out = ROC_LATENCY_TUNER_PROFILE_INTACT;
	else if (spa_streq(str, "responsive"))
		*out = ROC_LATENCY_TUNER_PROFILE_RESPONSIVE;
	else if (spa_streq(str, "gradual"))
		*out = ROC_LATENCY_TUNER_PROFILE_GRADUAL;
	else
		return -EINVAL;
	return 0;
}

static inline int pw_roc_create_endpoint(roc_endpoint **result, roc_protocol protocol, const char *ip, int port)
{
	roc_endpoint *endpoint;

	if (roc_endpoint_allocate(&endpoint))
		return -ENOMEM;

	if (roc_endpoint_set_protocol(endpoint, protocol))
		goto out_error_free_ep;

	if (roc_endpoint_set_host(endpoint, ip))
		goto out_error_free_ep;

	if (roc_endpoint_set_port(endpoint, port))
		goto out_error_free_ep;

	*result = endpoint;
	return 0;

out_error_free_ep:
	(void) roc_endpoint_deallocate(endpoint);
	return -EINVAL;
}

static inline void pw_roc_fec_encoding_to_proto(roc_fec_encoding fec_code, roc_protocol *audio, roc_protocol *repair)
{
	switch (fec_code) {
	case ROC_FEC_ENCODING_DEFAULT:
	case ROC_FEC_ENCODING_RS8M:
		*audio = ROC_PROTO_RTP_RS8M_SOURCE;
		*repair = ROC_PROTO_RS8M_REPAIR;
		break;
	case ROC_FEC_ENCODING_LDPC_STAIRCASE:
		*audio = ROC_PROTO_RTP_LDPC_SOURCE;
		*repair = ROC_PROTO_LDPC_REPAIR;
		break;
	default:
		*audio = ROC_PROTO_RTP;
		*repair = 0;
		break;
	}
}

static inline roc_log_level pw_roc_log_level_pw_2_roc(const enum spa_log_level pw_log_level)
{
	if (pw_log_level == SPA_LOG_LEVEL_NONE)
		return ROC_LOG_NONE;
	else if (pw_log_level == SPA_LOG_LEVEL_ERROR)
		return ROC_LOG_ERROR;
	else if (pw_log_level == SPA_LOG_LEVEL_WARN)
		return ROC_LOG_ERROR;
	else if (pw_log_level == SPA_LOG_LEVEL_INFO)
		return ROC_LOG_INFO;
	else if (pw_log_level == SPA_LOG_LEVEL_DEBUG)
		return ROC_LOG_DEBUG;
	else if (pw_log_level == SPA_LOG_LEVEL_TRACE)
		return ROC_LOG_TRACE;
	else
		return ROC_LOG_NONE;
}

static inline enum spa_log_level pw_roc_log_level_roc_2_pw(const roc_log_level roc_log_level)
{
	if (roc_log_level == ROC_LOG_NONE)
		return SPA_LOG_LEVEL_NONE;
	else if (roc_log_level == ROC_LOG_ERROR)
		return SPA_LOG_LEVEL_ERROR;
	else if (roc_log_level == ROC_LOG_INFO)
		return SPA_LOG_LEVEL_INFO;
	else if (roc_log_level == ROC_LOG_DEBUG)
		return SPA_LOG_LEVEL_DEBUG;
	else if (roc_log_level == ROC_LOG_TRACE)
		return SPA_LOG_LEVEL_TRACE;
	else
		return SPA_LOG_LEVEL_NONE;
}

static inline int pw_roc_parse_log_level(roc_log_level *loglevel, const char *str,
                                         roc_log_level default_level)
{
	if (spa_streq(str, "DEFAULT"))
		*loglevel = default_level;
	else if (spa_streq(str, "NONE"))
		*loglevel = ROC_LOG_NONE;
	else if (spa_streq(str, "ERROR"))
		*loglevel = ROC_LOG_ERROR;
	else if (spa_streq(str, "INFO"))
		*loglevel = ROC_LOG_INFO;
	else if (spa_streq(str, "DEBUG"))
		*loglevel = ROC_LOG_DEBUG;
	else if (spa_streq(str, "TRACE"))
		*loglevel = ROC_LOG_TRACE;
	else
		return -EINVAL;
	return 0;
}

#endif /* MODULE_ROC_COMMON_H */
