/* ------------------------------------------------------------------------------------------- */
// DS1390 - Library for interfacing the DS1390 RTC using SPI
// Version: 1.0
// Author:  Renan R. Duarte
// E-mail:  duarte.renan@hotmail.com
// Date:    October 9, 2019
//
// Notes:   A 200ms (min) delay is required after boot to read/write device memory 
//          Works with DS1391 aswell.
//			Alarm-related functions not implemented yet
//          
// Knwon bugs:  - In 12h format, the device do not change the AM/PM bit neither increments
//              the day of the week and day counters. Everything works in 24h mode
//
//              - Reading Hundredths of Seconds too often makes the device loose accuracy
//
// Released into the public domain
/* ------------------------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------------------------- */
// Includes
/* ------------------------------------------------------------------------------------------- */

#include "Arduino.h"
#include "SPI.h"
#include "DS1390_SPI.h"

/* ------------------------------------------------------------------------------------------- */
// Constructor
/* ------------------------------------------------------------------------------------------- */

DS1390::DS1390(unsigned int PinCs)
{
  // Save CS pin bitmask
  _PinCs = PinCs;
    
  // Set chip select pin as output
  pinMode (_PinCs, OUTPUT);

  // Deselect device (active low)
  digitalWrite (_PinCs, HIGH);

  // Start SPI bus
  SPI.begin ();
}

/* ------------------------------------------------------------------------------------------- */
// Functions definitions
/* ------------------------------------------------------------------------------------------- */

// Name:        dec2bcd
// Description: Converts decimal to BCD numbers
// Arguments:   DecValue - Value to be converted
// Returns:     Value in BCD format

unsigned char DS1390::dec2bcd (unsigned char DecValue) 
{
  return (((DecValue / 10) << 4) | (DecValue % 10));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        bcd2dec
// Description: Converts BCD numbers to decimal 
// Arguments:   BCDValue - Value to be converted
// Returns:     Value in decimal format

unsigned char DS1390::bcd2dec (unsigned char BCDValue) 
{
  return ((((BCDValue >> 4) & 0x0F) * 10) + (BCDValue & 0x0F));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        writeByte
// Description: Writes a byte to DS1390 memory
// Arguments:   Address - Register to be written
//              Data - Byte to be written
// Returns:     none

void DS1390::writeByte(unsigned char Address, unsigned char Data)
{
  // Configure SPI transaction
  SPI.beginTransaction(SPISettings(DS1390_SPI_CLOCK, MSBFIRST, SPI_MODE1));

  // Select device (active low)
  digitalWrite (_PinCs, LOW); 

  // Send address byte
  SPI.transfer (Address);

  // Send data byte
  SPI.transfer (Data);
  
  // Deselect device (active low)
  digitalWrite (_PinCs, HIGH);

  // End SPI transaction
  SPI.endTransaction ();    
}

/* ------------------------------------------------------------------------------------------- */

// Name:        readByte
// Description: Reads a byte from DS1390 memory
// Arguments:   Address - Register to be read
// Returns:     Byte read

unsigned char DS1390::readByte (unsigned char Address)
{
  // Data byte
  unsigned char Data = 0;

  // Configure SPI transaction  
  SPI.beginTransaction(SPISettings(DS1390_SPI_CLOCK, MSBFIRST, SPI_MODE1));

  // Select device (active low)  
  digitalWrite (_PinCs, LOW);

  // Send address byte
  SPI.transfer (Address); 

  // Read data byte (0xFF = dummy)
  Data = SPI.transfer (0xFF);
  
  // Deselect device (active low)
  digitalWrite (_PinCs, HIGH);

  // End SPI transaction
  SPI.endTransaction();   

  // Return read byte
  return Data;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getTimeFormat
// Description: Gets the current time format (12h/24h) from DS1390 memory
// Arguments:   None
// Returns:     DS1390_FORMAT_24H (logic 0) or DS1390_FORMAT_12H (logic 1)

unsigned char DS1390::getTimeFormat ()
{
  // Return format bit of Hours register
  return ((readByte (DS1390_ADDR_READ_HRS) & DS1390_MASK_FORMAT) >> 5);
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setTimeFormat
// Description: Sets the time format (12h/24h) on DS1390 memory
// Arguments:   DS1390_FORMAT_24H (logic 0) or DS1390_FORMAT_12H (logic 1)
// Returns:     false if new format is equal to current or true on completion

bool DS1390::setTimeFormat (unsigned char Format)
{
  // Current value stored on Hours register
  unsigned char HrsReg = readByte(DS1390_ADDR_READ_HRS);

  // Check if new format is equal to current
  if (Format == ((HrsReg & DS1390_MASK_FORMAT) >> 6))
    return false;

  // Check if new format is invalid
  else if ((Format != DS1390_FORMAT_24H) && (Format != DS1390_FORMAT_12H))
    return false;
  
  // Prepare new value of Hours register
  if (Format == DS1390_FORMAT_24H)
    HrsReg &= ~DS1390_MASK_FORMAT;
  else
    HrsReg |= DS1390_MASK_FORMAT;

  // Send new Hours register
  writeByte (DS1390_ADDR_WRITE_HRS, HrsReg);

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeAll
// Description: Gets all time related register values from DS1390 memory
// Arguments:   DateTime - DS1390DateTime structure to store the data
// Returns:     false on error or true on completion

// TODO valores de retorno
bool DS1390::getDateTimeAll(DS1390DateTime &DateTime)
{
  // Struck to store raw read data
  DS1390DateTime Buffer;
  
  // Configure SPI transaction  
  SPI.beginTransaction(SPISettings(DS1390_SPI_CLOCK, MSBFIRST, SPI_MODE1));

  // Select device (active low)  
  digitalWrite (_PinCs, LOW);

  // Send first address byte
  SPI.transfer (DS1390_ADDR_READ_HSEC); 

  // Read data bytes sequentially (0xFF = dummy)
  Buffer.Hsecond = SPI.transfer (0xFF);
  Buffer.Second = SPI.transfer (0xFF);
  Buffer.Minute = SPI.transfer (0xFF);
  Buffer.Hour = SPI.transfer (0xFF);
  Buffer.Wday = SPI.transfer (0xFF);
  Buffer.Day = SPI.transfer (0xFF);
  Buffer.Month = SPI.transfer (0xFF);
  Buffer.Year = SPI.transfer (0xFF);  

  // Deselect device (active low)
  digitalWrite (_PinCs, HIGH);

  // End SPI transaction
  SPI.endTransaction();       

  // Convert hours - 24h format
  if (((Buffer.Hour & DS1390_MASK_FORMAT) >> 5) ==  DS1390_FORMAT_24H)
  {
    DateTime.Hour = bcd2dec(Buffer.Hour & 0x3F);
    DateTime.AmPm = 0;
  }
  
  // Convert hours and AM/PM flag - 12h format
  else
  {
    DateTime.Hour = bcd2dec(Buffer.Hour & 0x1F); 
    DateTime.AmPm = ((Buffer.Hour & DS1390_MASK_AMPM) >> 5);
  }

  // Convert remaining fields
  DateTime.Hsecond = bcd2dec(Buffer.Hsecond);
  DateTime.Second = bcd2dec(Buffer.Second);
  DateTime.Minute = bcd2dec(Buffer.Minute);
  DateTime.Wday = bcd2dec(Buffer.Wday);
  DateTime.Day = bcd2dec(Buffer.Day);
  DateTime.Month = bcd2dec(Buffer.Month & 0x1F); // Ignore Century bit
  DateTime.Year = bcd2dec(Buffer.Year);
  DateTime.Century = ((Buffer.Month & DS1390_MASK_CENTURY) >> 7);

  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeAll
// Description: Sets all time related register values in DS1390 memory
// Arguments:   DateTime - DS1390DateTime structure with the data to be written
// Returns:     false on error or true on completion

bool DS1390::setDateTimeAll(DS1390DateTime &DateTime)
{
  // Struck to store raw data to be written
  DS1390DateTime Buffer;  
  
  // Prepares buffer - Constrain values within allowed limits
  Buffer.Hsecond = dec2bcd(constrain(DateTime.Hsecond, 0, 99));
  Buffer.Second = dec2bcd(constrain(DateTime.Second, 0, 59));
  Buffer.Minute = dec2bcd(constrain(DateTime.Minute, 0, 59));
  Buffer.Wday = dec2bcd(constrain(DateTime.Wday, 1, 7));
  Buffer.Day = dec2bcd(constrain(DateTime.Day, 1, 31));
  Buffer.Year = dec2bcd(constrain(DateTime.Year, 0, 99));  

  // 24h mode
  if (getTimeFormat() == DS1390_FORMAT_24H)
    Buffer.Hour = dec2bcd(constrain(DateTime.Hour, 0, 23));  

  // 12h mode - Store AmPm info in AmPm bit of Hour register and make sure format bit is 1
  else
    Buffer.Hour = dec2bcd(constrain(DateTime.Hour, 1, 12)) | (DateTime.AmPm << 5) | DS1390_MASK_FORMAT;  

  // Store Century info in Century bit of Month register
  Buffer.Month = dec2bcd(constrain(DateTime.Month, 1, 12)) | (DateTime.Century << 7);  

  // Configure SPI transaction  
  SPI.beginTransaction(SPISettings(DS1390_SPI_CLOCK, MSBFIRST, SPI_MODE1));

  // Select device (active low)  
  digitalWrite (_PinCs, LOW);

  // Send first address byte
  SPI.transfer (DS1390_ADDR_WRITE_HSEC); 

  // Write data bytes sequentially (0xFF = dummy)
  SPI.transfer (Buffer.Hsecond);
  SPI.transfer (Buffer.Second);
  SPI.transfer (Buffer.Minute);
  SPI.transfer (Buffer.Hour);
  SPI.transfer (Buffer.Wday);
  SPI.transfer (Buffer.Day);
  SPI.transfer (Buffer.Month);
  SPI.transfer (Buffer.Year);  

  // Deselect device (active low)
  digitalWrite (_PinCs, HIGH);

  // End SPI transaction
  SPI.endTransaction(); 

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeHSeconds
// Description: Gets hundredths of seconds from DS1390 memory
// Arguments:   None
// Returns:     Hundredths of seconds

unsigned char DS1390::getDateTimeHSeconds ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_HSEC)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeHSeconds
// Description: Sets hundredths of seconds in DS1390 memory
// Arguments:   Hundredths of seconds (constrained between 0 and 99)
// Returns:     none

void DS1390::setDateTimeHSeconds (unsigned char Value)
{
  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_HSEC, dec2bcd(constrain(Value, 0, 99)));  
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeSeconds
// Description: Gets seconds from DS1390 memory
// Arguments:   None
// Returns:     Seconds

unsigned char DS1390::getDateTimeSeconds ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_SEC)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeSeconds
// Description: Sets seconds in DS1390 memory
// Arguments:   Seconds (constrained between 0 and 59)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeSeconds (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeSeconds())
    return false;
      
  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_SEC, dec2bcd(constrain(Value, 0, 59)));  

  // Success
  return true;  
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeMinutes
// Description: Gets minutes from DS1390 memory
// Arguments:   None
// Returns:     Minutes

unsigned char DS1390::getDateTimeMinutes ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_MIN)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeMinutes
// Description: Sets minutes in DS1390 memory
// Arguments:   Minutes (constrained between 0 and 59)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeMinutes (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeMinutes())
    return false;
      
  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_MIN, dec2bcd(constrain(Value, 0, 59)));

  // Success
  return true;    
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeHours
// Description: Gets hours from DS1390 memory
// Arguments:   None
// Returns:     Hours

unsigned char DS1390::getDateTimeHours ()
{
  // Get current value
  unsigned char Buffer = readByte(DS1390_ADDR_READ_HRS);
  
  // Convert hours - 24h format
  if (((Buffer & DS1390_MASK_FORMAT) >> 5) ==  DS1390_FORMAT_24H)
    Buffer = bcd2dec(Buffer & 0x3F);

  // Convert hours - 12h format
  else
    Buffer = bcd2dec(Buffer & 0x1F); 

  // Return value
  return (Buffer);
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeHours
// Description: Sets hours in DS1390 memory
// Arguments:   Hours (constrained between 0 and 23 in 24h format or 1 and 12 in 12h format)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeHours (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeHours())
    return false;

  // 24h mode
  if (getTimeFormat() == DS1390_FORMAT_24H)
    Value = dec2bcd(constrain(Value, 0, 23));  

  // 12h mode - Store AmPm info in AmPm bit of Hour register and make sure format bit is 1
  else
  {
    unsigned char AmPm = (readByte(DS1390_ADDR_READ_HRS) & DS1390_MASK_AMPM) >> 5;    
    Value = dec2bcd(constrain(Value, 1, 12)) | (AmPm << 5) | DS1390_MASK_FORMAT;  
  }
      
  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_HRS, Value);  

  // Success
  return true;  
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeWday
// Description: Gets day of the week from DS1390 memory
// Arguments:   None
// Returns:     Day of the week

unsigned char DS1390::getDateTimeWday ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_WDAY)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeWday
// Description: Sets day of the week in DS1390 memory
// Arguments:   Day of the week (constrained between 1 and 7)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeWday (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeWday())
    return false;
      
  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_WDAY, dec2bcd(constrain(Value, 0, 7))); 

  // Success
  return true;   
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeDay
// Description: Gets day from DS1390 memory
// Arguments:   None
// Returns:     Day

unsigned char DS1390::getDateTimeDay ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_DAY)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeDay
// Description: Sets day in DS1390 memory
// Arguments:   Day (constrained between 1 and 31)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeDay (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeDay())
    return false;
      
  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_DAY, dec2bcd(constrain(Value, 0, 31)));

  // Success
  return true;    
}     

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeMonth
// Description: Gets month from DS1390 memory
// Arguments:   None
// Returns:     Month

unsigned char DS1390::getDateTimeMonth ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_MON) & 0x1F));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeMonth
// Description: Sets month in DS1390 memory
// Arguments:   Month (constrained between 1 and 12)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeMonth (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeMonth())
    return false;

  // Century flag
  unsigned char Century = ((readByte(DS1390_ADDR_READ_MON) & DS1390_MASK_CENTURY) >> 7);
  
  // Store Century info in Century bit of Month register
  Value = dec2bcd(constrain(Value, 1, 12)) | (Century << 7);  

  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_MON, Value);  

  // Success
  return true;  
}   

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeYear
// Description: Gets year from DS1390 memory
// Arguments:   None
// Returns:     Year

unsigned char DS1390::getDateTimeYear ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_YRS)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeYear
// Description: Sets year in DS1390 memory
// Arguments:   Year (constrained between 0 and 99)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeYear (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeYear())
    return false;
      
  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_YRS, dec2bcd(constrain(Value, 0, 99)));  

  // Success
  return true;  
}  

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeAmPm
// Description: Gets AM/PM flag from DS1390 memory
// Arguments:   None
// Returns:     DS1390_AM (logic 0) or DS1390_PM (logic 1) - Returns 0 if format is set to 24h

unsigned char DS1390::getDateTimeAmPm ()
{
  // 24h mode
  if (getTimeFormat() == DS1390_FORMAT_24H)
    return 0;  

  // 12h mode
  return (readByte(DS1390_ADDR_READ_HRS) & DS1390_MASK_AMPM) >> 5;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeAmPm
// Description: Sets AM/PM flag in DS1390 memory
// Arguments:   DS1390_AM (logic 0) or DS1390_PM (logic 1)
// Returns:     false if new value is equal to current or format is set to 24h or true on completion

bool DS1390::setDateTimeAmPm (unsigned char Value)
{ 
  // Check if device is in 24h mode
  if (getTimeFormat() == DS1390_FORMAT_24H)
    return false;  
      
  // Check if new value is equal to current
  else if (Value == getDateTimeAmPm())
    return false;

  // Check if new value is invalid
  else if ((Value != DS1390_AM) && (Value != DS1390_PM))
    return false;

  // 12h mode - Store value in AmPm bit of Hour register and make sure format bit is 1
  unsigned char BufferHrs = dec2bcd(getDateTimeHours()) | (Value << 5) | DS1390_MASK_FORMAT;

  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_HRS, BufferHrs);     

  // Success
  return true;  
}  

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeCentury
// Description: Gets century flag from DS1390 memory
// Arguments:   None
// Returns:     Century

unsigned char DS1390::getDateTimeCentury ()
{
  // Return century flag
  return ((readByte(DS1390_ADDR_READ_MON) & DS1390_MASK_CENTURY) >> 7);
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeCentury
// Description: Sets century flag in DS1390 memory
// Arguments:   Century (constrained between 0 and 1)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeCentury (unsigned char Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeCentury())
    return false;

  // Get current month 
  unsigned char BufferMonth = getDateTimeMonth();

  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_MON, (dec2bcd(getDateTimeMonth()) | (constrain(Value, 0, 1) << 7)));
  
  // Success
  return true;  
}     

/* ------------------------------------------------------------------------------------------- */

// Name:        getTrickleChargerMode
// Description: Gets the current trickle charger settings
// Arguments:   none
// Returns:     DS1390_TCH_DISABLE  - Disabled
//              DS1390_TCH_250_NO_D - 250 Ohms without diode
//              DS1390_TCH_250_D    - 250 Ohms with diode
//              DS1390_TCH_2K_NO_D  - 2 kOhms without diode
//              DS1390_TCH_2K_D     - 2 kOhms with diode
//              DS1390_TCH_4K_NO_D  - 4 kOhms without diode
//              DS1390_TCH_4K_D     - 4 kOhms with diode

unsigned char DS1390::getTrickleChargerMode ()
{
  // Return Tch register value
  return (readByte (DS1390_ADDR_READ_TCH));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setTrickleChargerMode
// Description: Gets the current trickle charger settings
// Arguments:   DS1390_TCH_DISABLE  - Disabled
//              DS1390_TCH_250_NO_D - 250 Ohms without diode
//              DS1390_TCH_250_D    - 250 Ohms with diode
//              DS1390_TCH_2K_NO_D  - 2 kOhms without diode
//              DS1390_TCH_2K_D     - 2 kOhms with diode
//              DS1390_TCH_4K_NO_D  - 4 kOhms without diode
//              DS1390_TCH_4K_D     - 4 kOhms with diode
// Returns:     false if new mode is equal to current or true on completion

bool DS1390::setTrickleChargerMode (unsigned char Mode)
{
  // Check if new mode is equal to current
  if (Mode == getTrickleChargerMode())
    return false;

  // Check if value is valid
  else if ((Mode != DS1390_TCH_DISABLE) && (Mode != DS1390_TCH_250_NO_D) && (Mode != DS1390_TCH_250_D)
          && (Mode != DS1390_TCH_2K_NO_D) && (Mode != DS1390_TCH_2K_D) && (Mode != DS1390_TCH_4K_NO_D)
          && (Mode != DS1390_TCH_4K_D))
    return false;          

  // Send new configuration byte
  writeByte (DS1390_ADDR_WRITE_TCH, Mode);
  
  // Success
  return true;  
}

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
