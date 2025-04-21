#include "nvs.h"

#include <esp_log.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_wifi_types_generic.h"
#include "wifi.h"

static const char TAG[] = "NVS";

// NVS name space for station mode credentials
const char app_nvs_sta_creds_namespace[] = "stacreds";

esp_err_t app_nvs_save_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;

    ESP_LOGI(TAG, "app_nvs_save_sta_creds: saving station mode credentials to flash");

    wifi_config_t *wifi_sta_config = wifi_get_config();
    if (wifi_sta_config)
    {
        esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
        if (esp_err != ESP_OK)
        {
            ESP_LOGE(TAG, "app_nvs_save_sta_creds: error (%s) opening NVS handle", esp_err_to_name(esp_err));
            return esp_err;
        }

        // set SSID
        esp_err = nvs_set_blob(handle, "ssid", wifi_sta_config->sta.ssid, MAX_SSID_LENGTH);
        if (esp_err != ESP_OK)
        {
            ESP_LOGE(TAG, "app_nvs_save_sta_creds: error (%s) setting SSID to NVS", esp_err_to_name(esp_err));
            return esp_err;
        }

        // set password
        esp_err = nvs_set_blob(handle, "password", wifi_sta_config->sta.password, MAX_PASSWORD_LENGTH);
        if (esp_err != ESP_OK)
        {
            ESP_LOGE(TAG, "app_nvs_save_sta_creds: error (%s) setting password to NVS", esp_err_to_name(esp_err));
            return esp_err;
        }

        // commit credentials to nvs
        esp_err = nvs_commit(handle);
        if (esp_err != ESP_OK)
        {
            ESP_LOGE(TAG, "app_nvs_save_sta_creds: error (%s) commiting credentials to NVS", esp_err_to_name(esp_err));
            return esp_err;
        }

        nvs_close(handle);
        ESP_LOGI(TAG,
                 "app_nvs_save_sta_creds: wrote wifi_sta_config: ssid: %s password: %s",
                 wifi_sta_config->sta.ssid,
                 wifi_sta_config->sta.password);
    }

    ESP_LOGI(TAG, "app_nvs_save_sta_creds: return ESP_OK");
    return ESP_OK;
}

bool app_nvs_load_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;

    ESP_LOGI(TAG, "app_nvs_load_sta_creds: loading wifi credentials from flash");
    if (nvs_open(app_nvs_sta_creds_namespace, NVS_READONLY, &handle) == ESP_OK)
    {
        wifi_config_t *wifi_sta_config = wifi_get_config();
        if (wifi_sta_config == NULL)
        {
            wifi_sta_config = (wifi_config_t *)malloc(sizeof(wifi_config_t));
        }
        memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));

        // Allocate buffer
        size_t wifi_config_size = sizeof(wifi_config_t);
        uint8_t *wifi_config_buff = (uint8_t *)malloc(sizeof(uint8_t) * wifi_config_size);
        memset(wifi_config_buff, 0x00, sizeof(wifi_config_size));

        // Load ssid
        wifi_config_size = sizeof(wifi_sta_config->sta.ssid);
        esp_err = nvs_get_blob(handle, "ssid", wifi_config_buff, &wifi_config_size);
        if (esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            ESP_LOGI(TAG, "app_nvs_load_sta_creds: (%s) no station ssid found in nvs\n", esp_err_to_name(esp_err));
            return false;
        }
        memcpy(wifi_sta_config->sta.ssid, wifi_config_buff, wifi_config_size);

        // Load password
        wifi_config_size = sizeof(wifi_sta_config->sta.password);
        esp_err = nvs_get_blob(handle, "password", wifi_config_buff, &wifi_config_size);
        if (esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            ESP_LOGI(TAG, "app_nvs_load_sta_creds: (%s) no station password found in nvs\n", esp_err_to_name(esp_err));
            return false;
        }
        memcpy(wifi_sta_config->sta.password, wifi_config_buff, wifi_config_size);

        free(wifi_config_buff);
        nvs_close(handle);
        ESP_LOGI(TAG,
                 "app_nvs_load_sta_creds: found ssid: %s password: %s",
                 wifi_sta_config->sta.ssid,
                 wifi_sta_config->sta.password);
        return wifi_sta_config->sta.ssid[0] != '\0';
    }
    else
    {
        return false;
    }
}

esp_err_t app_nvs_clear_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG,
             "app_nvs_clear_sta_creds: clearing wifi station mode credentials "
             "from flash");

    esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "app_nvs_clear_sta_creds: error %s opening nvs handle\n", esp_err_to_name(esp_err));
        return esp_err;
    }

    // erase credentials
    esp_err = nvs_erase_all(handle);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "app_nvs_clear_sta_creds: error %s erasing station mode credentials\n", esp_err_to_name(esp_err));
        return esp_err;
    }

    // commit clearing credentials from nvs
    esp_err = nvs_commit(handle);
    if (esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "app_nvs_clear_sta_creds: error %s NVS commit\n", esp_err_to_name(esp_err));
        return esp_err;
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "app_nvs_clear_sta_creds: returned ESP_OK");
    return ESP_OK;
}
