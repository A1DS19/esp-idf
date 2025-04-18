#include "http_server.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <stdint.h>
#include <string.h>
#include <tasks_common.h>
#include <wifi.h>

#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "http_parser.h"
#include "portmacro.h"

// TAG used for ESP serial console messages
static const char TAG[] = "http_server";

// http server task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP server monitor handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to manipulae main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

extern const uint8_t jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

extern const uint8_t app_css_start[] asm("_binary_app_css_start");
extern const uint8_t app_css_end[] asm("_binary_app_css_end");

extern const uint8_t app_js_start[] asm("_binary_app_js_start");
extern const uint8_t app_js_end[] asm("_binary_app_js_end");

extern const uint8_t app_favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t app_favicon_ico_end[] asm("_binary_favicon_ico_end");

/*
 * HTTP server monitor task used to track events of http server
 * @param pvParameters parameter which can be passed to the task.
 */
static void http_server_monitor(void *parameter)
{
    http_server_queue_message_t msg;
    for (;;)
    {
        if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
            case HTTP_MSG_WIFI_CONNECT_INIT:
                ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
                break;

            case HTTP_MSG_WIFI_CONNECT_SUCCESS:
                ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
                break;

            case HTTP_MSG_WIFI_CONNECT_FAIL:
                ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");
                break;

            case HTTP_MSG_OTA_UPDATE_SUCCESSFULL:
                ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFULL");
                break;

            case HTTP_MSG_OTA_UPDATE_FAILED:
                ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
                break;

            case HTTP_MSG_OTA_UPDATE_INITIALIZED:
                ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_INITIALIZED");
                break;

            default:
                break;
            }
        }
    }
}

/*
 * Jquery get handler requested when accessing the web page.
 * @param req http request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "jquery requested");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);
    return ESP_OK;
}

/*
 * Sends index.html page.
 * @param req http request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "index.html requested");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

/*
 * App.css get handler.
 * @param req http request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.css requested");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);
    return ESP_OK;
}

/*
 * App.js get handler.
 * @param req http request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.js requested");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
    return ESP_OK;
}

/*
 * Favicon ico get handler.
 * @param req http request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "favicon.ico requested");
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)app_favicon_ico_start, app_favicon_ico_end - app_favicon_ico_start);
    return ESP_OK;
}

/*
 * Sets up the default httpd server configuration.
 * @return http server instance handle if sucessfull, NULL, otherwise.
 */
static httpd_handle_t http_server_configure(void)
{
    // generate defaul configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Create http server monitor task
    xTaskCreatePinnedToCore(&http_server_monitor,
                            "http_server_monitor",
                            HTTP_SERVER_MONITOR_STACK_SIZE,
                            NULL,
                            HTTP_SERVER_MONITOR_PRIORITY,
                            &task_http_server_monitor,
                            HTTP_SERVER_TASK_CORE_ID);

    // Create message queue
    http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

    config.core_id = HTTP_SERVER_TASK_CORE_ID;
    config.task_priority = HTTP_SERVER_TASK_PRIORITY;
    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
    config.max_uri_handlers = 20;
    config.recv_wait_timeout = 10; // seconds
    config.send_wait_timeout = 10; // seconds

    ESP_LOGI(TAG,
             "http_server_configure: starting server on port: '%d' with task "
             "priority: '%d'",
             config.server_port,
             config.task_priority);

    // start httpd server
    if (httpd_start(&http_server_handle, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_configure: registering URI handlers");
        httpd_uri_t jquery_js = {.uri = "/jquery-3.3.1.min.js",
                                 .method = HTTP_GET,
                                 .handler = http_server_jquery_handler,
                                 .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &jquery_js);

        httpd_uri_t index_html = {.uri = "/",
                                  .method = HTTP_GET,
                                  .handler = http_server_index_html_handler,
                                  .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &index_html);

        httpd_uri_t app_css = {.uri = "/app.css",
                               .method = HTTP_GET,
                               .handler = http_server_app_css_handler,
                               .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &app_css);

        httpd_uri_t app_js = {.uri = "/app.js",
                              .method = HTTP_GET,
                              .handler = http_server_app_js_handler,
                              .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &app_js);

        httpd_uri_t favicon_ico = {.uri = "/favicon.ico",
                                   .method = HTTP_GET,
                                   .handler = http_server_favicon_ico_handler,
                                   .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &favicon_ico);

        return http_server_handle;
    }

    return NULL;
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
    http_server_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void start_http_server(void)
{
    if (http_server_handle == NULL)
    {
        http_server_handle = http_server_configure();
    }
}

void stop_http_server(void)
{
    if (http_server_handle == NULL)
    {
        httpd_stop(http_server_handle);
        ESP_LOGI(TAG, "http_server_stop: stopping http server");
        http_server_handle = NULL;
    }

    if (task_http_server_monitor)
    {
        vTaskDelete(task_http_server_monitor);
        ESP_LOGI(TAG, "http_server_stop: stopping http server monitor");
        task_http_server_monitor = NULL;
    }
}
