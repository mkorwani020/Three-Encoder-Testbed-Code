#include <SPI.h>
#include <Arduino.h>
 
const int Renishaw1Pin = 25;
const int Renishaw2Pin = 27;
const int ZettlexPin   = 23;
const int NetzerPin    = 29;
 
const int loop_period  = 20;
const int cpsl         = 3;
 
// string order[4]   = {"Renishaw1", "Renishaw2", "Zettelx", "Netzer"};
const int pins[4]    = {Renishaw1Pin, Renishaw2Pin, ZettlexPin, NetzerPin};
const int resolution[4] = {32,32,18,20};
const int direction[4] = {-1,-1,1,1}; 

const byte active_codes[4] = {0b0111, 0b1011, 0b1101, 0b1110};
 
 