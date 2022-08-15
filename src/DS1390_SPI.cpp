/* ------------------------------------------------------------------------------------------- */
// DS1390 - Library for interfacing the DS1390 RTC using SPI
// Version: 1.4
// Author:  Renan R. Duarte
// E-mail:  duarte.renan@hotmail.com
// Date:    October 19, 2019
//
// Notes:   - A 200ms (min) delay is required after boot. It done inside the constructor
//      	- Epoch related functions assume year is higher than 2000.
//			- Century and Hundredths of Seconds registers are ignored in Epoch related functions
//      	- Works with DS1391 aswell.
//      	- Alarm-related functions not implemented yet
//
// Knwon bugs:  - In 12h format, the device do not change the AM/PM bit neither increments
//              the day of the week and day counters. Everything works in 24h mode
//
//              - Reading Hundredths of Seconds too often makes the device loose accuracy
//
// Changelog: 	v1.0 - 09/10/2019 - First release
//        		v1.1 - 10/10/2019 - Century bit is now used for validation
//        		v1.2 - 14/10/2019 - Bug fixes and Epoch timestamp related functions
//				v1.3 - 17/10/2019 - Oscillator Stop Flag used for validation
//				v1.4 - 19/10/2019 - Powerup delay occur only if necessary now
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

DS1390::DS1390(uint16_t PinCs)
{
  // Save CS pin bitmask
  _PinCs = PinCs;
}

/* ------------------------------------------------------------------------------------------- */
// Initializer
/* ------------------------------------------------------------------------------------------- */

void DS1390::begin ()
{
  // Set chip select pin as output
  pinMode (_PinCs, OUTPUT);

  // Deselect device (active low)
  digitalWrite (_PinCs, HIGH);

  // A 200ms powerup delay is mandatory for DS1391
  delay (200);
  
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

uint8_t DS1390::dec2bcd (uint8_t DecValue)
{
  return (((DecValue / 10) << 4) | (DecValue % 10));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        bcd2dec
// Description: Converts BCD numbers to decimal
// Arguments:   BCDValue - Value to be converted
// Returns:     Value in decimal format

uint8_t DS1390::bcd2dec (uint8_t BCDValue)
{
  return ((((BCDValue >> 4) & 0x0F) * 10) + (BCDValue & 0x0F));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        writeByte
// Description: Writes a byte to DS1390 memory
// Arguments:   Address - Register to be written
//              Data - Byte to be written
// Returns:     none

void DS1390::writeByte(uint8_t Address, uint8_t Data)
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

uint8_t DS1390::readByte (uint8_t Address)
{
  // Data byte
  uint8_t Data = 0;

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

// Name:        dateTimeToEpoch
// Description: Converts DS1390DateTime structure to Epoch timestamp - Ignores hundredths of sec.
// Arguments:   DateTime - DS1390DateTime structure with the data
//				      Timezone - Timezone info (-12 to +12, 0 = GMT) of DateTime
// Returns:     Epoch timestamp referred to Timezone

uint32_t DS1390::dateTimeToEpoch (DS1390DateTime &DateTime, int Timezone)
{
  // Seconds since 00:00:00 - Jan 1, 1970 GMT
  uint32_t Epoch = 0;

  // Counter
  uint16_t Counter = 0;

  // Epoch time starts in 1970 - Assumes current time is after 2000
  DateTime.Year += 30;

  // 12h mode
  if ((getTimeFormat() == DS1390_FORMAT_12H) && (DateTime.AmPm == DS1390_PM))
    DateTime.Hour += 12;
    
  // Correct value for given timezone
  if (Timezone != 0)
  {
    long Correction = (constrain(Timezone, -12, 12) * -3600);
    Epoch += Correction; 
  }
  
  // Seconds from 1970 until 1 jan 00:00:00 of the given year
  Epoch += DateTime.Year * (86400 * 365); // 31536000 seconds per year

  // Add extra days for leap years
  for (Counter = 0; Counter < DateTime.Year; Counter++)
  {
    if (LEAP_YEAR(Counter))
      Epoch += 86400;
  }

  // Add days for given year - Months start from 1
  for (Counter = 1; Counter < DateTime.Month; Counter++)
  {
    // February - Leap year
    if ((Counter == 2) && LEAP_YEAR(DateTime.Year))
      Epoch += (86400 * 29); // 29 days
    
    else
      Epoch += 86400 * _MonthDuration[Counter - 1];
  }

  // Add days for given month
  Epoch += (DateTime.Day - 1) * 86400; // 86400 seconds per day
  Epoch += DateTime.Hour * 3600;	// 3600 seconds per hour
  Epoch += DateTime.Minute * 60; // 60 seconds per minute
  Epoch += DateTime.Second;

  // Return seconds
  return Epoch;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        epochToDateTime
// Description: Converts Epoch timestamp to DS1390DateTime structure - Ignores hundredths of sec.
// Arguments:   Epoch - Epoch timestamp
//				      DateTime - DS1390DateTime structure to store the data
//				      Timezone - Timezone info (-12 to +12, 0 = GMT) of Epoch
// Returns:     None

void DS1390::epochToDateTime (uint32_t Epoch, DS1390DateTime &DateTime, int Timezone)
{
  // Variables
  uint32_t EpochTime = Epoch;
  uint8_t Year = 0;
  uint8_t Month = 0;
  uint8_t MonthLength = 0;
  uint32_t Days = 0;

  // Correct value for given timezone
  if (Timezone != 0)
    EpochTime += (Timezone * 3600);
  
  // Calculate seconds
  DateTime.Second = EpochTime % 60;

  // Get remaining time in minutes
  EpochTime /= 60;

  // Calculate minutes
  DateTime.Minute = EpochTime % 60;

  // Get remaining time in hours
  EpochTime /= 60;

  // Calculate hours
  DateTime.Hour = EpochTime % 24;

  // 12h format
  if (getTimeFormat() == DS1390_FORMAT_12H)
  {
    // 0 = 12AM
    if (DateTime.Hour == 0)
    {
      DateTime.Hour = 12;
      DateTime.AmPm = DS1390_AM;
    }

    // 12 = 12PM
    else if (DateTime.Hour == 12)
    {
      DateTime.Hour = 12;
      DateTime.AmPm = DS1390_PM;
    }

    // 1-11 = 1-11AM
    else if (DateTime.Hour < 12)
    {
      DateTime.AmPm = DS1390_AM;
    }

    // 13-23 = 1-11 PM
    else
    {
      DateTime.Hour -= 12;
      DateTime.AmPm = DS1390_PM;      
    }
  }

  // Get remaining time in days
  EpochTime /= 24;

  // Get weekday
  DateTime.Wday = ((EpochTime + 4) % 7) + 1;  // Sunday is day 1 

  // Calculate years since 1970
  while((unsigned)(Days += (LEAP_YEAR(Year) ? 366 : 365)) <= EpochTime) 
    Year++;

  // Get absolute value
  DateTime.Year = Year - 30;

  Days -= LEAP_YEAR(Year) ? 366 : 365;

  // Get remaining time in days since Jan 1 of the given year
  EpochTime -= Days;

  // Calculate month duration
  for (Month = 0; Month < 12; Month++) 
  {
    // February
    if (Month == 1) 
    { 
      if (LEAP_YEAR(Year)) 
        MonthLength = 29;
      
      else 
        MonthLength = 28;
    } 
    
    else 
      MonthLength = _MonthDuration [Month];
    
    if (EpochTime >= MonthLength) 
      EpochTime -= MonthLength;
    
    else 
      break;
  }

  // Calculate month
  DateTime.Month = Month + 1;

  // Calculate day
  DateTime.Day = EpochTime + 1; 
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getValidation
// Description: Gets the OSF bit from DS1390 memory
// Arguments:   None
// Returns:     false if memory content was recently lost or true otherwise

bool DS1390::getValidation ()
{
  // Return validation flag
  if (((readByte(DS1390_ADDR_READ_STS) & DS1390_MASK_OSF) >> 7) == 1)
    return false;
  else
    return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setValidation
// Description: Resets the OSF bit in DS1390 memory
// Arguments:   None
// Returns:     None

void DS1390::setValidation ()
{
  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_STS, readByte(DS1390_ADDR_READ_STS) & ~DS1390_MASK_OSF);
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getTimeFormat
// Description: Gets the current time format (12h/24h) from DS1390 memory
// Arguments:   None
// Returns:     DS1390_FORMAT_24H (logic 0) or DS1390_FORMAT_12H (logic 1)

uint8_t DS1390::getTimeFormat ()
{
  // Return format bit of Hours register
  return ((readByte (DS1390_ADDR_READ_HRS) & DS1390_MASK_FORMAT) >> 6);
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setTimeFormat
// Description: Sets the time format (12h/24h) on DS1390 memory
// Arguments:   DS1390_FORMAT_24H (logic 0) or DS1390_FORMAT_12H (logic 1)
// Returns:     false if new format is equal to current or true on completion

bool DS1390::setTimeFormat (uint8_t Format)
{
  // Current value stored on Hours register
  uint8_t HrsReg = readByte(DS1390_ADDR_READ_HRS);

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

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeAll
// Description: Gets all time related register values from DS1390 memory
// Arguments:   DateTime - DS1390DateTime structure to store the data
// Returns:     None

void DS1390::getDateTimeAll(DS1390DateTime &DateTime)
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
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeAll
// Description: Sets all time related register values in DS1390 memory
// Arguments:   DateTime - DS1390DateTime structure with the data to be written
// Returns:     None

void DS1390::setDateTimeAll(DS1390DateTime &DateTime)
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
  
  // Set validation bit
  setValidation ();  
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeHSeconds
// Description: Gets hundredths of seconds from DS1390 memory
// Arguments:   None
// Returns:     Hundredths of seconds

uint8_t DS1390::getDateTimeHSeconds ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_HSEC)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeHSeconds
// Description: Sets hundredths of seconds in DS1390 memory
// Arguments:   Hundredths of seconds (constrained between 0 and 99)
// Returns:     none

void DS1390::setDateTimeHSeconds (uint8_t Value)
{
  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_HSEC, dec2bcd(constrain(Value, 0, 99)));

  // Set validation bit
  setValidation ();
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeSeconds
// Description: Gets seconds from DS1390 memory
// Arguments:   None
// Returns:     Seconds

uint8_t DS1390::getDateTimeSeconds ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_SEC)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeSeconds
// Description: Sets seconds in DS1390 memory
// Arguments:   Seconds (constrained between 0 and 59)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeSeconds (uint8_t Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeSeconds())
    return false;

  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_SEC, dec2bcd(constrain(Value, 0, 59)));

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeMinutes
// Description: Gets minutes from DS1390 memory
// Arguments:   None
// Returns:     Minutes

uint8_t DS1390::getDateTimeMinutes ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_MIN)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeMinutes
// Description: Sets minutes in DS1390 memory
// Arguments:   Minutes (constrained between 0 and 59)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeMinutes (uint8_t Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeMinutes())
    return false;

  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_MIN, dec2bcd(constrain(Value, 0, 59)));

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeHours
// Description: Gets hours from DS1390 memory
// Arguments:   None
// Returns:     Hours

uint8_t DS1390::getDateTimeHours ()
{
  // Get current value
  uint8_t Buffer = readByte(DS1390_ADDR_READ_HRS);

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

bool DS1390::setDateTimeHours (uint8_t Value)
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
    uint8_t AmPm = (readByte(DS1390_ADDR_READ_HRS) & DS1390_MASK_AMPM) >> 5;
    Value = dec2bcd(constrain(Value, 1, 12)) | (AmPm << 5) | DS1390_MASK_FORMAT;
  }

  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_HRS, Value);

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeWday
// Description: Gets day of the week from DS1390 memory
// Arguments:   None
// Returns:     Day of the week

uint8_t DS1390::getDateTimeWday ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_WDAY)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeWday
// Description: Sets day of the week in DS1390 memory
// Arguments:   Day of the week (constrained between 1 and 7)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeWday (uint8_t Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeWday())
    return false;

  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_WDAY, dec2bcd(constrain(Value, 0, 7)));

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeDay
// Description: Gets day from DS1390 memory
// Arguments:   None
// Returns:     Day

uint8_t DS1390::getDateTimeDay ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_DAY)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeDay
// Description: Sets day in DS1390 memory
// Arguments:   Day (constrained between 1 and 31)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeDay (uint8_t Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeDay())
    return false;

  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_DAY, dec2bcd(constrain(Value, 0, 31)));

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeMonth
// Description: Gets month from DS1390 memory
// Arguments:   None
// Returns:     Month

uint8_t DS1390::getDateTimeMonth ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_MON) & 0x1F));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeMonth
// Description: Sets month in DS1390 memory
// Arguments:   Month (constrained between 1 and 12)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeMonth (uint8_t Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeMonth())
    return false;

  // Century flag
  uint8_t Century = ((readByte(DS1390_ADDR_READ_MON) & DS1390_MASK_CENTURY) >> 7);
  
  // Store Century info in Century bit of Month register
  Value = dec2bcd(constrain(Value, 1, 12)) | (Century << 7);  

  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_MON, Value);
  
  // Set validation bit
  setValidation ();  

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeYear
// Description: Gets year from DS1390 memory
// Arguments:   None
// Returns:     Year

uint8_t DS1390::getDateTimeYear ()
{
  // Return register value
  return (bcd2dec(readByte(DS1390_ADDR_READ_YRS)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeYear
// Description: Sets year in DS1390 memory
// Arguments:   Year (constrained between 0 and 99)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeYear (uint8_t Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeYear())
    return false;

  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_YRS, dec2bcd(constrain(Value, 0, 99)));

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeAmPm
// Description: Gets AM/PM flag from DS1390 memory
// Arguments:   None
// Returns:     DS1390_AM (logic 0) or DS1390_PM (logic 1) - Returns 0 if format is set to 24h

uint8_t DS1390::getDateTimeAmPm ()
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

bool DS1390::setDateTimeAmPm (uint8_t Value)
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
  uint8_t BufferHrs = dec2bcd(getDateTimeHours()) | (Value << 5) | DS1390_MASK_FORMAT;

  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_HRS, BufferHrs);

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeCentury
// Description: Gets century flag from DS1390 memory
// Arguments:   None
// Returns:     Century

uint8_t DS1390::getDateTimeCentury ()
{
  // Return century flag
  return ((readByte(DS1390_ADDR_READ_MON) & DS1390_MASK_CENTURY) >> 7);
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeCentury
// Description: Sets century flag in DS1390 memory
// Arguments:   Century (constrained between 0 and 1)
// Returns:     false if new value is equal to current or true on completion

bool DS1390::setDateTimeCentury (uint8_t Value)
{
  // Check if new value is equal to current
  if (Value == getDateTimeCentury())
    return false;

  // Get current month 
  uint8_t BufferMonth = getDateTimeMonth();

  // Constrain value within allowed limits and send to DS1390
  writeByte (DS1390_ADDR_WRITE_MON, (dec2bcd(getDateTimeMonth()) | (constrain(Value, 0, 1) << 7)));
  
  // Set validation bit
  setValidation ();  
  
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

uint8_t DS1390::getTrickleChargerMode ()
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

bool DS1390::setTrickleChargerMode (uint8_t Mode)
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

  // Set validation bit
  setValidation ();

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getDateTimeEpoch
// Description: Gets all time related register values from DS1390 memory in Epoch format
// Arguments:   Timezone - Timezone info (-12 to +12, 0 = GMT) to calculate Epoch
// Returns:     Epoch timestamp

uint32_t DS1390::getDateTimeEpoch (int Timezone)
{
	// Get date and time from DS1390 memory
	getDateTimeAll(_DateTimeBuffer);
	
	// Convert to Epoch format and return result
	return dateTimeToEpoch (_DateTimeBuffer, Timezone);
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setDateTimeEpoch
// Description: Sets all time related register values in DS1390 memory from an Epoch timestamp
// Arguments:   Epoch - Epoch timestamp
//				      Timezone - Timezone info (-12 to +12, 0 = GMT) of Epoch
// Returns:     None

void DS1390::setDateTimeEpoch(uint32_t Epoch, int Timezone)
{
	// Convert Epoch to DateTime
	epochToDateTime (Epoch, _DateTimeBuffer, Timezone);
	
	// Write data to DS1390
	setDateTimeAll(_DateTimeBuffer);
}

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
