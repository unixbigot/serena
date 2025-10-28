#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
//#include "esp_board_init.h"
#include "model_path.h"
// #include "esp_process_sdkconfig.h"
#include "nvs_flash.h"

#include "comms.h"
#include "serena_config.h"

#define TAG "serena"

extern esp_err_t esp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan);
extern esp_err_t esp_sdcard_init(char *mount_point, size_t max_files);
extern esp_err_t esp_sdcard_deinit(char *mount_point);
extern esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len);
extern int esp_get_feed_channel(void);

#include "commandset.h"

static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
srmodel_list_t *models = NULL;

void feed_Task(void *arg)
{
    ESP_LOGI(TAG, "%s\n", __func__);
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch <= feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag) {
        esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        afe_handle->feed(afe_data, i2s_buff);
    }
    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    ESP_LOGI(TAG, "%s\n", __func__);
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    ESP_LOGI(TAG, "audio frontend %p, chunksize %d\n", afe_handle, afe_chunksize);

    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    ESP_LOGI(TAG, "multinet: %s\n", mn_name);

    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    ESP_LOGI(TAG, "multinet interface:%p\n", multinet);
    
    model_iface_data_t *model_data = multinet->create(mn_name, 6000);
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    ESP_LOGI(TAG, "model data: %p, chunksize %d\n", model_data, mu_chunksize);
    assert(mu_chunksize == afe_chunksize);

    esp_err_t err;

    
    ESP_LOGI(TAG, "Commands alloc\n");
    err = esp_mn_commands_alloc(multinet, model_data);
    if (err != ESP_OK) {
	ESP_LOGI(TAG, "   command alloc failed: %d\n", (int)err);
    }
    
    err = set_commands("top");
    if (err != ESP_OK) {
	ESP_LOGI(TAG, "   top command set failed: %d\n", (int)err);
    }

    ESP_LOGI(TAG, "Active speech commands are\n");
    multinet->print_active_speech_commands(model_data);

    ESP_LOGI(TAG, "------------detect start------------\n");
    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGI(TAG, "fetch error!\n");
            break;
        }

	esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

	if (mn_state == ESP_MN_STATE_DETECTING) {
	    continue;
	}

	if (mn_state == ESP_MN_STATE_DETECTED) {
	    esp_mn_results_t *mn_result = multinet->get_results(model_data);
	    err = handle_command(mn_result);
	    if (err != ESP_OK) {
		ESP_LOGI(TAG, "  handle_command error %d\n", (int)err);
	    }
	}
    }
    if (model_data) {
        multinet->destroy(model_data);
        model_data = NULL;
    }
    ESP_LOGI(TAG, "detect exit\n");
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_LOGI(TAG, "%s\n", __func__);

    /* 
     * Log some info about the deivce we are on
     */
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);

    /* 
     * Invoke some essential esp-idf setup functions
     */
#if 0
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	ESP_LOGI(TAG, "Flash storage is scrambled, formatting");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err nvs_flash_init();
    }
#endif
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 
     * Establish Wi-Fi connection (see code in comms.c)
     */
    // 
    ESP_ERROR_CHECK(comms_wifi_init());
    ESP_ERROR_CHECK(comms_wifi_connect(WIFI_SSID, WIFI_PASSWORD));

    
    wifi_ap_record_t ap_info;
    ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_ERR_WIFI_CONN) {
        ESP_LOGE(TAG, "Wi-Fi station interface not initialized");
    }
    else if (ret == ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGE(TAG, "Wi-Fi station is not connected");
    } else {
        ESP_LOGI(TAG, "--- Access Point Information ---");
        ESP_LOG_BUFFER_HEX("MAC Address", ap_info.bssid, sizeof(ap_info.bssid));
        ESP_LOG_BUFFER_CHAR("SSID", ap_info.ssid, sizeof(ap_info.ssid));
        ESP_LOGI(TAG, "Primary Channel: %d", ap_info.primary);
        ESP_LOGI(TAG, "RSSI: %d", ap_info.rssi);

        ESP_LOGI(TAG, "Disconnecting in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }



    ESP_LOGI(TAG, "Load models\n");
    models = esp_srmodel_init("model"); // partition label defined in partitions.csv
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
#if CONFIG_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
#endif

    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;

    ESP_LOGI(TAG, "AFE config\n");
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();

    afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);;
    ESP_LOGI(TAG, "Wakenet name: %s\n", afe_config.wakenet_model_name?afe_config.wakenet_model_name:"(none)");

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);
    ESP_LOGI(TAG, "AFE handle %p\n", afe_data);

    task_flag = 1;
    ESP_LOGI(TAG, "Start detect task\n");
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 1);

    ESP_LOGI(TAG, "Start feed task\n");
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);


    ESP_LOGI(TAG, "Shutting down\n");
    // // You can call afe_handle->destroy to destroy AFE.
    // task_flag = 0;

    // ESP_LOGI(TAG, "destroy\n");
    // afe_handle->destroy(afe_data);
    // afe_data = NULL;
    // ESP_LOGI(TAG, "successful\n");

    ESP_ERROR_CHECK(comms_wifi_disconnect());
    ESP_ERROR_CHECK(comms_wifi_deinit());

    ESP_LOGI(TAG, "main() is done\n");


    
}

