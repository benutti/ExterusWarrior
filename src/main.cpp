/*Sample code for IEC62386 for send command/DACP and get QUERY on an ESP32*/

#include <Arduino.h>
#include <Wire.h>
#include <Dali.h>    //List of command bytes for IEC62386-102:2009 standard

/*!ATTENTION!*/
//#define NOP() asm volatile ("nop") //Workaround for delayMicrosecondeExt function


//LW14 I2C address 7Bit
#define LW14_I2C 0x23 // 8Bit: 0x46

//LW14 I2C command register
#define I2C_REG_STATUS          0x00   //read only, 1 byte
#define I2C_REG_COMMAND         0x01   //read/write, 2 bytes
#define I2C_REG_CONFIG          0x02   //write only, 1 byte
#define I2C_REG_SIGNATURE       0xF0   //read only, 6 bytes
#define I2C_REG_ADDRESS         0xFE   //write only, 2 bytes

//Status bits for register I2C_REG_STATUS
#define STATUS_1BYTE            0x01
#define STATUS_2BYTE            0x02
#define STATUS_3BYTE            0x03
#define STATUS_TIMEFRAME        0x04
#define STATUS_VALID            0x08
#define STATUS_FRAME_ERROR      0x10
#define STATUS_OVERRUN          0x20
#define STATUS_BUSY             0x40      //0: ready, 1: busy
#define STATUS_BUS_FAULT        0x80      //0: OK, 1: bus fault


void setup() 
{
    Serial.begin(115200);
    Dali dali;
    dali.begin();
}

void loop() 
{


    
}
