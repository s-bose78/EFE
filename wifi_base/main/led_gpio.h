#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_2 // Define the GPIO pin connected to the LED

void gpio_configure(int pin);
void set_gpio(int pin, uint32_t state);