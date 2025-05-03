#ifndef WIFI_H
#define WIFI_H

#include "esp_netif.h"
#include "esp_wifi_types_generic.h"
#include <freertos/FreeRTOS.h>
#include <stdint.h>


// callback typedef
typedef void (*wifi_connected_event_callback_t)(void);

// AP Name
#define WIFI_AP_SSID "ESP32_AP"
// AP Password
#define WIFI_AP_PASSWORD "password"
// AP Channel
#define WIFI_AP_CHANNEL 1
// AP Visibility (0 = visible)
#define WIFI_AP_SSID_HIDDEN 0
// AP Max clients
#define WIFI_AP_MAX_CONNECTIONS 1
// AP Beacon interval 100 miliseconds
#define WIFI_AP_BEACONE_INTERVAL 100
// AP WIFI IP address
#define WIFI_AP_IP "192.168.0.1"
// AP Gateway IP
#define WIFI_AP_GATEWAY "192.168.0.1"
// AP Netmask
#define WIFI_AP_NETMASK "255.255.255.0"
// AP Bandwidth (HT20 = 20mhz)
#define WIFI_AP_BANDWIDTH WIFI_BW_HT20
// WIFI Power save (NONE = not used)
#define WIFI_STA_POWER_SAVE WIFI_PS_NONE
// WIFI max SSID length
#define MAX_SSID_LENGTH 32
// WIFI max password length
#define MAX_PASSWORD_LENGTH 64
// WIFI max connection retries
#define MAX_CONNECTION_RETRIES 5

extern esp_netif_t *esp_netif_sta;
extern esp_netif_t *esp_netif_ap;

/*
 * Message IDs for the WIFI application task
 * @note Expand this based on requirements
 */
typedef enum wifi_app_message {
    WIFI_APP_MSG_START_HTTP_SERVER = 0,
    WIFI_APP_MSG_CONNECTING_HTTP_SERVER,
    WIFI_APP_MSG_STA_CONNECTED_GOT_IP,
    WIFI_APP_MSG_STA_DISCONNECTED,
    WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT,
    WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS
} wifi_app_message_e;

/*
 * Struct for message queue
 */
typedef struct wifi_app_queue_message {
    wifi_app_message_e msgID;
} wifi_app_queue_message_t;

/*
 * Sends message to the queue
 * @param msgID message ID from the wifi_app_message_e enum.
 * @return pdTRUE if an item is successfully sent to the queue, otherwise false.
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/*
 * Starts the wifi RTOs task
 */
void wifi_app_start(void);

/*
 * Get wifi config
 */
wifi_config_t *wifi_get_config(void);

/*
 * Sets callback
 */
void wifi_set_callback(wifi_connected_event_callback_t cb);

/*
 *Calls callback
 */
void wifi_call_callback(void);

/*
 * Get RSSI from wifi data
 */
int8_t wifi_get_rssi(void);

#endif
