#include <stdint.h>
#include "jump_table.h"

#define UART0_BASE      0x40011000
#define UART0_DR        (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define GPIO_BASE       0x40020000
#define GPIO_PC_MODER   (*(volatile uint32_t *)(GPIO_BASE + 0x100))
#define GPIO_PC_ODR     (*(volatile uint32_t *)(GPIO_BASE + 0x114))
#define TIM0_BASE       0x40012000
#define TIM0_CR1        (*(volatile uint32_t *)(TIM0_BASE + 0x00))
#define ADC_BASE        0x40012400
#define ADC_CR          (*(volatile uint32_t *)(ADC_BASE + 0x00))

void uart_print(const char *msg) {
    // Placeholder: Send to UART0 (PB1, 115200 baud)
    while (*msg) {
        UART0_DR = *msg++;
        while (!(UART0_DR & (1 << 7))); // Wait for TX complete
    }
}

void set_led(uint8_t state) {
    // Placeholder: Set LED (PC0)
    GPIO_PC_MODER |= (1 << 0); // Output mode
    GPIO_PC_ODR = state ? (GPIO_PC_ODR | (1 << 0)) : (GPIO_PC_ODR & ~(1 << 0));
}

void timer_init(void) {
    // Placeholder: Initialize Timer 0
    TIM0_CR1 |= (1 << 0); // Enable Timer
}

void adc_init(void) {
    // Placeholder: Initialize ADC
    ADC_CR |= (1 << 0); // Enable ADC
}

void format_data(char *buffer, int data) {
    // Placeholder: Simple decimal to string
    int i = 0;
    if (data == 0) {
        buffer[i++] = '0';
    } else {
        while (data) {
            buffer[i++] = (data % 10) + '0';
            data /= 10;
        }
    }
    buffer[i] = '\0';
}