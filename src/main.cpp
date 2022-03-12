#include <Arduino.h>

/*
 * Arduino Sample for LED-Warrior14
 */

#include <Wire.h>

//device register
#define I2C_REG_STATUS          0x00  //read only, 1 byte
#define I2C_REG_COMMAND         0x01  //read/write, 2 bytes
#define I2C_REG_CONFIG          0x02  //write only, 1 byte
#define I2C_REG_SIGNATURE       0xF0  //read only, 6 bytes
#define I2C_REG_ADDRESS         0xFE  //write only, 2 bytes

//basic DALI commands
#define DALI_OFF                0x00
#define DALI_UP                 0x01
#define DALI_DOWN               0x02
#define DALI_STEP_UP            0x03
#define DALI_STEP_DOWN          0x04
#define DALI_MAX                0x05
#define DALI_MIN                0x06
#define DALI_STEP_DOWN_OFF      0x07
#define DALI_ON_STEP_UP         0x08
#define DALI_SCENE_X            0x10  //valid: 0x10 - 0x1F

//status bits
#define STATUS_1BYTE            0x01
#define STATUS_2BYTE            0x02
#define STATUS_TIMEFRAME        0x04
#define STATUS_VALID            0x08
#define STATUS_FRAME_ERROR      0x10
#define STATUS_OVERRUN          0x20
#define STATUS_BUSY             0x40  //0: ready, 1: busy
#define STATUS_BUS_FAULT        0x80  //0: OK, 1: bus fault

//device constants
#define I2C_ADDRESS             0x23  //default I2C address for LW14

#define DALI_MODE_CMD           0x01
#define DALI_MODE_DACP          0x00
#define DALI_SHORT              0x00
#define DALI_GROUP              0x80
#define DALI_MAX_SHORT          0x3F  //63 devices
#define DALI_MAX_GROUP          0x0F  //15 groups
#define DALI_MAX_SCENE          0x0F  //15 scenes


void setup() {
   Wire.begin(); 
   Serial.begin(9600);

}

//Create valid DALI address. (Please take a look into the datasheet or DALI DIN spec)
//Address pattern: S AAA AAA Y
//S for group bit
//AAA AAA (adr) for DALI device 0...63 or group 0...15
//Y for command or DACP mode
byte get_dali_address(byte s, byte adr, byte y)
{
  return (adr << 1) | s | y;
}

//Wait until DALI bus is ready to send next data. DALI is slow!
byte wait_for_ready()
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
}

//Read 1 byte from command register of LW14
byte read_data()
{
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(I2C_REG_COMMAND);
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADDRESS, (uint8_t)1);
  return Wire.read();
}

//Send value to DALI device
void send_command(int a, int b)
{ 
  Wire.beginTransmission(I2C_ADDRESS);  //start transmitting to I2C device X
  Wire.write(I2C_REG_COMMAND);          //Set I2C register for commands
  Wire.write((byte) a);                 //Data byte 1 (f.e. DALI address)
  Wire.write((byte) b);                 //Data byte 2 (f.e. DALI value)
  Wire.endTransmission();               // stop transmitting
}

//Get a query value from dali device
byte read_query(int a, int b)
{
  send_command(a, b);
  wait_for_ready();
  return read_data();
}

//Test function to send DACP value (direct dimm) to device 0...63
void test_dacp(byte adr, byte value)
{
  byte dali = get_dali_address(DALI_SHORT, adr, DALI_MODE_DACP);    //create valid DALI address
  send_command(dali, value);                                        //Send DACP value 100 (valid for DACP is 0...254)
  wait_for_ready();                                                 //Wait until busy flag ist gone
  //To more stuff  
}

//Send DALI command OFF to device 0...63
void test_cmd(byte adr, byte value)
{
  byte dali = get_dali_address(DALI_SHORT, adr, DALI_MODE_CMD);   //create valid DALI address
  send_command(dali, value);                                      //Send command OFF to device 0
  wait_for_ready();                                               //Wait until busy flag ist gone
  //To more stuff  
}

//Send DALI command OFF to all devices (broadcast)
void test_broadcast(byte value)
{
  byte dali = get_dali_address(DALI_GROUP, 0x3F, DALI_MODE_CMD);   //Broadcast [ 0xFE + COMMAND BIT ]
  send_command(dali, value);                                      //Send command OFF to all devices
  wait_for_ready();                                               //Wait until busy flag ist gone
  //To more stuff  
}

//Get ActualLevel from DALI device 0...63
void test_query(byte adr, byte value)
{
  byte result = 0x00;
  byte dali = get_dali_address(DALI_SHORT, adr, DALI_MODE_CMD);   //create valid DALI address
  read_query(dali, value);                                       //f.e. 0xA0 -> QueryActualLevel

  Serial.print("QUERY  >>> ");
  Serial.println(result, HEX);
}

void loop() 
{
  //test_cmd();
  test_broadcast(DALI_MIN);
  delay(1000);
  test_broadcast(DALI_MAX);
  delay(1000);
}
