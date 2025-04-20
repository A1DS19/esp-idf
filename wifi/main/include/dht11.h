#ifndef DHT11_H
#define DHT11_H

#include <dht.h>

#define DHT11_GPIO 20

extern float humidity;
extern float temperature;


/*
 * Starts DHT11 sensor task
 */
void DHT11_task_start(void);

#endif // !
