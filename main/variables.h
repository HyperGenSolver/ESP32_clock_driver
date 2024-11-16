#ifndef VARIABLES // Header guard to prevent multiple inclusions
#define VARIABLES

uint32_t colon_led_state = false;

uint32_t IR_BUFFER;
// Infrared sensor state machine
struct ir_sm{
    bool isr_triggered;
    bool time_set_mode;
    uint32_t current_digit_index;
    uint32_t digits[4];
} ir_sm = {false, false, 0, {-1, -1, -1, -1}};


#endif