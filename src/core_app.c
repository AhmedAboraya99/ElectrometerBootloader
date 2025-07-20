#include "jump_table.h"
#include <string.h>

// V85XX register definitions (based on provided memory map)
#define RCC_BASE        0x40014000 // PMU (assumed for clock control)
#define GPIOB_BASE      0x40000020 // GPIOB
#define GPIOC_BASE      0x40000040 // GPIOC
#define UART0_BASE      0x40011800 // UART0

#define RCC_AHBENR      (*(volatile uint32_t *)(RCC_BASE + 0x10)) // AHB enable (adjust offset)
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x14)) // APB1 enable (adjust offset)
#define GPIOB_MODER     (*(volatile uint32_t *)(GPIOB_BASE + 0x00)) // Mode register
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00)) // Mode register
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x14)) // Output data
#define GPIOB_IDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x10)) // Input data
#define UART0_SR        (*(volatile uint32_t *)(UART0_BASE + 0x00)) // Status
#define UART0_DR        (*(volatile uint32_t *)(UART0_BASE + 0x04)) // Data
#define UART0_BRR       (*(volatile uint32_t *)(UART0_BASE + 0x08)) // Baud rate
#define UART0_CR1       (*(volatile uint32_t *)(UART0_BASE + 0x0C)) // Control 1

// Memory addresses
#define CONFIG_TABLE_ADDRESS 0x00000000
#define FETCHED_CODE_ADDRESS 0x00002000

// Function declarations
extern void spi_flash_read(uint32_t address, uint8_t *buffer, uint32_t length);
extern int fetch_function(uint32_t ext_address, uint32_t offset, uint32_t int_address, uint32_t size, uint32_t expected_crc);
extern int fetch_got_plt(uint32_t ext_address, uint32_t got_offset, uint32_t got_size, uint32_t plt_offset, uint32_t plt_size);
void delay_ms(uint32_t ms);
uint32_t get_tick(void);

// Initialize UART0 (PB1: TX, adjust pin per V85XX manual)
void uart_init(void) {
    RCC_AHBENR |= (1U << 1); // Enable GPIOB clock
    RCC_APB1ENR |= (1U << 0); // Enable UART0 clock (adjust)
    GPIOB_MODER &= ~(0x3U << 2); // Clear PB1
    GPIOB_MODER |= (2U << 2); // PB1 alternate function
    // Adjust alternate function per V85XX manual (e.g., AF0 for UART0)
    UART0_BRR = 0x36; // 115200 baud at 6.5 MHz (adjust: 6500000 / 115200 ˜ 56.4)
    UART0_CR1 = (1U << 13) | (1U << 3); // UE=1, TE=1
}

// UART print
void uart_print(const char *msg) {
    while (*msg) {
        while (!(UART0_SR & (1U << 7))); // Wait for TXE
        UART0_DR = *msg++;
    }
}

// Set LED (PC0, adjust pin)
void set_led(uint8_t state) {
    GPIOC_ODR = state ? 0 : (1U << 0); // PC0 output (active low, adjust)
}

// Dummy sensor
int read_sensor(void) {
    return 42;
}

// Format data
void format_data(char *buffer, int data) {
    char *p = buffer;
    *p++ = 'S'; *p++ = 'e'; *p++ = 'n'; *p++ = 's'; *p++ = 'o'; *p++ = 'r'; *p++ = ':';
    *p++ = ' ';
    int val = data;
    if (val < 0) { *p++ = '-'; val = -val; }
    char digits[10];
    int i = 0;
    do { digits[i++] = (val % 10) + '0'; val /= 10; } while (val);
    while (i--) *p++ = digits[i];
    *p++ = '\n';
    *p = '\0';
}

// Execute function
void execute_function(uint32_t code_address, JumpTable *jt, uint32_t param) {
    void (*func)(JumpTable *, uint32_t) = (void (*)(JumpTable *, uint32_t))(code_address + 1);
    func(jt, param);
}

// Core application
void core_application(void) {
    RCC_AHBENR |= (1U << 1) | (1U << 2); // Enable GPIOB, GPIOC clocks
    GPIOB_MODER &= ~(0x3U << 0); // PB0 input (button)
    GPIOC_MODER &= ~(0x3U << 0); // Clear PC0
    GPIOC_MODER |= (1U << 0); // PC0 output (LED)
    uart_init();

    JumpTable jump_table = {uart_print, set_led, read_sensor, format_data};
    ModuleInfo config[20];
    uint32_t module_id = 0;
    uint32_t func_index = 0;
    uint8_t press_count = 0;
    uint32_t last_press = 0;

    spi_flash_read(CONFIG_TABLE_ADDRESS, (uint8_t *)config, sizeof(config));
    
    while (1) {
        if (!(GPIOB_IDR & (1U << 0)) && (get_tick() - last_press > 200)) {
            press_count++;
            last_press = get_tick();
            
            if (press_count == 1) {
                module_id = (module_id % 2) + 1;
                char msg[32];
                format_data(msg, module_id);
                uart_print("Selected module ");
                uart_print(msg);
            } else if (press_count == 2) {
                for (int i = 0; i < 20; i++) {
                    if (config[i].module_id == module_id) {
                        func_index = (func_index % config[i].func_count) + 1;
                        char msg[32];
                        strncpy(msg, config[i].functions[func_index - 1].func_name, 15);
                        msg[15] = '\0';
                        uart_print("Selected function ");
                        uart_print(msg);
                        uart_print("\n");
                        break;
                    }
                }
            } else if (press_count == 3) {
                for (int i = 0; i < 20; i++) {
                    if (config[i].module_id == module_id) {
                        FunctionInfo *func = &config[i].functions[func_index - 1];
                        if (fetch_function(config[i].address, func->offset, FETCHED_CODE_ADDRESS, func->size, func->crc) &&
                            fetch_got_plt(config[i].address, 0x1000, 100, 0x1064, 50)) {
                            execute_function(FETCHED_CODE_ADDRESS, &jump_table, module_id);
                        } else {
                            uart_print("Function load failed\n");
                            while (1) { set_led(1); delay_ms(250); set_led(0); delay_ms(250); }
                        }
                        break;
                    }
                }
                press_count = 0;
            }
        } else if (press_count > 0 && get_tick() - last_press > 1000) {
            press_count = 0;
        }
        delay_ms(10);
    }
}

// Delay (approximate, 6.5 MHz)
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 6500; i++) {
        __NOP();
    }
}

// System tick (approximate)
uint32_t get_tick(void) {
    static uint32_t ticks = 0;
    ticks += 10; // 10ms per loop
    return ticks;
}