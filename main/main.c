#include <stdio.h>
#include "driver\gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>

#include "variables.h"
#include "constants.h"
#include "chip_driver.c"
#include "time_keeping.c"
#include "ds1302.h"
#include "WIFI_receiver.h"

static const char* TAG = "Clock_Driver";
extern QueueHandle_t temperature_data_queue;

void set_time_task(void *pvParameters)
{
  while (1)
  {

    if (ir_sm.isr_triggered){// a button pressed
      uint8_t _hour = 0;
      uint8_t _min = 0;
      if(get_time_user_input(&_hour, &_min)){
        set_system_time(_hour, _min, 0);
      }
      
      ir_sm.isr_triggered = false;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
// Display the time and temperature on the NIXI display and send the data to e-ink 
void display_nixi_temp_send_to_eInk(void *pvParameters)
{
  TickType_t xLastExecutionTime;
  xLastExecutionTime = xTaskGetTickCount();
  temperature_data_struct latest_temp_data = {-1, 0, 0};
  uint16_t ms_since_temp_update = 0;
  float temp_to_display;
  while (1)
  {
    xQueueReceive(temperature_data_queue, &latest_temp_data, ( TickType_t ) 0); // get the latest temperature data and remove it from the queue
    if (ir_sm.time_set_mode)
    {
      set_colon_led(true);
      set_nixi_digit(ir_sm.digits[0], ir_sm.digits[1], ir_sm.digits[2], ir_sm.digits[3]);
      
    }
    
    else if (latest_temp_data.id != -1 && ms_since_temp_update > MAX_DELAY_TEMP_UPDATE_DISPLAY){

      temp_to_display = latest_temp_data.temperature_outside;


      bool temp_is_negative = temp_to_display < 0;
      if(temp_is_negative){
        temp_to_display *= -1.0;
      }

      uint8_t digit_a = (int)temp_to_display / 10;
      uint8_t digit_b = ((int)temp_to_display) % 10;
      uint8_t digit_c = ((int)(temp_to_display * 10)) % 10;
      uint8_t digit_d  = 0;

      set_nixi_digit(digit_a, digit_b, digit_c, digit_d);
      if(temp_is_negative){ //blink if negative temperature
        toggle_colon_led();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        toggle_colon_led();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        toggle_colon_led();
      }
      if(ms_since_temp_update > (MAX_DELAY_TEMP_UPDATE_DISPLAY + TIME_TEMPERATURE_IS_SHOWN )){ // Send the data to e-ink after a while
        uint16_t time_hour = 0;
        uint16_t time_min = 0;
        uint16_t time_sec = 0;
        get_system_time(&time_hour, &time_min, &time_sec);                                 // get the current time
        encode_time_to_int(time_hour, time_min, time_sec, &latest_temp_data.time); // encode the time to int
        esp_err_t result = esp_now_send(NULL, (uint8_t *)&latest_temp_data, sizeof(latest_temp_data)); // NULL as receiver means send to all peers
        if (result != ESP_OK)
        {
          ESP_LOGE(TAG, "Error sending data: %d", result);
        }
      

        latest_temp_data.id = -1; // set current temperature data to outdated
        ms_since_temp_update = 0; // reset the time since last temperature update

      }
    }
    else
    {
      uint16_t time_hour = 0;
      uint16_t time_min = 0;
      uint16_t time_sec = 0;
      get_system_time(&time_hour, &time_min, &time_sec);
      set_nixi_digit(time_hour / 10, time_hour % 10, time_min / 10, time_min % 10);
      toggle_colon_led();
    }
    ms_since_temp_update += DISPLAY_NIXI_FREQ;
    xTaskDelayUntil(&xLastExecutionTime, DISPLAY_NIXI_FREQ / portTICK_PERIOD_MS);    
  }
}
void print_for_debugging(void *pvParameters) {
  while (1) {
    uint16_t time_hour = 0;
    uint16_t time_min = 0;
    uint16_t time_sec = 0;
    get_system_time(&time_hour, &time_min, &time_sec);

    // Print the time string to console (replace with your display method)
    ESP_LOGI(TAG, "Current time: %i %i %i", time_hour, time_min, time_sec);
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Update every second
  }
}
void app_main(void)
{
  setup_gpio();
  // --- Setup ESP Now
  init_esp_now();
  get_mac_address();
  // Setup temperature data queue
  temperature_data_queue = xQueueCreate(1, sizeof(temperature_data_struct));
  if (temperature_data_queue == NULL) {
    ESP_LOGE(TAG, "Failed to create temperature queue"); while(1){}
  }


  TaskHandle_t time_task_handle;
  TaskHandle_t display_nixi_and_temp_handle;
  xTaskCreate(set_time_task, "Set_time_task", 2048, NULL, 2, &time_task_handle);
  xTaskCreate(display_nixi_temp_send_to_eInk, "display_nixi_task", 2024, NULL, 1, &display_nixi_and_temp_handle);
  xTaskCreate(print_for_debugging, "display_time", 2048, NULL, 1, NULL);
  

  // DS1302 time keeping
  ds1302_t dev = {
        .ce_pin = GPIO_NUM_14,
        .io_pin = GPIO_NUM_23,
        .sclk_pin = GPIO_NUM_13
  };
  ESP_ERROR_CHECK(ds1302_init(&dev));
  ESP_ERROR_CHECK(ds1302_set_write_protect(&dev, false));

  struct tm time = {
        .tm_year = 123, //year since 1900 (2018 - 1900)
        .tm_mon  = 0,  // 0-based
        .tm_mday = 11,
        .tm_hour = 0,
        .tm_min  = 0,
        .tm_sec  = 0
    };
  ESP_ERROR_CHECK(ds1302_set_time(&dev, &time));
  ESP_ERROR_CHECK(ds1302_start(&dev, true));
  //ds1302_get_time(&dev, &time);
  //printf("%04d-%02d-%02d %02d:%02d:%02d\n", time.tm_year + 1900 /*Add 1900 for better readability*/, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
  //ESP_LOGI(TAG, "Read: %x", data_read);

  /*
  DS1302_write_register(0x8E, 0b00000000); // Override Write protect
  DS1302_write_register(0x80, 0b00000001); // Enable internal clock and seconds
  DS1302_write_register(0xC0, 0b00010011); // 
  uint8_t data_read = 0b00000000;
  DS1302_read_register(0xC1, &data_read);
  ESP_LOGI(TAG, "Read: %x", data_read);
  */
}
