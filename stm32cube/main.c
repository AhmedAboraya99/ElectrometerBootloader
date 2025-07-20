#include "jump_table.h"
#include <stdint.h>

// V85XX register base addresses (from provided memory map, adjust per manual)
#define RCC_BASE        0x40014000 // PMU (assumed for clock control)
#define GPIOB_BASE      0x40000020 // GPIOB
#define GPIOC_BASE      0x40000040 // GPIOC
#define SPI1_BASE       0x40011000 // SPI1
#define CRC_BASE        0x40016800 // RNG (assumed for CRC)
#define FLASH_BASE      0x40014000 // PMU (assumed for flash control)

// RCC registers (placeholders, adjust per V85XX manual)
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00)) // Control
#define RCC_AHBENR      (*(volatile uint32_t *)(RCC_BASE + 0x10)) // AHB enable
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x14)) // APB1 enable

// GPIO registers
#define GPIOB_MODER     (*(volatile uint32_t *)(GPIOB_BASE + 0x00)) // Mode
#define GPIOB_BSRR      (*(volatile uint32_t *)(GPIOB_BASE + 0x18)) // Bit set/reset
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00)) // Mode
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x14)) // Output data

// SPI1 registers
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00)) // Control 1
#define SPI1_SR         (*(volatile uint32_t *)(SPI1_BASE + 0x08)) // Status
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0C)) // Data

// CRC registers (placeholders, adjust per V85XX manual)
#define CRC_DR          (*(volatile uint32_t *)(CRC_BASE + 0x00)) // Data
#define CRC_CR          (*(volatile uint32_t *)(CRC_BASE + 0x08)) // Control

// Flash registers (placeholders, adjust per V85XX manual)
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x04)) // Key
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x10)) // Control
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x0C)) // Status

// Memory addresses
#define CORE_APP_ADDRESS 0x00001100
#define FETCHED_CODE_ADDRESS 0x00002000
#define GOT_ADDRESS 0x20000000
#define PLT_ADDRESS 0x20000064

// Initialize system clock (6.5 MHz RC, adjust per V85XX manual)
void system_clock_init(void) {
    RCC_CR |= (1U << 0); // Enable HSI (placeholder)
    while (!(RCC_CR & (1U << 1))); // Wait for HSI ready
    // Configure system clock to use HSI (adjust per manual)
}

// Initialize SPI1 for W25Q64 (PB0: CS, PB1: SCK, PB2: MISO, PB3: MOSI, adjust pins)
void spi_flash_init(void) {
    RCC_AHBENR |= (1U << 1) | (1U << 2); // Enable GPIOB, GPIOC clocks
    RCC_APB1ENR |= (1U << 0); // Enable SPI1 clock (adjust)
    GPIOB_MODER &= ~(0x3U << 0); // PB0 output (CS)
    GPIOB_MODER |= (1U << 0); // Output mode
    GPIOB_BSRR = (1U << 0); // CS high
    GPIOB_MODER &= ~(0xFU << 2 | 0xFU << 4 | 0xFU << 6); // Clear PB1–PB3
    GPIOB_MODER |= (2U << 2 | 2U << 4 | 2U << 6); // PB1–PB3 alternate function
    // Adjust alternate function per V85XX manual (e.g., AF0 for SPI1)
    SPI1_CR1 = (0U << 3) | (1U << 2) | (1U << 6); // CPHA=0, CPOL=0, MSTR=1, SPE=1
}

// Read from W25Q64 (command 0x03)
void spi_flash_read(uint32_t address, uint8_t *buffer, uint32_t length) {
    GPIOB_BSRR = (1U << 16); // CS low
    uint8_t cmd[4] = {0x03, (uint8_t)(address >> 16), (uint8_t)(address >> 8), (uint8_t)address};
    for (int i = 0; i < 4; i++) {
        while (!(SPI1_SR & (1U << 1))); // Wait for TXE
        SPI1_DR = cmd[i];
        while (!(SPI1_SR & (1U << 0))); // Wait for RXNE
        (void)SPI1_DR; // Dummy read
    }
    for (uint32_t i = 0; i < length; i++) {
        while (!(SPI1_SR & (1U << 1))); // Wait for TXE
        SPI1_DR = 0xFF; // Dummy write
        while (!(SPI1_SR & (1U << 0))); // Wait for RXNE
        buffer[i] = SPI1_DR;
    }
    GPIOB_BSRR = (1U << 0); // CS high
}

// Write to internal flash
void flash_write(uint32_t address, uint8_t *data, uint32_t length) {
    // Unlock flash (adjust keys per V85XX manual)
    FLASH_KEYR = 0x45670123; // Placeholder
    FLASH_KEYR = 0xCDEF89AB; // Placeholder
    for (uint32_t i = 0; i < length; i += 4) {
        FLASH_CR = (1U << 1) | (1U << 0); // PG=1, PSIZE=word (adjust)
        *(volatile uint32_t *)(address + i) = *(uint32_t *)(data + i);
        while (FLASH_SR & (1U << 16)); // Wait for BSY (adjust)
    }
    FLASH_CR |= (1U << 31); // Lock flash (adjust)
}

// Calculate CRC32 (adjust for V85XX CRC/RNG)
uint32_t calculate_crc(uint8_t *data, uint32_t length) {
    RCC_AHBENR |= (1U << 6); // Enable CRC/RNG clock (adjust)
    CRC_CR = 1U; // Reset CRC
    for (uint32_t i = 0; i < length; i += 4) {
        CRC_DR = *(uint32_t *)(data + i);
    }
    return CRC_DR;
}

// Fetch function code
int fetch_function(uint32_t ext_address, uint32_t offset, uint32_t int_address, uint32_t size, uint32_t expected_crc) {
    uint8_t buffer[1024];
    if (size > sizeof(buffer)) return 0;
    spi_flash_read(ext_address + offset, buffer, size);
    if (calculate_crc(buffer, size) != expected_crc) return 0;
    flash_write(int_address, buffer, size);
    return 1;
}

// Fetch .got and .plt
int fetch_got_plt(uint32_t ext_address, uint32_t got_offset, uint32_t got_size, uint32_t plt_offset, uint32_t plt_size) {
    uint8_t buffer[256];
    if (got_size > sizeof(buffer) || plt_size > sizeof(buffer)) return 0;
    spi_flash_read(ext_address + got_offset, buffer, got_size);
    flash_write(GOT_ADDRESS, buffer, got_size);
    spi_flash_read(ext_address + plt_offset, buffer, plt_size);
    flash_write(PLT_ADDRESS, buffer, plt_size);
    return 1;
}

// Bootloader entry point
void main(void) {
    system_clock_init();
    spi_flash_init();
    void (*core_app)(void) = (void (*)(void))CORE_APP_ADDRESS;
    core_app();
}