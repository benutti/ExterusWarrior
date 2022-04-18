#include <Arduino.h>
#include <Wire.h>
#include "Dali.h"
#include "iec_62386.h"

//LW14 Specific
#define I2C_ADDRESS             0x23  //default I2C address for LW14

#define I2C_REG_STATUS          0x00  //read only, 1 byte
#define I2C_REG_COMMAND         0x01  //read/write, 2 bytes
#define I2C_REG_CONFIG          0x02  //write only, 1 byte
#define I2C_REG_SIGNATURE       0xF0  //read only, 6 bytes
#define I2C_REG_ADDRESS         0xFE  //write only, 2 bytes

//status bits
#define STATUS_1BYTE            0x01
#define STATUS_2BYTE            0x02
#define STATUS_TIMEFRAME        0x04
#define STATUS_VALID            0x08
#define STATUS_FRAME_ERROR      0x10
#define STATUS_OVERRUN          0x20
#define STATUS_BUSY             0x40  //0: ready, 1: busy
#define STATUS_BUS_FAULT        0x80  //0: OK, 1: bus fault

//Dali bits
#define DALI_MODE_CMD           0x01
#define DALI_MODE_DACP          0x00
#define DALI_SHORT              0x00
#define DALI_GROUP              0x80
#define DALI_MAX_SHORT          0x3F  //63 devices
#define DALI_MAX_GROUP          0x0F  //15 groups
#define DALI_MAX_SCENE          0x0F  //15 scenes

// This is your main class that users will import into their application
class Dali
{
public:
  /**
   * Constructor
   */
  Dali();

/*
Begin; set the address of the LED Warrior
*/
  void begin();

/*Create valid DALI address. (Please take a look into the datasheet or DALI DIN spec)
Address pattern: S AAA AAA Y
S for group bit
AAA AAA (adr) for DALI device 0...63 or group 0...15
Y for command or DACP mode
*/
  byte get_dali_address(byte s, byte adr, byte y);

//Wait until DALI bus is ready to send next data. DALI is slow!
  byte wait_for_ready();

//Read 1 byte from command register of LW14
  byte read_data();

//Send value to DALI device
  void send_command(int a, int b);

//Get a query value from dali device
  byte read_query(int a, int b);

//Test function to send DACP value (direct dimm) to device 0...63
  void test_dacp(byte adr, byte value);

//Send DALI command OFF to device 0...63
  void test_cmd(byte adr, byte value);

//Send DALI command OFF to all devices (broadcast)
  void test_broadcast(byte value);

//Get ActualLevel from DALI device 0...63
  void test_query(byte adr, byte value);
};

/**
 * Constructor.
 */
Dali::Dali()
{}

/**
 * Functions
 */
void Dali::begin()
{
    Wire.begin(21,22);
};

byte Dali::get_dali_address(byte s, byte adr, byte y)
{
  return (adr << 1) | s | y;
};

byte Dali::wait_for_ready()
{
  byte result = 0x00;
  
  while (true)
  {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(I2C_REG_STATUS);
    Wire.endTransmission();
    Wire.requestFrom(I2C_ADDRESS, (uint8_t)1);
    result = Wire.read();

    if ((result & STATUS_BUSY) == 0x00)          //exit if busyflag is gone
      break;
  }
    
  return result;
};

byte Dali::read_data()
{
    Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(I2C_REG_COMMAND);
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADDRESS, (uint8_t)1);
  return Wire.read();
};

void Dali::send_command(int a, int b)
{
  Wire.beginTransmission(I2C_ADDRESS);  //start transmitting to I2C device X
  Wire.write(I2C_REG_COMMAND);          //Set I2C register for commands
  Wire.write((byte) a);                 //Data byte 1 (f.e. DALI address)
  Wire.write((byte) b);                 //Data byte 2 (f.e. DALI value)
  Wire.endTransmission();               // stop transmitting
};

byte Dali::read_query(int a, int b)
{
  send_command(a, b);
  wait_for_ready();
  return read_data();
};

void Dali::test_dacp(byte adr, byte value)
{
  byte dali = get_dali_address(DA_SHORT_BIT, adr, DA_MODE_DACP);    //create valid DALI address
  send_command(dali, value);                                        //Send DACP value 100 (valid for DACP is 0...254)
  wait_for_ready();                                                 //Wait until busy flag ist gone
  //To more stuff  
};

void Dali::test_cmd(byte adr, byte value)
{
  byte dali = get_dali_address(DA_SHORT_BIT, adr, DA_MODE_COMMAND);   //create valid DALI address
  send_command(dali, value);                                      //Send command OFF to device 0
  wait_for_ready();                                               //Wait until busy flag ist gone
  //To more stuff  
};

void Dali::test_broadcast(byte value)
{
  byte dali = get_dali_address(DA_GROUP_BIT, 0x3F, DA_MODE_COMMAND);   //Broadcast [ 0xFE + COMMAND BIT ]
  send_command(dali, value);                                      //Send command OFF to all devices
  wait_for_ready();                                               //Wait until busy flag ist gone
  //To more stuff  
};

void Dali::test_query(byte adr, byte value)
{
  byte result = 0x00;
  byte dali = get_dali_address(DA_SHORT_BIT, adr, DA_MODE_COMMAND);   //create valid DALI address
  read_query(dali, value);                                       //f.e. 0xA0 -> QueryActualLevel

  Serial.print("QUERY  >>> ");
  Serial.println(result, HEX);
};

