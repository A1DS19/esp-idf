#include "wifi.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <lwip/netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_interface.h"
#include "esp_log_level.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "http_server.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include "portmacro.h"
#include "rgb_led.h"
#include "tasks_common.h"

// TAG used for serial console messages
static const char TAG[] = "wifi_app";

// Queue handle used to manipulate the main queue of events
static QueueHandle_t wifi_app_queue_handle;

// netif objects for the station and access point
esp_netif_t *esp_netif_sta = NULL;
esp_netif_t *esp_netif_ap = NULL;

/*
 * Used for returning wifi config
 */
wifi_config_t *wifi_config = NULL;

/*
 * Used to track number of retries when connection attempt fails
 */
static int g_retry_number;

/*
 * Wifi app event group handle and status bits
 */
static EventGroupHandle_t wifi_event_group;
const int WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT = BIT0;
const int WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT = BIT1;
const int WIFI_APP_USER_REQUESTED_STA_DISONNECT_BIT = BIT2;
const int WIFI_APP_STA_CONNECTED_GOT_IP_BIT = BIT3;

/*
 * WIFI application event handler
 * @param arg data, aside from event data, that is passed when it is called
 * @param event_base the base id of the event to register the handler
 * @param evebt_id the id of the event to register the handler for
 * @param event_data for the event data
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
            break;
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            wifi_event_sta_disconnected_t *wifi_event_sta_disconnected =
                (wifi_event_sta_disconnected_t *)malloc(sizeof(wifi_event_sta_disconnected_t));
            *wifi_event_sta_disconnected = *((wifi_event_sta_disconnected_t *)event_data);
            printf("WIFI_EVENT_STA_DISCONNECTED, reason code %d\n", wifi_event_sta_disconnected->reason);
            if (g_retry_number < MAX_CONNECTION_RETRIES)
            {
                esp_wifi_connect();
                g_retry_number += 1;
            }
            else
            {
                wifi_app_send_message(WIFI_APP_MSG_STA_DISCONNECTED);
            }
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
            wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);
            break;
        }
    }
}

/*
 * initializes the WIFI application event handler for WIFI and IP events.
 */
static void wifi_app_event_handler_init(void)
{
    // event loop for driver
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // event handler for the connection
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_app_event_handler,
                                                        NULL,
                                                        &instance_wifi_event));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_app_event_handler,
                                                        NULL,
                                                        &instance_ip_event));
}

/*
 * Initializes the TCP stack and default wifi config
 */
void wifi_app_default_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    // Default wifi config
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap = esp_netif_create_default_wifi_ap();
}

/*
 *Configures the wifi access point settings and assigns the static IP to the
 *SoftAP;
 */
void wifi_app_soft_ap_config(void)
{
    // SoftAP - Wifi access point config
    wifi_config_t ap_config = {.ap = {
                                   .ssid = WIFI_AP_SSID,
                                   .ssid_len = strlen(WIFI_AP_SSID),
                                   .password = WIFI_AP_PASSWORD,
                                   .channel = WIFI_AP_CHANNEL,
                                   .ssid_hidden = WIFI_AP_SSID_HIDDEN,
                                   .authmode = WIFI_AUTH_WPA2_PSK,
                                   .max_connection = WIFI_AP_MAX_CONNECTIONS,
                                   .beacon_interval = WIFI_AP_BEACONE_INTERVAL,
                               }};

    // configure dhcp
    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

    esp_netif_dhcps_stop(esp_netif_ap); // muust be called first
    inet_pton(AF_INET, WIFI_AP_IP,
              &ap_ip_info.ip); // assigns AP static IP, GW, netmask
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));
}

/*
 * Connects ESP32 to external access point using updated station configuration
 */
static void wifi_connect_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_get_config()));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

/*
 * Main task for the wifi application
 * @param pvParameters parameter which can be passed to the task
 */
static void wifi_app_task(void *pvParamters)
{
    wifi_app_queue_message_t msg;
    EventBits_t event_bits;

    // initialize event handler
    wifi_app_event_handler_init();

    // initialize the TCP/IP stack and wifi config
    wifi_app_default_wifi_init();

    // SoftAP config
    wifi_app_soft_ap_config();

    // start wifi
    ESP_ERROR_CHECK(esp_wifi_start());

    // send first event message
    wifi_app_send_message(WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS);

    for (;;)
    {
        if (xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
            case WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS:
                ESP_LOGI(TAG, "WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS");
                if (app_nvs_load_sta_creds())
                {
                    ESP_LOGI(TAG, "wifi_app_task: loaded station configuration");
                    wifi_connect_sta();
                    xEventGroupSetBits(wifi_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
                }
                else
                {
                    ESP_LOGI(TAG, "wifi_app_task: unable to load station configuration");
                }

                wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);
                break;

            case WIFI_APP_MSG_START_HTTP_SERVER:
                ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");
                start_http_server();
                rgb_led_http_server_started();
                break;

            case WIFI_APP_MSG_CONNECTING_HTTP_SERVER:
                ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_HTTP_SERVER");
                xEventGroupSetBits(wifi_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);

                // attemp connection
                wifi_connect_sta();

                // set retries to 0
                g_retry_number = 0;

                // let http_server_know of connection attempt
                http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_INIT);

                break;

            case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");
                xEventGroupSetBits(wifi_event_group, WIFI_APP_STA_CONNECTED_GOT_IP_BIT);

                rgb_led_wifi_connected();
                http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_SUCCESS);
                event_bits = xEventGroupGetBits(wifi_event_group);
                if (event_bits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT)
                { // Save sta creds
                    // only if
                    // connecting from
                    // the http server
                    // (not loaded from
                    // nvs)
                    xEventGroupClearBits(wifi_event_group,
                                         WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT); // clear bits
                }
                else
                {
                    app_nvs_save_sta_creds();
                }

                if (event_bits & WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT)
                {
                    xEventGroupClearBits(wifi_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);
                }
                break;

            case WIFI_APP_MSG_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED");
                event_bits = xEventGroupGetBits(wifi_event_group);
                if (event_bits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT)
                {
                    ESP_LOGI(TAG,
                             "WIFI_APP_MSG_STA_DISCONNECTED: attemp using saved "
                             "credentials");
                    xEventGroupClearBits(wifi_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
                    app_nvs_clear_sta_creds();
                }
                else if (event_bits & WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT)
                {
                    ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: attemp from http server");
                    xEventGroupClearBits(wifi_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);
                    http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_FAIL);
                }
                else if (event_bits & WIFI_APP_USER_REQUESTED_STA_DISONNECT_BIT)
                {
                    ESP_LOGI(TAG,
                             "WIFI_APP_USER_REQUESTED_STA_DISONNECT_BIT: user "
                             "requested disonnection");
                    xEventGroupClearBits(wifi_event_group, WIFI_APP_USER_REQUESTED_STA_DISONNECT_BIT);
                    http_server_monitor_send_message(HTTP_MSG_WIFI_USER_DISCONNECT);
                }
                else
                {
                    ESP_LOGI(TAG,
                             "WIFI_APP_MSG_STA_DISCONNECTED: attempt failed, check "
                             "access point");
                    // adust case, keep trying to connect, etc
                }

                if (event_bits & WIFI_APP_STA_CONNECTED_GOT_IP_BIT)
                {
                    xEventGroupClearBits(wifi_event_group, WIFI_APP_STA_CONNECTED_GOT_IP_BIT);
                }

                break;

            case WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT:
                ESP_LOGI(TAG, "WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT");
                event_bits = xEventGroupGetBits(wifi_event_group);
                if (event_bits & WIFI_APP_STA_CONNECTED_GOT_IP_BIT)
                {
                    xEventGroupSetBits(wifi_event_group, WIFI_APP_USER_REQUESTED_STA_DISONNECT_BIT);
                    g_retry_number = MAX_CONNECTION_RETRIES;
                    ESP_ERROR_CHECK(esp_wifi_disconnect());
                    app_nvs_clear_sta_creds();
                    // rename to a more meaninful name when there is not a wifi
                    // connection
                    rgb_led_http_server_started();
                }

            default:
                break;
            }
        }
    }
}

BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
    wifi_app_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

void wifi_app_start(void)
{
    ESP_LOGI(TAG, "STARTING WIFI APPLICATION");
    rgb_led_wifi_app_started();
    esp_log_level_set("wifi", ESP_LOG_NONE);

    // allocate memory for wifi config
    wifi_config = (wifi_config_t *)malloc(sizeof(wifi_config_t));
    memset(wifi_config, 0, sizeof(wifi_config_t));

    // create message queue
    wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

    // create wifi app event group
    wifi_event_group = xEventGroupCreate();

    // start wifi app task
    xTaskCreatePinnedToCore(&wifi_app_task,
                            "wifi_app_task",
                            WIFI_APP_TASK_STACK_SIZE,
                            NULL,
                            WIFI_APP_TASK_PRIORITY,
                            NULL,
                            WIFI_APP_TASK_CORE_ID);
}

wifi_config_t *wifi_get_config(void)
{
    return wifi_config;
}
