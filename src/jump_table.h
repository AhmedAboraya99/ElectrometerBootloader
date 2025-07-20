#ifndef JUMP_TABLE_H
#define JUMP_TABLE_H

#include <stdint.h>

// Jump table for peripheral access
typedef struct {
    void (*uart_print)(const char *msg); // USART2 print
    void (*set_led)(uint8_t state);      // PC13 LED
    int (*read_sensor)(void);            // Dummy sensor
    void (*format_data)(char *buffer, int data); // Format data
} JumpTable;

// Function pointer type
typedef void (*module_function_ptr)(void *jump_table, uint32_t param);

// Function metadata
typedef struct {
    uint32_t func_index;     // Function index
    char func_name[16];      // Function name (15 chars + null)
    uint32_t offset;         // Offset in external flash
    uint32_t size;           // Function code size
    uint32_t crc;            // CRC32 of function
} FunctionInfo;

// Module metadata
typedef struct {
    uint32_t module_id;      // Module ID
    uint32_t address;        // External flash address
    uint32_t size;           // Module size
    uint32_t crc;            // Module CRC32
    uint32_t func_count;     // Number of functions
    FunctionInfo functions[20]; // Function metadata (max 20)
} ModuleInfo;

#endif