
typedef struct commandset 
{
    const char *cs_name;
    const char *cs_parent;
    const char **cs_commands;
} commandset_t;

#define COMMAND_MAX 200

extern commandset_t *find_commands(const char *name);
extern esp_err_t set_commands(const char *name);

extern const char *command_names[COMMAND_MAX];
extern commandset_t *cs_active;

extern void commandset_handle_mqtt_data(const char *topic, const char *payload);
