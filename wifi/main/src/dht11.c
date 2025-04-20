#include "dht11.h"

#include <stdio.h>

#include "dht.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "tasks_common.h"

float humidity;
float temperature;
static const char TAG[] = "DHT11";

/*
 * DHT11 Sensor task
 */
static void DHT11_task(void *pvParameter)
{
    ESP_LOGI(TAG, "DHT11_task: starting task");

    for (;;)
    {
        // dht_read_float_data(DHT_TYPE_DHT11, DHT11_GPIO, &humidity, &temperature);
        temperature = 20.0f;
        humidity = 100.0f;

        printf("humidity: %.1f ", humidity);
        printf("temperature: %.1f", temperature);
        printf("\n");

        vTaskDelay(4000 / portTICK_PERIOD_MS); // 4 seconds
    }
}

void DHT11_task_start(void)
{
    xTaskCreatePinnedToCore(&DHT11_task,
                            "DHT11_task",
                            DHT11_STACK_SIZE,
                            NULL,
                            DHT11_TASK_PRIORITY,
                            NULL,
                            DHT11_TASK_CORE_ID);
}
