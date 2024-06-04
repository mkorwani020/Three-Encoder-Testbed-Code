#include <SPI.h>
#include <Arduino.h>

const int Renishaw1Pin = 29;
const int Renishaw2Pin = 27;
const int ZettlexPin   = 23;
const int NetzerPin    = 25;

const int loop_period  = 100;
const int cpsl         = 3;

// string order[4]   = {"Renishaw1", "Renishaw2", "Zettelx", "Netzer"};
const int pins[4]    = {Renishaw1Pin, Renishaw2Pin, ZettlexPin, NetzerPin};

const byte active_codes[4] = {0b0111, 0b1011, 0b1101, 0b1110};

unsigned long long position;
unsigned long long positiona;
unsigned long long positionb;

const int numBytes = 8;
int currBit;
int prevBit;
int offset;

void setup() {
  SPI.begin();
  Serial.begin(9600);

  for (int i = 0; i < 4; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);
  }
  // (ZettlexPin, 0);
}

void setMode(byte value) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(pins[i], bitRead(value, 3-i));
  }
}

byte* transact(int index) {
  static byte buffer[numBytes];
  setMode(active_codes[index]);
  SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE3));
  for (int i = 0; i < numBytes; i++) {
    buffer[i] = SPI.transfer(0x00);
  }
  SPI.endTransaction();
  return buffer;
}

int get_start_bit(int index, byte* buffer) {     
  
  // setMode(active_codes[index]);
  // SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE3));
  // for (int i = 0; i < numBytes; i++) {
  //   buffer[i] = SPI.transfer(0x00);
  // }
  // SPI.endTransaction();
  prevBit = 0;
  int count=0;
  for (int i=0; i < numBytes; i++) {     // Iterate through each byte (left to right)
    for (int j=0; j < 8; j++) {          // Iterate through each bit  (left to right)
      currBit = bitRead(buffer[i], 7-j); // Read the current bit
      if ((prevBit)&&!(currBit)) {
        count+=1;
      }
      if (count>1) {                     // Check for two falling edges
        offset = j+(j*i)+1;
        Serial.println(offset);
        break;                           // Yeet and skeet
      }
      prevBit=currBit;
    }
    if (count>1) {
      break;
    }
  }
  return offset;
}

unsigned long get_position(int index, int offset, byte* buffer) {
  byte bitmask = 0b11111111 >> offset;
  int bitsize;
  switch (index) {
    case 2:
      // 21-bit mode (ZETTLEX HAS NO START BIT, so the offset is always 7)
      position = ((unsigned long)(buffer[0] & 0b1) << 20 | (unsigned long)(buffer[1]) << 12 | (unsigned long)(buffer[2]) << 4 | (unsigned long)(buffer[3]) >> 4);
      // Serial.print(position/pow(2,21)*360.0+String(", "));
      // sprintf(out, "%lu, ", position);
      // Serial.print(out);
      break;
    case 0 || 1:
      bitsize = 32;
      break;
    case 3:
      bitsize = 20;
      break;
  }
  position = (unsigned long)(buffer[0] & bitmask) << bitsize-(8-offset);
    for (int i = 1; i < 5; i++) {
      position = position | (unsigned long)(buffer[i]) << bitsize-(8-offset)-8*i;
    }
  return position;
}

void get_simple_position(int j) {
      byte buffer[8];
      char out[34];
      setMode(active_codes[j]);
      // Serial.print(active_codes[j],2);
      SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE3));
      for (int i = 0; i < 8; i++) {
          buffer[i] = SPI.transfer(0x00);
      }
      SPI.endTransaction();
      switch (j) {
        case 0:
          // 32-bit mode
          positiona = ((unsigned long)(buffer[1]) << 24 | (unsigned long)(buffer[2]) << 16 | (unsigned long)(buffer[3]) << 8 | (unsigned long)(buffer[4]));
          // Serial.print(position,BIN);
          Serial.print(positiona/pow(2,32)*360.0, 8);
          Serial.print(String(","));
          // sprintf(out, "%lu, ", position);
          // Serial.print(out);
          break;
        case 1:
          // 32-bit mode
          positionb = ((unsigned long)(buffer[1]) << 24 | (unsigned long)(buffer[2]) << 16 | (unsigned long)(buffer[3]) << 8 | (unsigned long)(buffer[4]));
          if(positionb>positiona) {
            position=positiona+positionb;
          }
          else {
            position=(positiona+positionb+0x100000000)&0x1ffffffff;
          }
          
          Serial.print(position/pow(2,33)*360.0, 8);
          Serial.print(String(","));
          // sprintf(out, "%lu, ", position);
          // Serial.print(out);
          break;
        case 2:
          // 21-bit mode
          position = ((unsigned long)(buffer[0] & 0b1) << 20 | (unsigned long)(buffer[1]) << 12 | (unsigned long)(buffer[2]) << 4 | (unsigned long)(buffer[3]) >> 4);
          Serial.print(position/pow(2,21)*360.0, 8);
          Serial.print(String(","));
          // sprintf(out, "%lu, ", position);
          // Serial.print(out);
          break;
        case 3:
          // 20-bit mode
          position = ((unsigned long)(buffer[0] & 0b111) << 17 | (unsigned long)(buffer[1]) << 9 | (unsigned long)(buffer[2]) << 1 | (unsigned long)(buffer[3]) >> 7);
          Serial.print(position/pow(2,20)*360.0, 8);
          Serial.println(String(","));
          // sprintf(out, "%lu, ", position);
          // Serial.print(out);
          break;
      }
}

void print_bin(unsigned long integer) {

}

void loop() {
    byte* buffer;
    for (int j = 0; j < 4; j++) {
      get_simple_position(j); 
      // get_position_start_bit(j);
      // Serial.println(j+String(", "));
      // buffer = transact(j);
      
      // Serial.println(buffer[0],BIN);
      // offset = get_start_bit(j, buffer);
      // delay(cpsl);
    }
    // Serial.println("--");
  delay(loop_period-4*cpsl);
}