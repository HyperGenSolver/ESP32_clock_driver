#ifndef WIFI_RECEIVER_H
#define WIFI_RECEIVER_H
#include <stdio.h>
#include <string.h>
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_async_memcpy.h"



typedef struct temperature_data_struct { //structure of the latest temperature data stored locally
    int id;
    int time;
    float temperature_outside;
} temperature_data_struct;

void on_data_receive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status);
void get_mac_address();
void init_esp_now();

#endif 