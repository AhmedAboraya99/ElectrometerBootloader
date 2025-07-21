#include <stdint.h>
#include "jump_table.h"

// Placeholder V85XX registers
#define GPIO_BASE       0x40020000
#define GPIO_PB_IDR     (*(volatile uint32_t *)(GPIO_BASE + 0x10))
#define GPIO_PC_ODR     (*(volatile uint32_t *)(GPIO_BASE + 0x114))
#define UART0_BASE      0x40011000
#define UART0_DR        (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define FLASH_BASE      0x40022000
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x04))

#define TRACE_ENABLED // Enable function tracing (disable in production)

#ifdef TRACE_ENABLED
static uint32_t trace_buffer[16]; // Store up to 16 function addresses
static uint32_t trace_index = 0;

// Shared peripheral functions (in .shared section)
void uart_print(const char *msg);
void set_led(uint8_t state);
void timer_init(void);
void adc_init(void);
void format_data(char *buffer, int data);

void trace_function(void *func_addr) {
    if (trace_index < 16) {
        trace_buffer[trace_index++] = (uint32_t)func_addr;
        char buffer[32];
        format_data(buffer, (int)func_addr);
        uart_print("Trace: Function at 0x");
        uart_print(buffer);
    }
}
#endif

// From bootloader.c
void spi_flash_read(uint32_t addr, uint8_t *buffer, uint32_t len);
uint32_t calc_crc(uint8_t *data, uint32_t len);

void core_application(void) {
    // Read config table from external flash
    ModuleInfo config_table[2];
    spi_flash_read(0x00000000, (uint8_t *)config_table, sizeof(config_table));

    JumpTable jt = { uart_print, set_led, timer_init, adc_init, 0, format_data };
    uint32_t selected_module = 0;
    uint32_t selected_func = 0;

    while (1) {
        // Check button press (PB0, assumed)
        if (GPIO_PB_IDR & (1 << 0)) {
            if (selected_module == 0) {
                selected_module = 1; // Select Module 1
                jt.uart_print("Selected module 1");
            } else if (selected_func == 0) {
                selected_func = 1; // Select process_data or read_sensor
                jt.uart_print("Selected function");
            } else {
                // Load and execute function
                ModuleInfo *mod = &config_table[selected_module - 1];
                FunctionInfo *func = &mod->functions[selected_func - 1];
                uint8_t buffer[1024];

                // Check peripheral dependency
                if (func->peripheral & 0x4) { // Requires Timer
                    jt.timer_init();
                }
                if (func->peripheral & 0x8) { // Requires ADC
                    jt.adc_init();
                }

                // Map virtual to physical address
                uint32_t physical_addr = func->offset - 0x10000000 + (selected_module == 1 ? 0x00001000 : 0x00004000);
                spi_flash_read(physical_addr, buffer, func->size);

                // Verify CRC
                if (calc_crc(buffer, func->size) == func->crc) {
                    // Unlock flash
                    FLASH_KEYR = 0x45670123;
                    FLASH_KEYR = 0xCDEF89AB;

                    // Write to 0x00002000 (overlay)
                    uint32_t *dest = (uint32_t *)0x00002000;
                    for (uint32_t i = 0; i < func->size / 4; i++) {
                        dest[i] = ((uint32_t *)buffer)[i];
                    }

                    // Execute function
                    void (*func_ptr)(JumpTable *, uint32_t) = (void (*)(JumpTable *, uint32_t))0x00002000;
                    #ifdef TRACE_ENABLED
                    trace_function((void *)func_ptr);
                    #endif
                    func_ptr(&jt, selected_module);

                    // Reset selection
                    selected_module = 0;
                    selected_func = 0;
                } else {
                    // Error: Blink LED (PC0)
                    for (int i = 0; i < 10; i++) {
                        jt.set_led(1);
                        for (volatile int j = 0; j < 500000; j++);
                        jt.set_led(0);
                        for (volatile int j = 0; j < 500000; j++);
                    }
                }
            }
            // Debounce
            for (volatile int i = 0; i < 100000; i++);
        }
    }
}