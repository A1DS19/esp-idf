#include "wifi_reset_button.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "esp_attr.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "tasks_common.h"
#include "wifi.h"

static const char TAG[] = "wifi_reset_button";

// semaphore handler
SemaphoreHandle_t wifi_reset_semaphore = NULL;

/*
 * ISR handler for the wifi reset boot button
 * @param arg parameter which can be passed to the ISR handler
 */
void IRAM_ATTR wifi_reset_button_isr_handler(void *arg)
{
    // notify the button task
    xSemaphoreGiveFromISR(wifi_reset_semaphore, NULL);
}

/*
 * wifi reset button react to a boot event by sending a msg to
 * the wifi application to disconnect from wifi and clear the saved credentials.
 * @param pvParam parameter to pass to task
 */
void wifi_reset_button_task(void *pvParam)
{
    for (;;)
    {
        if (xSemaphoreTake(wifi_reset_semaphore, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "WIFI RESET BUTTON INTERRUPT OCCURRED");

            // send message to disconnect wifi and clear credentials.
            wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }
}

void wifi_reset_button_config(void)
{
    // create binary semaphore
    wifi_reset_semaphore = xSemaphoreCreateBinary();

    // configure the button and set direction
    gpio_reset_pin(WIFI_RESET_BUTTON);
    gpio_set_direction(WIFI_RESET_BUTTON, GPIO_MODE_INPUT);

    // enable interrupt on negative edge
    gpio_set_intr_type(WIFI_RESET_BUTTON, GPIO_INTR_NEGEDGE);

    // create the wifi reset button task
    xTaskCreatePinnedToCore(&wifi_reset_button_task,
                            "wifi_reset_button",
                            WIFI_RESET_BUTTON_TASK_STACK_SIZE,
                            NULL,
                            WIFI_RESET_BUTTON_TASK_PRIORITY,
                            NULL,
                            WIFI_RESET_BUTTON_TASK_CORE_ID);

    // install gpio ISR service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // attach the ISR
    gpio_isr_handler_add(WIFI_RESET_BUTTON, wifi_reset_button_isr_handler, NULL);
}
