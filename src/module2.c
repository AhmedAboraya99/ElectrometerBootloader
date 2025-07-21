#include "jump_table.h"

// Module 2 function
void other_func(JumpTable *jt, uint32_t param) {
    char buffer[32];
    jt->format_data(buffer, param);
    jt->uart_print("Other function for module ");
    jt->uart_print(buffer);
}

