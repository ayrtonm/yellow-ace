#define F_CPU 1000000
#include <stdbool.h>
#include <stddef.h>
#include <avr/io.h>
#include <util/delay.h>

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
    select_ram_bank(1);
    write(RAM_OFFSET + BAG_DATA_OFFSET, 0x9C);
    write(RAM_OFFSET + BAG_DATA_OFFSET + 1, 0x63);
    disable_ram();
    done();
}
