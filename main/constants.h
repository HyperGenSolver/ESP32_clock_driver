#ifndef CONSTANTS // Header guard to prevent multiple inclusions
#define CONSTANTS

uint16_t SER_L =        GPIO_NUM_32; // Serial clock pin for the left shift register
uint16_t SER_R =        GPIO_NUM_26; // Serial clock pin for the right shift register
uint16_t SRCLK =        GPIO_NUM_33;
uint16_t RCLK =         GPIO_NUM_25;

uint16_t COLON_EN =     GPIO_NUM_27;

uint16_t DS1302_CE =     GPIO_NUM_14;
uint16_t DS1302_IO =     GPIO_NUM_23;
uint16_t DS1302_CLK =    GPIO_NUM_13;

uint16_t IR_REC =    GPIO_NUM_22;
uint16_t BLINK_GPIO =   GPIO_NUM_2;

unsigned char k155id1_bit_pattern[] =
    {0b00000000, // '0' output
     0b00000001, // '1' output
     0b00000010, 
     0b00000011, 
     0b00000100, 
     0b00000101, 
     0b00000110, 
     0b00000111, 
     0b00001000, 
     0b00001001 // '9'
     };
uint16_t SHIFT_REG_FREQ = 20; // shift register clock frequency [HZ]
uint16_t DS1302_FREQ = 100; // shift register clock frequency [HZ]

// IR remote control codes
const uint32_t MENU = 115771;
const uint32_t ZERO = 53551;
const uint32_t ONE = 24991;
const uint32_t TWO = 12751;
const uint32_t THREE = 62731;
const uint32_t FOUR = 8671;
const uint32_t FIVE = 29071;
const uint32_t SIX = 46411;
const uint32_t SEVEN = 34171;
const uint32_t EIGHT = 38251;
const uint32_t NINE = 42331;
const uint32_t ON_OFF = 83131;

const uint16_t DISPLAY_NIXI_FREQ = 500; // Frequency of the nixie tube display update [ms]

const uint16_t MAX_DELAY_TEMP_UPDATE_DISPLAY = 6000; // Maximum delay between temperature updates displayed on the nixie tubes [ms]
const uint16_t TIME_TEMPERATURE_IS_SHOWN = 3000; // Time the temperature is shown on the nixie tubes [ms]
#endif 