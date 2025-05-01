#include <pipewire/log.h>

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
