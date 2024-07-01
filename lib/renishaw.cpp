
#include "Renishaw.h"

#define BUFFER_SIZE 10

Renishaw::Renishaw(int CS_Pin, bool invert) {
    CS = CS_Pin;
    inverted = invert;
    pinMode(CS, OUTPUT);
    disable();
}

void Renishaw::enable() {
    // delayMicroseconds(250);
    delayMicroseconds(1250);
    digitalWrite(CS, LOW);
    delayMicroseconds(1250);
}

uint32_t Renishaw::getPosition() {
    enable();
    SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE3));
    byte buffer[BUFFER_SIZE];
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    SPI.endTransaction();

    // For 2.5MHz Clock
    // uint32_t position = ((uint32_t)(buffer[2] & 0b111) << 29) |
    //                          ((uint32_t)buffer[3] << 21) |
    //                          ((uint32_t)buffer[4] << 13) |
    //                          ((uint32_t)buffer[5] << 5) |
    //                          ((uint32_t)buffer[6] >> 3);

    // For 250kHz Clock
    uint32_t position = ((uint32_t)(buffer[1] & 0b111111) << 26) |
                        ((uint32_t)buffer[2] << 18) |
                        ((uint32_t)buffer[3] << 10) |
                        ((uint32_t)buffer[5] <<  2) |
                        ((uint32_t)buffer[6] >>  6);

    position = inverted ? (pow(2, 32) - position) : position;

    disable();
    return position;
}

void Renishaw::disable() {
    // delayMicroseconds(250);
    delayMicroseconds(1250);
    digitalWrite(CS, HIGH);
    delayMicroseconds(1250);
}
