#ifndef RENISHAW_H
#define RENISHAW_H

#include <Arduino.h>
#include <SPI.h>

class Renishaw {
private:
    int CS;
    bool inverted;

public:
    Renishaw(int CS_Pin, bool inverted);

    void enable();
    uint32_t getPosition();
    void disable();
};

#endif