#define F_CPU 1000000
#include <stdbool.h>
#include <stddef.h>
#include <avr/io.h>
#include <util/delay.h>
#include "ace_sav.h"

#define DELAY 20
#define WRITE_ALL ((uint8_t)0xFF)
#define READ_ALL ((uint8_t)0)

#define LED (1 << PB1)
#define SR_OE (1 << PB0)
#define SR_DATA (1 << PB6)
#define SR_CLK (1 << PB7)

#define ENABLE_RAM_ADDR ((uint16_t)0)
#define ENABLE_RAM ((uint8_t)0x0A)
#define ROM_SIZE ((uint16_t)0x8000)
#define RAM_BANK_SELECT ((uint16_t)0x4000)
#define RAM_OFFSET ((uint16_t)0xA000)
#define RAM_BANK_SIZE ((uint16_t)0x2000)
#define NUM_BANKS ((uint16_t)4)
#define CHECKSTART_OFFSET ((uint16_t)0x598)
#define CHECKSUM_OFFSET ((uint16_t)0x1523)

#define BAG_DATA_OFFSET ((uint16_t)0x5C9)

#define GB_WR (1 << PC0)
#define GB_RD (1 << PC1)
#define GB_CS (1 << PC2)

void blink() {
    PORTB |= LED;
    _delay_ms(250);
    PORTB &= ~LED;
    _delay_ms(500);
}

void panic() {
    while (1) {
        blink();
    }
}

void init_gb() {
    DDRC |= GB_WR | GB_RD | GB_CS;
    DDRD = WRITE_ALL;
    PORTC |= GB_WR | GB_RD | GB_CS;
}

void init_sr() {
    DDRB |= SR_OE | SR_DATA | SR_CLK;
    PORTB &= ~(SR_DATA | SR_CLK);
    PORTB |= SR_OE;
}

void init() {
    DDRB |= LED;
    PORTB &= ~LED;
    init_gb();
    init_sr();
}

void done() {
    PORTB |= LED;
    while (1) {}
}

void shift_out(uint16_t value) {
    PORTB |= SR_OE;
    for (int i = 0; i < 16; i++) {
        uint8_t bit = (value >> i) & 1;
        if (bit == 0) {
            PORTB &= ~SR_DATA;
        } else {
            PORTB |= SR_DATA;
        }
        PORTB |= SR_CLK;
        PORTB &= ~SR_CLK;
    }
    PORTB &= ~SR_OE;
}

uint8_t read(uint16_t addr) {
    PORTC |= GB_WR | GB_RD | GB_CS;
    shift_out(addr);
    if (addr >= ROM_SIZE) {
        PORTC &= ~GB_CS;
    }
    PORTC &= ~GB_RD;
    _delay_ms(DELAY);
    uint8_t value = PIND;
    PORTC |= GB_RD;
    if (addr >= ROM_SIZE) {
        PORTC |= GB_CS;
    }
    return value;
}

void write(uint16_t addr, uint8_t value) {
    PORTC |= GB_WR | GB_RD | GB_CS;
    shift_out(addr);
    PORTD = value;
    if (addr >= ROM_SIZE) {
        PORTC &= ~GB_CS;
    }
    PORTC &= ~GB_WR;
    _delay_ms(DELAY);
    PORTC |= GB_WR;
    if (addr >= ROM_SIZE) {
        PORTC |= GB_CS;
    }
}

void select_ram_bank(uint8_t bank) {
    write(RAM_BANK_SELECT, bank);
}

void enable_ram() {
    write(ENABLE_RAM_ADDR, ENABLE_RAM);
}

void disable_ram() {
    write(ENABLE_RAM_ADDR, 0);
}

int main() {
    init();
    enable_ram();

    select_ram_bank(0);
    uint16_t pong_offset = RAM_OFFSET + 0x20;
    for (uint16_t i = 0; i < PONG_LEN; i++) {
        write(pong_offset + i, pong[i]);
    }

    select_ram_bank(1);
    uint16_t trampoline_offset = RAM_OFFSET + 0x10C0;
    for (uint16_t i = 0; i < TRAMPOLINE_LEN; i++) {
        write(trampoline_offset + i, trampoline[i]);
    }
    write(RAM_OFFSET + BAG_DATA_OFFSET, 1);
    write(RAM_OFFSET + BAG_DATA_OFFSET + 1, 0x63);
    DDRD = READ_ALL;
    uint8_t checksum = 255;
    for (uint16_t i = CHECKSTART_OFFSET; i < CHECKSUM_OFFSET; i++) {
        checksum -= read(RAM_OFFSET + i);
    }
    DDRD = WRITE_ALL;
    write(RAM_OFFSET + CHECKSUM_OFFSET, checksum);
    disable_ram();
    done();
}
