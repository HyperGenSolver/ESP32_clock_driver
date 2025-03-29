#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"

#ifndef TIME_KEEPING // Header guard to prevent multiple inclusions
#define TIME_KEEPING
bool set_system_time(uint8_t hour, uint8_t min, uint8_t sec){
    const char *tag = "Time_keeping";
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  if(hour > 23 || min > 59 || sec > 59){
    ESP_LOGE(tag, "Invalid time input");
    return false;
  }
  tv.tv_sec = hour * 3600 + min * 60 + sec;
  settimeofday(&tv, NULL);
  return true;
}
bool get_system_time(uint16_t* time_hour, uint16_t* time_min, uint16_t* time_sec){
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    *time_hour = timeinfo->tm_hour;
    *time_min = timeinfo->tm_min;
    *time_sec = timeinfo->tm_sec;
    return true;
}
bool encode_time_to_int(uint16_t hour, uint16_t min, uint16_t sec, int* time_int){
    if(hour > 24 || min > 60 || sec > 60){
        return false;
    }
    *time_int = (hour * 3600) + (min * 60) + sec;
    return true;
}
#endif 