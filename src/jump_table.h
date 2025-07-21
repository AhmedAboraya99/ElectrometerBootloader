#ifndef JUMP_TABLE_H
#define JUMP_TABLE_H
#include <stdint.h>

typedef struct {
    void (*uart_print)(const char *msg);
    void (*set_led)(uint8_t state);
    void (*timer_init)(void);
    void (*adc_init)(void);
    int (*read_sensor)(void);
    void (*format_data)(char *buffer, int data);
} JumpTable;

typedef struct {
    char func_name[16];
    uint32_t offset; // Virtual address
    uint32_t size;
    uint32_t crc;
    uint32_t peripheral; // Peripheral dependency (e.g., 1: UART, 2: GPIO, 3: Timer, 4: ADC)
} FunctionInfo;

typedef struct {
    uint32_t module_id;
    uint32_t address; // Virtual address (0x10000000 for Module 1, 0x10004000 for Module 2)
    uint32_t func_count;
    FunctionInfo functions[10];
} ModuleInfo;

#endif