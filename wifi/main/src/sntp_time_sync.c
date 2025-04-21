#include "sntp_time_sync.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/apps/sntp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/idf_additions.h"
#include "http_server.h"
#include "portmacro.h"
#include "tasks_common.h"
#include "wifi.h"

static const char TAG[] = "sntp_time_sync";

// sntp operating mode status
static bool sntp_op_mode_set = false;

/*
 * initialize sntp service
 */
static void sntp_time_sync_init_sntp()
{
    ESP_LOGI(TAG, "sntp_time_sync_init_sntp: initializing sntp service");

    if (!sntp_op_mode_set)
    {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_op_mode_set = true;
    }

    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // let http_server know sntp is initialized
    http_server_monitor_send_message(HTTP_TIME_SERVICE_INITIALIZED);
}

/*
 * Gets the current time and if the current time is not up to date then the
 * sntp_time_sync_init_sntp is called
 */
static void sntp_time_sync_obtain_time()
{
    time_t now = 0;
    struct tm time_info = {0};

    time(&now);
    localtime_r(&now, &time_info);

    // check time in case we need to initialize or reinitialize
    if (time_info.tm_year < (2023 - 1900))
    {
        sntp_time_sync_init_sntp();

        // set local time zone
        setenv("TZ", "CST+6", 1);
        tzset();
    }
}

/*
 * the sntp time syncronization task.
 * @param arg pvparam
 */
static void sntp_time_sync(void *pvparam)
{
    while (1)
    {
        sntp_time_sync_obtain_time();
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void sntp_time_sync_task_start(void)
{
    xTaskCreatePinnedToCore(&sntp_time_sync,
                            "sntp_time_sync",
                            SNTP_TIME_SYNC_STACK_SIZE,
                            NULL,
                            SNTP_TIME_SYNC_PRIORITY,
                            NULL,
                            SNTP_TIME_SYNC_CORE_ID);
}

char *sntp_time_sync_get_time(void)
{
    static char time_buffer[100] = {0};
    time_t now = 0;
    struct tm time_info = {0};

    time(&now);
    localtime_r(&now, &time_info);

    if (time_info.tm_year < (2023 - 1900))
    {
        ESP_LOGI(TAG, "sntp_time_sync_get_time: time is not set yet");
    }
    else
    {
        strftime(time_buffer, sizeof(time_buffer), "%d.%m.%Y %H:%M:%S", &time_info);
        ESP_LOGI(TAG, "sntp_time_sync_get_time: %s", time_buffer);
    }

    return time_buffer;
}
