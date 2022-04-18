/*Sample code for IEC62386 for send command/DACP and get QUERY on an ESP32*/

#include <Arduino.h>
#include <Wire.h>
#include "iec_62386.h"    //List of command bytes for IEC62386-102:2009 standard

/*!ATTENTION!*/
#define NOP() asm volatile ("nop") //Workaround for delayMicrosecondeExt function


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
    Wire.begin(21,22);
    Serial.begin(115200);
}

//delayMicroseconds on ESP32 faulty, workaround 
void IRAM_ATTR delayMicrosecondsExt(uint32_t us)
{
    uint32_t m = micros(); 
    if(us)
    {    
        while((uint32_t)(micros() - m) < us) 
        {
            NOP();
        }
    }
}

//Set I2C register to read from
void SetI2CReadRegister(uint8_t reg)
{
    Wire.beginTransmission(LW14_I2C);
    Wire.write(reg);
    Wire.endTransmission();
}

//Clear some flags by reading Command Register
void clear_status()
{
    SetI2CReadRegister(I2C_REG_COMMAND);    //Set I2C register to read from
    Wire.requestFrom(LW14_I2C, 3);          //Read N-Bytes from I2C register
    Wire.read();
}

//Generate DALI short address, DALI address format: 0 AAA AAA S
uint8_t GetShortAddress(uint8_t dali_adr, uint8_t mode)
{
    return ((DA_SHORT_BIT | ((dali_adr & DA_MAX_SHORT) << 1)) | mode);
}

//Generate DALI group address, DALI address format: 0 AAA AAA S
uint8_t GetGroupAddress(uint8_t dali_adr, uint8_t mode)
{
    return ((DA_GROUP_BIT | ((dali_adr & DA_MAX_GROUP) << 1)) | mode);
}

//Generate DALI broadcast address, DALI address format: 0 AAA AAA S
uint8_t GetBroadcastAddress(uint8_t mode)
{
    return ((DA_GROUP_BIT | (0x3F << 1)) | mode);
}

//Wait until DALI bus is ready to send next data. DALI is slow!
void WaitForReady()
{
    uint8_t result = 0x00;

    while (true)
    {
        SetI2CReadRegister(I2C_REG_STATUS);

        Wire.requestFrom(LW14_I2C, 1);
        result = Wire.read();

        if ((result & STATUS_BUSY) == 0x00) //exit if busyflag is gone
            break;

        delayMicrosecondsExt(DA_TE);
    }
}

//Wait for query result from slave device
uint8_t WaitForReply()
{
    uint8_t result = 0x00;

    while (true)
    {
        SetI2CReadRegister(I2C_REG_STATUS);

        Wire.requestFrom(LW14_I2C, 1, false);
        result = Wire.read();

        //Continue if bus is not ready
        if((result & (STATUS_BUSY)) == STATUS_BUSY)
        {
            delayMicrosecondsExt(DA_TE);
            continue;
        }

        if ((result & (STATUS_VALID | STATUS_1BYTE)) == (STATUS_VALID | STATUS_1BYTE))
            return result;
        else if (result == 0x00) //No device
            return 0;

        delayMicrosecondsExt(DA_TE);
    }

    return result;
}

//Send 8bit command into the DALI bus
void SendCommand(uint8_t dali_adr, uint8_t value)
{
    Wire.beginTransmission(LW14_I2C);
    Wire.write(I2C_REG_COMMAND); //Register: command
    Wire.write((uint8_t)dali_adr);            //DALI address
    Wire.write((uint8_t)value);  //Value
    Wire.endTransmission();
}


//Send value to all DALI devices (for special commands like 'TERMINATE', 'DTR', etc)
void SendValue(uint8_t byte1, uint8_t byte2)
{
    Wire.beginTransmission(LW14_I2C);
    Wire.write(I2C_REG_COMMAND);        //Register: command
    Wire.write((uint8_t)byte1);         //DALI address
    Wire.write((uint8_t)byte2);         //Value
    Wire.endTransmission();
}

//Store value into DTR1 or DTR2
void SendToDTR(uint8_t dtr, uint8_t value)
{
    Wire.beginTransmission(LW14_I2C);
    Wire.write(I2C_REG_COMMAND);        //Register: command
    Wire.write((uint8_t) dtr);          // DTR, DTR1 or DTR2
    Wire.write((uint8_t) value);        //Value
    Wire.endTransmission();
}

//Get value from QUERY command by DALI slave address
uint8_t GetQuery(uint8_t dali_adr, uint8_t cmd)
{
    WaitForReady();                     //Wait for bus ready
    SendCommand(dali_adr, cmd);         //Send query command
    uint8_t res = WaitForReply();       //Wait for data

    //No one resonse, wrong DALI address
    if(res == 0x00)
        return res;

    //Set register to read from
    Wire.beginTransmission(LW14_I2C);
    Wire.write(I2C_REG_COMMAND);
    Wire.endTransmission();

    //Read from register
    Wire.requestFrom((uint8_t)LW14_I2C, (uint8_t)1);
    return Wire.read();
}

void loop() 
{
    uint8_t query = 0;
    uint8_t adr = 0;
    
    //Demo "blink" slave devices
    /*  
    GetBroadcastAddress(DA_MODE_COMMAND);   //Set address as braodcast in COMMAND mode
    
    WaitForReady();                         //Wait for Bus ready
    SendCommand(adr, DA_MAX);               //Send MAX command to device
    delay(1000);                        
    
    WaitForReady();                         //Wait for Bus ready
    SendCommand(adr, DA_MIN);               //Send MIN command to device
    delay(1000);
    */
    
    //Demo get query "ACTUAL Level"
    adr = GetShortAddress(15, DA_MODE_DACP);             //Set device address wit DACP mode
    SendCommand(adr, 132);                               //Send a DACP value to the device
    WaitForReady();                                     //Wait for Bus ready
    adr = GetShortAddress(15, DA_MODE_COMMAND);          //Change address from DACP mode to COMMAND mode
    query = GetQuery(adr, DA_QUERY_ACTUAL_LEVEL);       //Get Actual level from device
    Serial.print("Query: ");
    Serial.println(query, DEC);

    delay(2000);
  
    adr = GetShortAddress(15, DA_MODE_DACP);             //Set device address wit DACP mode
    SendCommand(adr, 66);                               //Send a DACP value to the device
    WaitForReady();                                     //Wait for Bus ready
    adr = GetShortAddress(15, DA_MODE_COMMAND);          //Change address from DACP mode to COMMAND mode
    query = GetQuery(adr, DA_QUERY_ACTUAL_LEVEL);       //Get Actual level from device
    Serial.print("Query: ");
    Serial.println(query, DEC);

    delay(2000);
    
}
