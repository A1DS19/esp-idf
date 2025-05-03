#include <esp_log.h>
#include <nvs_flash.h>
#include <wifi.h>

#include "aws_iot.h"
#include "dht11.h"
#include "esp_err.h"
#include "nvs.h"
#include "sntp_time_sync.h"
#include "wifi_reset_button.h"

static char TAG[] = "main";

void wifi_connected_events(void)
{
    ESP_LOGI(TAG, "wifi application connected");
    sntp_time_sync_task_start();
    aws_iot_start();
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    wifi_app_start();

    wifi_reset_button_config();

    DHT11_task_start();

    wifi_set_callback(&wifi_connected_events);
}
