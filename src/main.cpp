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
unsigned long long orig_pos_deg;

const int numBytes = 8;
int currBit;
int prevBit;
double offset[4];
int i;

// first byte flag
bool initBit = true; //turns false after first reading 
bool test = false;

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
/*
int get_start_bit(int index, byte* buffer) {     
  
  // setMode(active_codes[index]);
  // SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE3));
  // for (int i = 0; i < numBytes; i++) {
  //   buffer[i] = SPI.transfer(0x00);
  // }
  // SPI.endTransaction();
  prevBit = 0;
  int count=0;
  int pos = 0; 
  for (int i=0; i < numBytes; i++) {     // Iterate through each byte (left to right)
    for (int j=0; j < 8; j++) {          // Iterate through each bit  (left to right)
      currBit = bitRead(buffer[i], 7-j); // Read the current bit
      if ((prevBit)&&!(currBit)) {
        count+=1;
      }
      if (count>1) {                     // Check for two falling edges
        pos = j+(j*i)+1;
        //Serial.println(offset);
        break;                           // Yeet and skeet
      }
      prevBit=currBit;
    }
    if (count>1) {
      break;
    }
  }
  //pos = get_simple_position(index);
  return pos;
}
*/
/*
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
*/

double OffsetConvert(int pin, double pos) {
    //double Display_Angle_Integer;
    double adj_ang;
    if (pos >= offset[pin]) {
      // in this case the subtraction will not go negative
      // so we just go ahead and do the subtraction
      adj_ang = pos - offset[pin];
    }
    else {
      // in this case, the subtraction would go negative
      // so we do an addition instead
      adj_ang = pos + (360.0 - offset[pin]);
    }
    return adj_ang; 
}

double get_simple_position(int j) {
      byte buffer[5];
      setMode(active_codes[j]);
      SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE3)); //250kHz aka 4micro seconds
      for (int i = 0; i < sizeof(buffer); i++) { //go through each byte and put it in the buffer
        buffer[i] = SPI.transfer(0x00);
      }
      SPI.endTransaction();
      //Serial.println(String(buffer[0], BIN)+String(buffer[1], BIN)+String(buffer[2], BIN)+String(buffer[3], BIN));
      switch (j) {
        case 0: //RESA30 1
          // 32-bit mode
          // Shift bits to eliminate ack and start and only accept position data
          positiona = ((unsigned long)(buffer[1]) << 24 | (unsigned long)(buffer[2]) << 16 | (unsigned long)(buffer[3]) << 8 | (unsigned long)(buffer[4]));
          return (positiona/pow(2,32)*360.0); // (binary value / counts per rev) * 360 => degrees, 8: how many decimal points to spit out
          break;
        case 1: //RESA30 2
          // 32-bit mode
          positionb = ((unsigned long)(buffer[1]) << 24 | (unsigned long)(buffer[2]) << 16 | (unsigned long)(buffer[3]) << 8 | (unsigned long)(buffer[4]));
          // averaging logic from the Renishaw whitepaoer TODO: put that on the teams in "COTS_Datasheets/Renishaw"
          if(positionb>positiona) {
            position=positiona+positionb;
          }
          else {
            position=(positiona+positionb+0x100000000)&0x1ffffffff;
          }
          return position/pow(2,33)*360.0;
          break;
        case 2: // Zettlex (funny because his BISSC is wrong, there is no start bit)
          // 18-bit mode
          position = ((unsigned long)(buffer[0] & 0b11) << 16 | (unsigned long)(buffer[1]) << 8 | (unsigned long)(buffer[2]) );
          // Serial.println(position, BIN);
          return (position/pow(2,18)*360.0);
          break;
        case 3: // Netzer
          // 20-bit mode
          position = ((unsigned long)(buffer[0] & 0b111) << 17 | (unsigned long)(buffer[1]) << 9 | (unsigned long)(buffer[2]) << 1 | (unsigned long)(buffer[3]) >> 7);
          return (position/pow(2,20)*360.0); 
          break;
      }
}

void print_bin(double integer) {
    Serial.print(integer, 8);
    Serial.print(String(","));
}

void loop() {
    //byte* buffer;
    double adjusted_angle;
    double raw_angle;
    if(test){
        raw_angle = get_simple_position(2);
        adjusted_angle = OffsetConvert(2, raw_angle);
        //print_bin(raw_angle);
        //Serial.println(raw_angle,8);
        //Serial.print("        ");
        //Serial.println(adjusted_angle);
    }else{
    for (int j = 0; j < 4; j++) {
      if (initBit) { //only runs for first loop
        offset[j] = get_simple_position(j);
        i++;
        if (i==8){
          initBit = false;
          Serial.print(String("flag set to false and offset angles are:  "));
          Serial.print( offset[0] ,8);
          Serial.print(String(", "));
          Serial.print( offset[1], 8 );
          Serial.print(String(", "));
          Serial.print( offset[2], 8 );
          Serial.print(String(", "));
          Serial.println( offset[3], 8 );
          Serial.println("FIRST ANGLE OUT:  ");
        }
      }
      else {
        raw_angle = get_simple_position(j);
        adjusted_angle = OffsetConvert(j, raw_angle);
        print_bin(raw_angle);
        //Serial.print(raw_angle);
        //Serial.print("        ");
        //Serial.println(adjusted_angle);
        //print_bin(adjusted_angle);
      }
      delay(cpsl);
    }
    }
    Serial.println(" ");
    delay(loop_period-4*cpsl);
    //Serial.println("Clock:  " + String(loop_period-4*cpsl));
 
}
