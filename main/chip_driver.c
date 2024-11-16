#include <stdio.h>
#include "driver\gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "constants.h"
#include <ets_sys.h>
#include "variables.h"


#ifndef CHIP_DRIVER // Header guard to prevent multiple inclusions
#define CHIP_DRIVER

static const char* chip_driver_str_tag = "CHIP_DRIVER";

// Output both 8 bit data to 
void set_shift_register(unsigned char data_l, unsigned char data_r)
{
  uint16_t _delay = 1000 / SHIFT_REG_FREQ;
  for (int i = 0; i < 8; i++)
  {
    // Set the serial data bit based on the current data byte
    gpio_set_level(SER_L, (data_l >> i) & 1);
    gpio_set_level(SER_R, (data_r >> i) & 1);

    // Pulse the clock to shift the data in
    gpio_set_level(BLINK_GPIO, 1);
    gpio_set_level(SRCLK, 1);
    vTaskDelay(_delay / portTICK_PERIOD_MS / 2.0);
    gpio_set_level(SRCLK, 0);
    vTaskDelay(_delay / portTICK_PERIOD_MS / 2.0);
    gpio_set_level(BLINK_GPIO, 0);
  }

  // Latch the data using the storage register clock
  gpio_set_level(RCLK, 1);
  gpio_set_level(RCLK, 0);
}
void set_nixi_digit(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  //First two digits (l)
  unsigned char bit_pattern_l = k155id1_bit_pattern[a]; 
  bit_pattern_l = (bit_pattern_l << 4) | k155id1_bit_pattern[b];
  unsigned char bit_pattern_r = k155id1_bit_pattern[c];
  //Digit three and four (r)
  bit_pattern_r = (bit_pattern_r << 4) | k155id1_bit_pattern[d];
  set_shift_register(bit_pattern_l, bit_pattern_r);
}
void toggle_colon_led()
{
  colon_led_state = !colon_led_state;
  gpio_set_level(COLON_EN, colon_led_state);
}
void set_colon_led(bool _state)
{
  if(_state == true)
  {
    colon_led_state = true;
    gpio_set_level(COLON_EN, colon_led_state);
  }
  else
  {
    colon_led_state = false;
    gpio_set_level(COLON_EN, colon_led_state);
  }
}
void flash_colon_led_fast()
{
  for (int i = 0; i < 3; i++)
  {
    toggle_colon_led();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
// --- DS1302 (time)
void DS1302_write_register(uint8_t write_command, uint8_t data) {
  // input as least siginificant bit
  uint16_t _delay = 1000 / DS1302_FREQ / portTICK_PERIOD_MS / 2.0;
  gpio_set_level(DS1302_CE, 1);
  gpio_set_level(DS1302_IO, 0);
  gpio_set_level(DS1302_CLK, 0);
  vTaskDelay(_delay);
  for (int i = 0; i < 8; i++) // Send write command
  {
    gpio_set_level(DS1302_CLK, 0);
    gpio_set_level(DS1302_IO, 0);
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_IO, (write_command >> i) & 1); // LSb in
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_CLK, 1);
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_IO, 0);
    vTaskDelay(_delay/2);
    
  }
  for (int i = 0; i < 8; i++) // Send data
  {
    gpio_set_level(DS1302_CLK, 0);
    gpio_set_level(DS1302_IO, 0);
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_IO, (data >> i) & 1); // LSb in
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_CLK, 1);
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_IO, 0);
    vTaskDelay(_delay/2);
  }
  gpio_set_level(DS1302_CLK, 0);
  vTaskDelay(_delay);
  gpio_set_level(DS1302_CE, 0);
  vTaskDelay(_delay); // requires atleast 4 us of buffer time
}
void DS1302_read_register(uint8_t read_command, uint8_t* data) { // data needs to be proved as 0b0000000
  gpio_set_level(DS1302_CE, 1);
  gpio_set_level(DS1302_IO, 0);
  gpio_set_level(DS1302_CLK, 0);
  uint16_t _delay = 1000 / DS1302_FREQ / portTICK_PERIOD_MS / 2.0;
  vTaskDelay(_delay);

  for (int i = 0; i < 8; i++) // Send write command
  {
    gpio_set_level(DS1302_CLK, 0);
    gpio_set_level(DS1302_IO, 0);
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_IO, (read_command >> i) & 1); // LSb in
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_CLK, 1);
    vTaskDelay(_delay/2);
    gpio_set_level(DS1302_IO, 0);
    vTaskDelay(_delay/2);
  }
  for (int i = 0; i < 8; i++) // Read data
  {
    gpio_set_level(DS1302_CLK, 1);
    gpio_set_level(DS1302_IO, 0);
    vTaskDelay(_delay);
    gpio_set_level(DS1302_CLK, 0);
    vTaskDelay(_delay/2);
    uint8_t bit_read = gpio_get_level(DS1302_IO);
    vTaskDelay(_delay/2);
    *data = *data | (bit_read << i) ;
  }
  gpio_set_level(DS1302_CLK, 0);
  gpio_set_level(DS1302_IO, 0);
  vTaskDelay(_delay);
  gpio_set_level(DS1302_CE, 0);
  vTaskDelay(_delay);
}
void IRAM_ATTR IR_rec_isr_handler(void* arg) 
{
    gpio_intr_disable(IR_REC);

    IR_BUFFER = 0; // reset buffer
    ir_sm.isr_triggered = true;
    uint8_t _max_wait_isr_iter = 0;

    // start sampling
    ets_delay_us(40250);
    ets_delay_us(275);
    bool _done = false;
    while (!_done && _max_wait_isr_iter < 17)
    {
      uint8_t sample_a = gpio_get_level(IR_REC);
      ets_delay_us(530);
      uint8_t sample_b = gpio_get_level(IR_REC);
      ets_delay_us(550);
      uint8_t sample_c = gpio_get_level(IR_REC);
      if (sample_a == 1 && sample_b == 1 && sample_c == 1)
      {
        _done = true;
      }
      else if (sample_b == 1 && sample_c == 0)
      {
        // short pulse
        IR_BUFFER = (IR_BUFFER << 1);
      }
      else if (sample_b == 1 && sample_c == 1)
      {
        // long pulse
        IR_BUFFER = (IR_BUFFER << 1) | (uint16_t)1;
        uint8_t _max_wait_long_pulse_iter = 0;
        while (gpio_get_level(IR_REC) != 0 && _max_wait_long_pulse_iter < 41)
        {
          ets_delay_us(40); // wait unitl next low
          _max_wait_long_pulse_iter++;
        }
        ets_delay_us(275);
      }
    }
    ets_delay_us(56000); // wait long enough to not trigger another interrupt
    gpio_intr_enable(IR_REC);
}

bool get_time_user_input(uint8_t *hour, uint8_t *min)
{
  if (IR_BUFFER == MENU && ir_sm.time_set_mode == false){
    ir_sm.time_set_mode = true;
    ir_sm.current_digit_index = 0;
    ir_sm.digits[0] = 0;
    ir_sm.digits[1] = 0;
    ir_sm.digits[2] = 0;
    ir_sm.digits[3] = 0;
    ESP_LOGI(chip_driver_str_tag, "Set time mode ENTERED");
  }
  else if (ir_sm.time_set_mode == true)
  {
    if (IR_BUFFER == MENU){ // exit time set mode
      ESP_LOGI(chip_driver_str_tag, "Set time mode EXITED");
      ir_sm.time_set_mode = false;
      return false;
    }
    else if (IR_BUFFER == ZERO)
    {
      
      ir_sm.digits[ir_sm.current_digit_index] = 0;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == ONE)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 1;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == TWO)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 2;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == THREE)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 3;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == FOUR)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 4;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == FIVE)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 5;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == SIX)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 6;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == SEVEN)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 7;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == EIGHT)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 8;
      (ir_sm.current_digit_index)++; 
    }
    else if (IR_BUFFER == NINE)
    {
      ir_sm.digits[ir_sm.current_digit_index] = 9;
      (ir_sm.current_digit_index)++; 
    } 
    if (ir_sm.current_digit_index == 4)
    { // set the time after the last digit
      *hour = ir_sm.digits[0]*10 + ir_sm.digits[1];
      *min = ir_sm.digits[2]*10 + ir_sm.digits[3];
      ir_sm.time_set_mode = false;
      return true;
    }
  }
  return false;
}

void setup_gpio(){
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << SER_L) | (1ULL << SER_R) | (1ULL << SRCLK) | (1ULL << RCLK) | (1ULL << COLON_EN) | (1ULL << DS1302_CE) | (1ULL << DS1302_CLK) | (1ULL << BLINK_GPIO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

  // DS1302 IO Pin 
  gpio_set_direction(DS1302_IO, GPIO_MODE_INPUT_OUTPUT);

  // Interrupt pin
  gpio_install_isr_service(0);
  gpio_reset_pin(IR_REC);
  gpio_set_direction(IR_REC, GPIO_MODE_INPUT);
  gpio_pullup_en(IR_REC);
  gpio_set_intr_type(IR_REC, GPIO_INTR_NEGEDGE);
  gpio_isr_handler_add(IR_REC, IR_rec_isr_handler, NULL);

}

#endif 