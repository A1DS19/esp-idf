#ifndef WIFI_RESET_BUTTON_H
#define WIFI_RESET_BUTTON_H

/*
 * Default interrupt flag
 */
#define ESP_INTR_FLAG_DEFAULT 0

// Wifi reset button is the BOOT button on the devkit
#define WIFI_RESET_BUTTON 0

/*
 * Configures wifi reset button and interrupt configuration
 */
void wifi_reset_button_config(void);

#endif // ! WIFI_RESET_BUTTON_H
