#include <stdint.h>
#include "jump_table.h"

// Placeholder V85XX registers (replace with actual values from manual)
#define RCC_BASE        0x40021000
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define SPI1_BASE       0x40013000
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0C))
#define FLASH_BASE      0x40022000
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define CRC_BASE        0x40023000
#define CRC_DR          (*(volatile uint32_t *)(CRC_BASE + 0x00))
#define CRC_CR          (*(volatile uint32_t *)(CRC_BASE + 0x08))
#define GPIO_BASE       0x40020000
#define GPIO_PB_MODER   (*(volatile uint32_t *)(GPIO_BASE + 0x00))
#define GPIO_PC_MODER   (*(volatile uint32_t *)(GPIO_BASE + 0x100))
#define GPIO_PB_IDR     (*(volatile uint32_t *)(GPIO_BASE + 0x10))
#define GPIO_PB_ODR     (*(volatile uint32_t *)(GPIO_BASE + 0x14))
#define GPIO_PC_ODR     (*(volatile uint32_t *)(GPIO_BASE + 0x114))

void system_clock_init(void) {
    // Placeholder: Configure 6.5 MHz RC oscillator
    RCC_CR |= (1 << 0); // Enable RC oscillator
}

void spi_flash_init(void) {
    // Placeholder: Configure SPI1 (PB0: CS, PB1: SCK, PB2: MISO, PB3: MOSI)
    GPIO_PB_MODER |= (1 << 0) | (1 << 2) | (1 << 6); // PB0, PB1, PB3 output
    SPI1_CR1 |= (1 << 6); // Enable SPI1
}

void spi_flash_read(uint32_t addr, uint8_t *buffer, uint32_t len) {
    // Placeholder: Read from W25Q64
    GPIO_PB_ODR &= ~(1 << 0); // CS low
    SPI1_DR = 0x03; // Read command
    SPI1_DR = (addr >> 16) & 0xFF;
    SPI1_DR = (addr >> 8) & 0xFF;
    SPI1_DR = addr & 0xFF;
    for (uint32_t i = 0; i < len; i++) {
        SPI1_DR = 0x00; // Dummy byte
        while (!(SPI1_CR1 & (1 << 1))); // Wait for RXNE
        buffer[i] = SPI1_DR;
    }
    GPIO_PB_ODR |= (1 << 0); // CS high
}

uint32_t calc_crc(uint8_t *data, uint32_t len) {
    // Placeholder: Compute CRC32
    CRC_CR |= (1 << 0); // Reset CRC
    for (uint32_t i = 0; i < len; i++) {
        CRC_DR = data[i];
    }
    return CRC_DR;
}

void main(void) {
    system_clock_init();
    spi_flash_init();

    // Read config table from external flash (0x00000000)
    ModuleInfo config_table[2];
    spi_flash_read(0x00000000, (uint8_t *)config_table, sizeof(config_table));

    // Jump to core app
    void (*core_app)(void) = (void (*)(void))0x00001100;
    core_app();
}