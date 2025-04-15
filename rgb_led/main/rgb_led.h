#ifndef RGB_LED_H
#define RGB_LED_H

// RGB LED GPIOs
#define RGB_LED_RED_GPIO 15
#define RGB_LED_GREEN_GPIO 23
#define RGB_LED_BLUE_GPIO 22

// RGB LED color mix channels
#define RGB_LED_CHANNEL_NUM 3

// RGB LED configuration
typedef struct {
    int channel;
    int gpio;
    int mode;
    int timer_index;
} ledc_info_t;
ledc_info_t ledc_ch[RGB_LED_CHANNEL_NUM];

/**
 * Color to indicate wifi application started.
 */
void rgb_led_wifi_app_started(void);

/**
 * Color to indicate http server started.
 */
void rgb_led_http_server_started(void);

/**
 * Color to indicate ESP32 is connected to access point.
 */
void rgb_led_wifi_connected(void);

#endif
