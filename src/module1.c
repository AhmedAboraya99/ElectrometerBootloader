#include "jump_table.h"

// Process data function
void process_data(JumpTable *jt, uint32_t param) {
    char buffer[32];
    jt->format_data(buffer, param);
    jt->uart_print("Processing data for module ");
    jt->uart_print(buffer);
}

// Toggle LED function
void toggle_led(JumpTable *jt, uint32_t param) {
    jt->set_led(1);
    delay_ms(100);
    jt->set_led(0);
}