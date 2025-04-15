#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdio.h>

void app_main(void)
{
    while (true)
    {
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}
