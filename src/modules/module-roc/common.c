#include <pipewire/log.h>
#include <pipewire/properties.h>

#include <spa/param/audio/raw-json.h>
#include <spa/utils/dict.h>

#include "common.h"

#include <roc/log.h>
PW_LOG_TOPIC_EXTERN(mod_topic);
#define PW_LOG_TOPIC_DEFAULT mod_topic

void pw_roc_log_handler(const roc_log_message* message, void* argument)
{
    const enum spa_log_level log_level = pw_roc_log_level_roc_2_pw(message->level);
    if (SPA_UNLIKELY(pw_log_topic_enabled(log_level, mod_topic))) {
        pw_log_logt(log_level, mod_topic, message->file, message->line, message->module, message->text, "");
    }
}

void pw_roc_parse_audio_info(const struct pw_properties *props, struct spa_audio_info_raw *info)
{
	spa_audio_info_raw_init_dict_keys(info,
			&SPA_DICT_ITEMS(
				 SPA_DICT_ITEM(SPA_KEY_AUDIO_FORMAT, DEFAULT_FORMAT),
				 SPA_DICT_ITEM(SPA_KEY_AUDIO_RATE, SPA_STRINGIFY(PW_ROC_DEFAULT_RATE)),
				 SPA_DICT_ITEM(SPA_KEY_AUDIO_POSITION, DEFAULT_POSITION)),
			&props->dict,
			SPA_KEY_AUDIO_FORMAT,
			SPA_KEY_AUDIO_RATE,
			SPA_KEY_AUDIO_CHANNELS,
			SPA_KEY_AUDIO_POSITION, NULL);
}
