// --------------------------------------
// MiniPirate
//
//   A human readable serial protocol
//   for basic I/O tasks.
//
// Heavily oriented at "Bus Pirate"
// http://dangerousprototypes.com/docs/Bus_Pirate_menu_options_guide
//
// Pin Layout:
// - A4: SDAp
// - A5: SCL
//
// Add command line UI and Port manipulations by O. Chatelain
// Including parts of I2C Scanner adapted by Arduino.cc user Krodal
// Including parts of Bus Pirate library by Ian Lesnet
//
// April 2014
// Using Arduino 1.0.1
//

#include <ctype.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
#include "pins_arduino.h"
#include "baseIO.h"

Servo servo;

#define I2C_LIST_SIZE 127

int i2c_address_list[I2C_LIST_SIZE];
int i2c_address_found  =  0;
int i2c_address_active = -1;

int getActiveAddress() {

  if(i2c_address_active > -1) {
    return i2c_address_list[i2c_address_active];
  } else {
    return -1;
  }

}

void printStrDec(char* str, int dec_nbre) {
  Serial.print(str);
  Serial.print(dec_nbre);
}

void printStrHex(char* str, int hex_nbre) {
  Serial.print(str);
  bpWhex(hex_nbre);
}

void printStrBin(char* str, int bin_nbre) {
  Serial.print(str);
  bpWbin(bin_nbre);
}

void listI2C() {
  for(int i = 0; i < i2c_address_found; i++) {
    Serial.print(i);
    printStrHex(": ",  i2c_address_list[i]);
    printStrBin(" - ", i2c_address_list[i]);
    Serial.println("");
  }
}

void scanI2C() {

  Serial.println("SEARCHING I2C DEVICES...");
  Serial.println("========================");

  i2c_address_found = 0;
  byte error   = 0;

  for(int address = 0; address <= 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Wire.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      i2c_address_list[i2c_address_found] = address;
      i2c_address_found++;
    }
    else if (error==4)
    {
      printStrHex("Unknow error at address ", address);
      Serial.println("");
    }
  }
  if(i2c_address_found == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.println("I2C devices found:");
    listI2C();
  }

  // Set the last address as active
  i2c_address_active = i2c_address_found - 1;
}

void helpI2C() {

  Serial.println("LIST OF SUPPORTED COMMANDS");
  Serial.println("==========================");
  Serial.println("h - Show this help");
  
  //
  // Arduino port manipulations
  //
  Serial.println("p - Show current port values & directions");

  Serial.println("< - Set a port as INPUT");
  Serial.println("> - Set a port as OUTPUT");

  Serial.println("/ - Set a port to HIGH (clock up)");
  Serial.println("\\ - Set a port to LOW (clock down)");
  Serial.println("^ - Set a port LOW-HIGH-LOW (one clock)");

  // Serial.println("b - Show bar graph of analog input");
  Serial.println("g - Set analog (pwm) value");

  Serial.println("s - Set servo value");

  //
  // I2C communication
  //
  // tbd: Serial.println("mi - Scan i2c device addresses");
  Serial.println("i - Scan i2c device addresses");
  Serial.println("# - Set i2c device active x ");
  Serial.println("r # - Read i2c n bytes from active device");
  Serial.println("w # # # - Write i2c bytes to active device");

  //
  // tbd: add SPI communication
  //
  // Serial.println("ms - spi enabled");
  // Serial.println("r # - spi read n bytes from active device");
  // Serial.println("w # # # - spi write bytes to active device");

  //
  // tbd: add LCD communication
  //
  // Serial.println("ml - LCD enabled");
  // Serial.println("r # - LCD read n bytes from active device");
  // Serial.println("w # # # - LCD write bytes to active device");

  //
  // tbd: add memory access
  //
  // Serial.println("mm - Memory access enabled");
  // Serial.println("# - Set memory position to");
  // Serial.println("r # - Read n bytes from memory");
  // Serial.println("w # # # - Write bytes to memory");

  //
  // tbd: add EEPROM access
  //
  // Serial.println("me - EEPROM access enabled");
  // Serial.println("# - Set EEPROM position to");
  // Serial.println("r # - Read n bytes from EEPROM");
  // Serial.println("w # # # - Write bytes to EEPROM");

  //
  // tbd: add FLASH access
  //
  // Serial.println("mf - Flash access enabled");
  // Serial.println("# - Set flash position to");
  // Serial.println("r # - Read n bytes from flash");
  // Serial.println("w # # # - Write bytes to flash");

  //
  // Storing a config to recover after power-up
  //
  Serial.println("x - save current config to eeprom");
  Serial.println("y - load last config from eeprom");
  Serial.println("z - set all ports to input and low");
}


char pollPeek() {
  while(Serial.available() == false);
  return Serial.peek();
}

boolean isNumberPeek() {
  char p = pollPeek();
  return p >= '0' && p <= '9';
}

boolean isBlankPeek() {
  char p = pollPeek();
  return p == ' ';
}

boolean isNumberOrBlankPeek() {
  return isNumberPeek() || isBlankPeek();
}

char pollSerial() {

  char c;
  // send data only when you receive data:
  while(Serial.available() == 0);
  c = Serial.read();
  Serial.print(c);
  return c;
}

char pollLowSerial() {
  return tolower(pollSerial());
}

void pollBlanks() {
 while(isBlankPeek()) {
   pollSerial();
 }
}

//
// Parse BIN (0bXXX), HEX (0xXXX) and DEC (XXX)
//
int pollInt() {

  pollBlanks();
  while(Serial.available() == 0);
  int value = 0;
  if(pollPeek() == '0') {
      pollSerial();
      switch(tolower(pollPeek())) {
        case 'x':
          pollSerial();
          while(   (tolower(pollPeek()) >= '0' && tolower(pollPeek()) <= '9')
                || (tolower(pollPeek()) >= 'a' && tolower(pollPeek()) <= 'f')
               ) {
            value <<= 4;
            char c = pollLowSerial();
            if(c <= '9') {
              value += c - '0';
            } else { // a-f
              value += 10 + c - 'a';
            }
          }
          return value;
          break;

        case 'b':
          pollSerial();
          while(pollPeek() == '0' || pollPeek() == '1') {
            value <<= 1;
            value += (pollSerial() == '1' ? 1 : 0);
          }
          return value;
          break;
         // default: would be octal, no clue if this is required yet.
         // take care about a "pure" zero

        default:
        break;
      }
  }
  if(isNumberPeek()) {
    value = Serial.parseInt();
    Serial.print(value);
  }
  return value;
}

int peekSerial() {

  // Peek data only when you receive data:
  while(Serial.available() == 0);
  return Serial.peek();
}

int pollPin() {

  pollBlanks();
  int pin = 0;
  if(!isNumberPeek()) {
    char c   = pollLowSerial();
    switch(c) {
      case 'a':
        pin += A0;
      case 'd':
        if(!isNumberPeek()) {
          return -1;
        }
    }
  } 
  return pin + pollInt();
}

void printHighLow(int value) {
  Serial.print(value == true ? "HIGH" : "LOW");
}

void printPin(int pin) {
  if(pin >= A0) {
    Serial.print("A");    
    pin -= A0;
  } else {
    Serial.print("D");    
  }
  Serial.print(pin);    
}

void printPorts() {
       for(int i = 0; i < 14; i++) {
         int value = digitalRead(i);

         Serial.print("Value on pin D");
         Serial.print(i);
         if(i < 10) Serial.print(' ');
         // http://garretlab.web.fc2.com/en/arduino/inside/arduino/Arduino.h/portModeRegister.html
         int pin_mode = *portModeRegister(digitalPinToPort(i)) & digitalPinToBitMask(i);
         Serial.print(( pin_mode == 0 ? " INPUT  " : " OUTPUT " ));
         Serial.print(": ");
         printHighLow(value);
         Serial.println("");
       }
       for(int i = 0; i < 6; i++) {
         int a_value = analogRead(i);
         int value = digitalRead(A0+i);

         printStrDec("Value on pin A", i);
         if(i < 10) Serial.print(' ');
         int pin_mode = *portModeRegister(digitalPinToPort(A0+i)) & digitalPinToBitMask(A0+i);
         Serial.print(( pin_mode == 0 ? " INPUT  " : " OUTPUT " ));
         Serial.print(": ");
         printHighLow(value);
         printStrDec(" / ", a_value);
         Serial.println();
       }
}

void setPin(int pin, int value) {

  pinMode(pin, OUTPUT);
  digitalWrite(pin, value);
  Serial.println("");
  Serial.print("New value on pin ");
  printPin(pin);
  if(pin < 10) Serial.print(' ');
  Serial.print(": ");
  printHighLow(value);
  Serial.println("");
}


void setup()
{

  //
  // Init the scan hit array
  //
  for(int i = 0; i++; i < I2C_LIST_SIZE) {
     i2c_address_list[i] = -1;
  }

  Wire.begin();

  Serial.begin(9600);
  Serial.println("ArduPirate: v0.1");

  // Run initial scan
  Serial.println("");
  helpI2C();
}

void loop()
{
  char c;
  Serial.println("");
  Serial.print("I2C");
  if(i2c_address_active >= 0) {
    printStrDec("[",   i2c_address_active);
    printStrHex(" - ", getActiveAddress());
    Serial.print("] ");
  }
  Serial.print("> ");
  Serial.flush();
  c = pollLowSerial();

  switch (c) {
    case 'h':
       Serial.println("");
       helpI2C();
    break;

    case 'g':
     {
       int pin_nbre = pollPin();
       pollBlanks();
       if(pin_nbre >= 0 && isNumberPeek()) {
           int value = pollInt();
           analogWrite(pin_nbre, value);
           Serial.println("");
           Serial.print("New analog value on pin ");
           printPin(pin_nbre);
           printStrDec(": ", value);
           Serial.println();
       }
     }
    break;

    case 's':
     {
       int pin = pollPin();
       pollBlanks();
       if(pin >= 0 && isNumberPeek()) {
           int value = pollInt();
           
           servo.attach(pin);
           servo.write(value);
           
           Serial.println("");
           Serial.print("New servo value on pin ");
           printPin(pin);
           printStrDec(": ", value);
           Serial.println();
           
           // Keep the position until next input
           pollPeek();
           servo.attach(pin);
       }
     }
    break;

    case '/':
     {
      int pin = pollPin();
      if(pin >= 0) {
        setPin(pin,1);
      }
     }
    break;

    case '\\':
     {
      int pin = pollPin();
      if(pin >= 0) {
        setPin(pin, 0);
      }
     }
    break;

    case '^':
     {
      int pin = pollPin();
      if(pin >= 0) {
        setPin(pin, 0);
        setPin(pin, 1);
        setPin(pin, 0);
      }
     }
    break;

    case '<':
     {
      Serial.println();
      int pin = pollPin();
      if(pin >= 0) {
        pinMode(pin, INPUT);
        Serial.println();
        Serial.print("Pin ");
        printPin(pin);
        Serial.println(" is now INPUT");
       }
      }
    break;

    case '>':
     {
      Serial.println();
      int pin = pollPin();
      if(pin >= 0) {
        pinMode(pin, OUTPUT);
        Serial.println();
        Serial.print("Pin ");
        printPin(pin);
        Serial.println(" is now OUTPUT");
       }    
      }
     break;
    
    case 'p':
       Serial.println();
       printPorts();
    break;

    case 'i':
       Serial.println();
       scanI2C();
    break;

    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case '0':
     {
       int new_address = (char)c - 48;
       if(new_address < i2c_address_found) {
         i2c_address_active = new_address;
         Serial.println("");
         Serial.print("Device changed to ");
         Serial.print(i2c_address_active);
       }
       Serial.println("");
     }
    break;

    case 'r':
     {
       pollBlanks();
       if(isNumberPeek()) {
       int read_nbre = pollInt();

       Serial.println();
       Serial.print("Requested ");
       Serial.print(read_nbre);
       Serial.print(" bytes ");

       int return_nbre = Wire.requestFrom(getActiveAddress(), read_nbre);
       Serial.print(" got ");
       Serial.print(return_nbre);
       Serial.print(" bytes from ");
       bpWhex(getActiveAddress());
       Serial.println("");

       for( int i = 0; i < return_nbre; i++) {
         while(Wire.available() == false);
         int value = Wire.read();
         bpWhex(i);
         Serial.print(": ");
         bpWbin(value);
         Serial.print(" ");
         bpWhex(value);
         Serial.print(" ");
         Serial.println(value);
       }
       Wire.endTransmission();
       }
     }
    break;

    case 'w':
     {
       int count = 0;
       Wire.beginTransmission(getActiveAddress());

       while(isNumberOrBlankPeek()) {
         pollBlanks();
         if(isNumberPeek()) {
           byte write_value = pollInt();
           Wire.write(write_value);
           count++;
         }
       }
       Wire.endTransmission();

       Serial.println();
       Serial.print("Wrote ");
       Serial.print(count);
       Serial.print(" bytes to ");
       bpWhex(getActiveAddress());
       Serial.println();
     }
    break;
    
    case 'x':
     {
       // Write all directions to EEPROM
       // Write all digital values to EEPROM
       // Write all pwm values to EEPROM
       for(int i = 0; i <= A7; i++) {
         int pin_mode  = *portModeRegister(digitalPinToPort(i)) & digitalPinToBitMask(i);// != 0;
         int pin_value = digitalRead(i);
         EEPROM.write(2*i,   pin_mode);
         EEPROM.write(2*i+1, pin_value);
       }
     }
     Serial.println();
     Serial.print("Saved digital pins to EEPROM");
    break;
    
    case 'y':
     {
       // Read all directions to EEPROM
       // Read all digital values to EEPROM
       // Read all pwm values to EEPROM
       for(int i = 0; i <= A7; i++) {
         int pin_mode  = EEPROM.read(2*i);
         int pin_value = EEPROM.read(2*i + 1);
         pinMode(i, pin_mode);
         //if(pin_mode != 0) {
           digitalWrite(i, pin_value);
         //}
       }
     }
     Serial.println();
     Serial.println("Loaded digital pins from EEPROM");
     printPorts();
     break;
     
   case 'z':
     Serial.println();
     Serial.print("Reset ...");
     for(int i = 0; i <= A7; i++) {
       pinMode(i, INPUT);
       digitalWrite(i, LOW);
     }
     break;
  }
}
