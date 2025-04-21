#ifndef NVS_H
#define NVS_H

#include "esp_err.h"
#include <stdbool.h>

/*
 * Saves station mode wifi credentials to nvs.
 * @return ESP_OK if successful.
 */
esp_err_t app_nvs_save_sta_creds(void);

/*
 * Loads previosly saved credentials from nvs.
 * @return true if previosly saved credentials are found.
 */
bool app_nvs_load_sta_creds(void);

/*
 * Clear sta credentials from nvs
 * @return ESP_OK if successful
 */
esp_err_t app_nvs_clear_sta_creds(void);

#endif // !NVS_H
