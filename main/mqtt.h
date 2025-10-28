

typedef void (*data_handler_fp_t)(const char *t, const char *p);

extern esp_err_t mqtt_connect(const char *broker_uri, data_handler_fp_t data_handler);
extern esp_err_t mqtt_publish(const char *topic, const char *payload);
