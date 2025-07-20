#include "jump_table.h"
#include <stdint.h>

// STM32F401CCU6 register base addresses
#define RCC_BASE        0x40023800
#define GPIOA_BASE      0x40020000
#define GPIOB_BASE      0x40020400
#define GPIOC_BASE      0x40020800
#define SPI1_BASE       0x40013000
#define CRC_BASE        0x40023000
#define FLASH_BASE      0x40023C00

// RCC registers
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44))

// GPIO registers
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOB_MODER     (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_BSRR      (*(volatile uint32_t *)(GPIOB_BASE + 0x18))
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00))

// SPI1 registers
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_SR         (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0C))

// CRC registers
#define CRC_DR          (*(volatile uint32_t *)(CRC_BASE + 0x00))
#define CRC_CR          (*(volatile uint32_t *)(CRC_BASE + 0x08))

// Flash registers
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x0C))

// Memory addresses
#define CORE_APP_ADDRESS 0x08001100
#define FETCHED_CODE_ADDRESS 0x08002000
#define GOT_ADDRESS 0x20000000
#define PLT_ADDRESS 0x20000064

// Initialize system clock (84 MHz)
void system_clock_init(void) {
    RCC_CR |= (1 << 0); // Enable HSI
    while (!(RCC_CR & (1 << 1))); // Wait for HSIRDY
    RCC_PLLCFGR = (336 << 6) | (8 << 24) | (1 << 22); // PLLN=336, PLLQ=8, PLLSRC=HSI
    RCC_CR |= (1 << 24); // PLLON
    while (!(RCC_CR & (1 << 25))); // Wait for PLLRDY
    RCC_CFGR = (2 << 0); // System clock = PLL
    while ((RCC_CFGR & (3 << 2)) != (2 << 2)); // Wait for SWS=PLL
}

// Initialize SPI1 for W25Q64 (PA5: SCK, PA6: MISO, PA7: MOSI, PB0: CS)
void spi_flash_init(void) {
    RCC_AHB1ENR |= (1 << 0) | (1 << 1); // Enable GPIOA, GPIOB
    RCC_APB2ENR |= (1 << 12); // Enable SPI1
    GPIOA_MODER &= ~(0xF << 10 | 0xF << 14); // Clear PA5, PA7
    GPIOA_MODER |= (2 << 10 | 2 << 14); // AF mode
    GPIOA_AFRL |= (5 << 20 | 5 << 28); // AF5 for PA5, PA7
    GPIOB_MODER &= ~(0x3); // Clear PB0
    GPIOB_MODER |= (1 << 0); // PB0 output
    GPIOB_BSRR = (1 << 0); // CS high
    SPI1_CR1 = (0 << 3) | (1 << 2) | (1 << 6); // CPHA=0, CPOL=0, MSTR=1, SPE=1
}

// Read from W25Q64 (command 0x03)
void spi_flash_read(uint32_t address, uint8_t *buffer, uint32_t length) {
    GPIOB_BSRR = (1 << 16); // CS low
    uint8_t cmd[4] = {0x03, (address >> 16) & 0xFF, (address >> 8) & 0xFF, address & 0xFF};
    for (int i = 0; i < 4; i++) {
        while (!(SPI1_SR & (1 << 1))); // Wait for TXE
        SPI1_DR = cmd[i];
        while (!(SPI1_SR & (1 << 0))); // Wait for RXNE
        (void)SPI1_DR; // Dummy read
    }
    for (uint32_t i = 0; i < length; i++) {
        while (!(SPI1_SR & (1 << 1))); // Wait for TXE
        SPI1_DR = 0xFF; // Dummy write
        while (!(SPI1_SR & (1 << 0))); // Wait for RXNE
        buffer[i] = SPI1_DR;
    }
    GPIOB_BSRR = (1 << 0); // CS high
}

// Write to internal flash
void flash_write(uint32_t address, uint8_t *data, uint32_t length) {
    FLASH_KEYR = 0x45670123; // Unlock flash
    FLASH_KEYR = 0xCDEF89AB;
    for (uint32_t i = 0; i < length; i += 4) {
        FLASH_CR = (1 << 1) | (1 << 0); // PG=1, PSIZE=word
        *(volatile uint32_t *)(address + i) = *(uint32_t *)(data + i);
        while (FLASH_SR & (1 << 16)); // Wait for BSY
    }
    FLASH_CR |= (1 << 31); // Lock flash
}

// Calculate CRC32
uint32_t calculate_crc(uint8_t *data, uint32_t length) {
    RCC_AHB1ENR |= (1 << 12); // Enable CRC clock
    CRC_CR = 1; // Reset CRC
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

// Bootloader entry
__root void bootloader_main(void) {
    system_clock_init();
    spi_flash_init();
    void (*core_app)(void) = (void (*)(void))CORE_APP_ADDRESS;
    core_app();
}