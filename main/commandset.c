#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"

#include "model_path.h"
#include "commandset.h"

#define TAG "commandset"

static char *global_commands[] = {
    "start over",
    "all done now",
    "list commands",
    "what was i doing",
    NULL
};

static char *top_commands[] = {
    "microscope",
    "soldering",
    "visor",
    NULL
};

static commandset_t cs_top = {
    "top",
    NULL,
    top_commands,
    NULL
};

static char *solder_commands[]={
    "more feed",
    "less feed",
    "feed one",
    "feed two",
    "feed three",
    "feed four",
    "feed five",
    "small wire",
    "mid wire",
    "big wire",
    "hut",
    "retract",
    "hit me",
    NULL
};

static commandset_t cs_solder = {
    "solder",
    "top",
    solder_commands,
};

static char *scope_commands[]={
    "stop",
    "wait a minute",
    "go back",
    "track left",
    "track right",
    "tilt up",
    "tilt down",
    "enhance",
    "pull back",
    "give me a hardcopy",
    "thirteen",
    "fourteen",
    "fifteen",
    "sixteen",
    "seventeen",
    "eighteen",
    "nineteen",
    "twenty",
    "twenty one",
    "twenty two",
    "twenty three",
    "twenty four",
    "twenty five",
    "twenty six",
    "twenty seven",
    "twenty eight",
    "twenty nine",
    "thirty",
    NULL
};

static commandset_t cs_scope = {
    "scope",
    "top",
    scope_commands,
};

static char *visor_commands[]={
    "distance",
    "close up",
    "tilt up",
    "tilt down",
    NULL
};

static commandset_t cs_visor = {
    "visor",
    "top",
    visor_commands,
};

static commandset_t *cs_table[] = {
    &cs_top,
    &cs_solder,
    &cs_scope,
    &cs_visor,
    NULL
};
    
commandset_t *cs_active=NULL;
const char *command_names[COMMAND_MAX];

commandset_t *find_commands(const char *name) 
{
    for (int i=0; cs_table[i]; i++) {
	if (strcmp(cs_table[i]->cs_name, name)==0) return cs_table[i];
    }
    ESP_LOGI(TAG, "ERROR: no command set named '%s'", name);
    return NULL;
}

esp_err_t set_commands(const char *name) 
{
    esp_err_t err;
    
    ESP_LOGI(TAG, "SET COMMANDS %s\n", name);

    commandset_t *cs = find_commands(name);
    if (!cs) {
	return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "  clearing commands\n");
    err = esp_mn_commands_clear();
    if (err != ESP_OK) {
	ESP_LOGI(TAG, "   command clear failed: %d\n", (int)err);
	return err;
    }
#if 0
    err = esp_mn_commands_update();
    if (err != ESP_OK) {
	ESP_LOGI(TAG, "   command update failed: %d\n", (int)err);
    }
    esp_mn_commands_print();
    esp_mn_active_commands_print();
#endif
    
    ESP_LOGI(TAG, "  adding global commands :\n");
    int cmd_id = 0;
    for (int i=0; global_commands[i] != NULL; i++) {
	ESP_LOGI(TAG, "    global %d: %d[%s]...", i+1, cmd_id, global_commands[i]);
	err = esp_mn_commands_add(cmd_id, global_commands[i]);
	if (err == ESP_OK) {
	    ESP_LOGI(TAG, "      OK\n");
	}
	else {
	    ESP_LOGI(TAG, "      ERROR %d\n", (int)err);
	    return err;
	}
	command_names[cmd_id] = global_commands[i];
	++cmd_id;
    }

    ESP_LOGI(TAG, "  adding contextual commands:\n");
    for (int i=0; cs->cs_commands[i] != NULL; i++) {
	ESP_LOGI(TAG, "    context %d: %d[%s]...", i+1, cmd_id, cs->cs_commands[i]);
	err = esp_mn_commands_add(cmd_id, cs->cs_commands[i]);
	if (err == ESP_OK) {
	    ESP_LOGI(TAG, "      OK\n");
	}
	else {
	    ESP_LOGI(TAG, "      ERROR %d\n", (int)err);
	    return err;
	}
	command_names[cmd_id] = cs->cs_commands[i];
	++cmd_id;
    }
    ESP_LOGI(TAG, "  Activating commands\n");
    err = esp_mn_commands_update();
    if (err == ESP_OK) {
	ESP_LOGI(TAG, "    OK\n");
    }
    else {
	ESP_LOGI(TAG, "    ERROR %d\n", (int)err);
	return err;
    }
    esp_mn_commands_print();
    esp_mn_active_commands_print();
    cs_active = cs;

    return ESP_OK;
}

	    
esp_err_t handle_command(esp_mn_results_t *mn_result) 
{
    esp_err_t err = ESP_OK;
    const char *command_name=NULL;

    for (int i = 0; i < mn_result->num; i++) {
	int command_id = mn_result->command_id[i];
	int phrase_id = mn_result->phrase_id[i];
	command_name = command_names[command_id];
	ESP_LOGI(TAG, "handle_command: result #%d, cmd/phr %d/%d (%.1f%% %s \"%s\")\n", 
	       i+1, command_id, phrase_id, 100*mn_result->prob[i], command_name, mn_result->string);
	if (NULL == cs_active) {
	    ESP_LOGI(TAG, "WARNING: no active command set\n");
	    continue;
	}

	if (strcmp(command_name, "start over")==0) {
	    err = set_commands("top");
	}
	if ( (strcmp(command_name, "all done now")==0) &&
	     (cs_active->cs_parent)) {
	    err = set_commands(cs_active->cs_parent);
	}
	
	if (cs_active == &cs_top) {
	    if (strcmp(command_name, "soldering")==0) {
		err = set_commands("solder");
	    }
	    else if (strcmp(command_name, "microscope")==0) {
		err = set_commands("scope");
	    }
	}
	else if (cs_active == &cs_solder) {
	    ESP_LOGI(TAG, "SOLDERING ACTION: %s\n", command_name);
	}
	else if (cs_active == &cs_scope) {
	    ESP_LOGI(TAG, "MICROSCOPE ACTION: %s\n", command_name);
	}
	else if (cs_active == &cs_visor) {
	    ESP_LOGI(TAG, "VISOR ACTION: %s\n", command_name);
	}
    }

    if (err != ESP_OK) {
	ESP_LOGI(TAG, "handle_command: ERROR %d", (int)err);
    }
    return err;
}

