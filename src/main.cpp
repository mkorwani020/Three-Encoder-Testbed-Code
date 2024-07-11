#include <config.h>


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

unsigned long long get_pos_start_bit(int index, byte* buffer) {    
  unsigned long long position;
  prevBit = 0;
  int count=0;
  int offset = 0;
  for (int i=0; i < numBytes; i++) {     // Iterate through each byte (left to right)
    for (int j=0; j < 8; j++) {          // Iterate through each bit  (left to right)
      currBit = bitRead(buffer[i], 7-j); // Read the current bit (7-j for left to right)
      if ((prevBit)&&!(currBit)) {       // Here, the two adjacent bits are NOT the same
        count+=1;
      }
      if ((count>1)) {                     // If the two adjacent bits are NOT the same twice in a row, you have
        offset = j+(j*i)+1;                // reached the end of the ack period AND the start bits
        break;
      prevBit=currBit;
    }
  }
  }
  Serial.print(offset+String(" "));
  byte bitmask = 0b11111111 >> offset;
  position = (unsigned long)(buffer[0] & bitmask) << resolution[index]-(8-offset);
    for (int i = 1; i < 5; i++) {
      position = position | (unsigned long)(buffer[i]) << resolution[index]-(8-offset)-8*i;
    }
  return position;
}
 
double get_position(int j) {
  byte* buffer;
  buffer = transact(j);
  switch (j) {
    case 0: //RESA30 1
      // 32-bit mode
      // Shift bits to eliminate ack and start and only accept position data
      positiona = get_pos_start_bit(j, buffer);
      return (positiona/pow(2,32)*360.0)* direction[j]; // (binary value / counts per rev) * 360 => degrees, 8: how many decimal points to spit out
      break;
    case 1: //RESA30 2
      // 32-bit mode
      positionb = get_pos_start_bit(j, buffer);
      // averaging logic from the Renishaw whitepaoer TODO: put that on the teams in "COTS_Datasheets/Renishaw"
      if(positionb>positiona) {
        position=positiona+positionb;
      }
      else {
        position=(positiona+positionb+0x100000000)&0x1ffffffff;
      }
      return position/pow(2,33)*360.0 * direction[j];
      break;
    case 2: // Zettlex (funny because his BISSC is wrong, there is no start bit)
      // 18-bit mode
      position = ((unsigned long)(buffer[0] & 0b11) << 16 | (unsigned long)(buffer[1]) << 8 | (unsigned long)(buffer[2]) );
      // Serial.println(position, BIN);
      return (position/pow(2,18)*360.0)* direction[j];
      break;
    case 3: // Netzer
      // 20-bit mode
      position = get_pos_start_bit(j, buffer);
      return (position/pow(2,20)*360.0)* direction[j];
      break;
  }
}

double get_simple_position(int j) {
      byte buffer[5];
      byte bitmask = 0b11111111 >> bit_offsets[j];
      setMode(active_codes[j]);
      SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE3)); //250kHz aka 4micro seconds
      for (int i = 0; i < sizeof(buffer); i++) { //go through each byte and put it in the buffer
        buffer[i] = SPI.transfer(0x00);
      }
      SPI.endTransaction();
      // Serial.println(String(buffer[0], BIN)+String(buffer[1], BIN)+String(buffer[2], BIN)+String(buffer[3], BIN)+String(buffer[4], BIN));
      switch (j) {
        case 0: //RESA30 1
          // 32-bit mode
          // Shift bits to eliminate ack and start and only accept position data
          positiona = ((unsigned long)(buffer[1]) << 24 | (unsigned long)(buffer[2]) << 16 | (unsigned long)(buffer[3]) << 8 | (unsigned long)(buffer[4]));
          // positiona = (unsigned long)(buffer[0] & bitmask) << resolution[j]-(8-bit_offsets[j]);
          // for (int i = 1; i < 5; i++) {
          //   int shift = resolution[j]-(8-bit_offsets[j])-8*i;
          //   if (shift >= 0) {
          //     positiona = positiona | (unsigned long)(buffer[i]) << shift;
          //   }
          //   else {
          //     positiona = positiona | (unsigned long)(buffer[i]) >> -1*shift;
          //   }
          // }          
          return (positiona/pow(2,32)*360.0)* direction[j]; // (binary value / counts per rev) * 360 => degrees, 8: how many decimal points to spit out
          break;
        case 1: //RESA30 2
          // 32-bit mode
          positionb = ((unsigned long)(buffer[1]) << 24 | (unsigned long)(buffer[2]) << 16 | (unsigned long)(buffer[3]) << 8 | (unsigned long)(buffer[4]));
          // averaging logic from the Renishaw whitepaoer TODO: put that on the teams in "COTS_Datasheets/Renishaw"
          return positionb/pow(2,32)*360.0* direction[j];
          break;
        case 2: // Zettlex (funny because his BISSC is wrong, there is no start bit)
          // 18-bit mode
          position = ((unsigned long)(buffer[0] & 0b11) << 16 | (unsigned long)(buffer[1]) << 8 | (unsigned long)(buffer[2]) );
          // Serial.println(position, BIN);
          return (position/pow(2,18)*360.0)* direction[j];
          break;
        case 3: // Netzer
          // 20-bit mode
          position = ((unsigned long)(buffer[0] & 0b111) << 17 | (unsigned long)(buffer[1]) << 9 | (unsigned long)(buffer[2]) << 1 | (unsigned long)(buffer[3]) >> 7);
          return (position/pow(2,20)*360.0)* direction[j]; 
          break;
      }
}

void print_bin(double integer, int j) {
    Serial.print(integer, 8);
    if (j!=3) {
    Serial.print(String(","));
    }
}

double renishaw_adjustment() {
  if(positionb>positiona) {
    position_adj=positiona+positionb;
    }
    else {
      position_adj=(positiona+positionb+0x100000000)&0x1ffffffff;
    }
    return position_adj/pow(2,33)*360.0*-1.0;
}

void loop() {
    //byte* buffer;
    double adjusted_angle;
    double raw_angle;
    double avrg_offset;

    int k;
    if(test){
        raw_angle = get_simple_position(3);
        adjusted_angle = OffsetConvert(3, raw_angle);
        // print_bin(raw_angle);
        //Serial.println(raw_angle,8);
        //Serial.println(adjusted_angle);
    }else{
    for (int j = 0; j < 4; j++) {
      k = order[j];
      if (initBit) { //only runs for first loop
        offset[k] = get_simple_position(k);
        if (k==1) {
          offset[4] = renishaw_adjustment();
        }
        i++;
        if (i==8){
          initBit = false;
          
        }
      }
      else {
        raw_angle = get_simple_position(k);
        adjusted_angle = OffsetConvert(k, raw_angle);
        print_bin(adjusted_angle, j);
        if (k==1) {
          raw_angle = renishaw_adjustment();
          adjusted_angle = OffsetConvert(4, raw_angle);
          print_bin(adjusted_angle, j);
        }
      }
      delay(cpsl);
    }
    }
    Serial.println(" ");
    delay(loop_period-4*cpsl);
    //Serial.println("Clock:  " + String(loop_period-4*cpsl));
 
}
