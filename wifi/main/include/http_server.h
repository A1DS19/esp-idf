#ifndef HTTP_SERVER_H
#define HTTP_SEVER_H

#define OTA_UPDATE_PENDING 0
#define OTA_UPDATE_SUCCESSFUL 1
#define OTA_UPDATE_FAILED -1

/*
 * Connection status for wifi
 */
typedef enum http_server_wifi_connect_status{
    NONE = 0,
    HTTP_WIFI_STATUS_CONNECTING,
    HTTP_WIFI_STATUS_CONNECT_FAIL,
    HTTP_WIFI_STATUS_CONNECT_SUCCESS,
    HTTP_WIFI_STATUS_DISCONNECTED
} http_server_wifi_connect_status_e;

/*
 * Messages for the http monitor
 */
#include "portmacro.h"
typedef enum http_server_message {
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
    HTTP_MSG_WIFI_CONNECT_SUCCESS,
    HTTP_MSG_WIFI_CONNECT_FAIL,
    HTTP_MSG_OTA_UPDATE_SUCCESSFULL,
    HTTP_MSG_OTA_UPDATE_FAILED,
    HTTP_MSG_WIFI_USER_DISCONNECT
} http_server_message_e;

/*
 * Struct for the message queue
 */
typedef struct http_server_queue_message{
    http_server_message_e msgID;
} http_server_queue_message_t;

/*
 * Sends a message to the queue
 * @param msgID message ID from the http_server_message_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFalse.
 */
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

/*
 * Starts http server
 */
void start_http_server(void);

/*
 * Stop http server
 */
void stop_http_server(void);

/*
 * Timer callback, calls esp_restart when successful firmware OTA_UPDATE_FAILED
 */
void http_server_fw_update_reset_callback(void *arg);

#endif // !HTTP_SERVER_H
