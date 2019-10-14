/* ------------------------------------------------------------------------------------------- */
// DS1390 - Library for interfacing the DS1390 RTC using SPI
// Version: 1.2
// Author:  Renan R. Duarte
// E-mail:  duarte.renan@hotmail.com
// Date:    October 14, 2019
//
// Notes:   - This library uses the Century bit of the Month register as a way to check if the
//			memory contents are valid. When date or time registers are written, this bit is set
// 			to 1. This bit is used to check if the device memory was lost since last time the
//			registers were written (this bit is reseted to 0 when Vcc and Vbackup are lost).
//			- As a consequence, this library does not support years higher than 99.
//			- A 200ms (min) delay is required after boot to read/write device memory.
//          - Works with DS1391 aswell.
//			- Alarm-related functions not implemented yet
//
// Knwon bugs:  - In 12h format, the device do not change the AM/PM bit neither increments
//              the day of the week and day counters. Everything works in 24h mode
//
//              - Reading Hundredths of Seconds too often makes the device loose accuracy
//
// Changelog:	v1.0 - 09/10/2019 - First release
//				v1.1 - 10/10/2019 - Century bit is now used for validation
//				v1.2 - 14/10/2019 - Bug fixes and Unix timestamp related functions
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

// Name:        getValidation
// Description: Gets the validation bit (century) from DS1390 memory
// Arguments:   None
// Returns:     false if memory content was recently lost or true otherwise

bool DS1390::getValidation ()
{
  // Return validation flag
  if (((readByte(DS1390_ADDR_READ_MON) & DS1390_MASK_CENTURY) >> 7) == 1)
    return true;
  else
    return false;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        setValidation
// Description: Sets the validation bit (century) in DS1390 memory
// Arguments:   Validation bit value - 0 or 1
// Returns:     none

void DS1390::setValidation (bool Value)
{
  // Send value to DS1390
  writeByte (DS1390_ADDR_WRITE_MON, (dec2bcd(getDateTimeMonth()) | (Value << 7)));
}

/* ------------------------------------------------------------------------------------------- */

// Name:        getTimeFormat
// Description: Gets the current time format (12h/24h) from DS1390 memory
// Arguments:   None
// Returns:     DS1390_FORMAT_24H (logic 0) or DS1390_FORMAT_12H (logic 1)

unsigned char DS1390::getTimeFormat ()
{
  // Return format bit of Hours register
  return ((readByte (DS1390_ADDR_READ_HRS) & DS1390_MASK_FORMAT) >> 6);
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

  // Set validation bit
  setValidation (true);

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

  // Set validation bit
  Buffer.Month = dec2bcd(constrain(DateTime.Month, 1, 12)) | DS1390_MASK_CENTURY;

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

  // Set validation bit
  setValidation (true);
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

  // Set validation bit
  setValidation (true);

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

  // Set validation bit
  setValidation (true);

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

  // Set validation bit
  setValidation (true);

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

  // Set validation bit
  setValidation (true);

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

  // Set validation bit
  setValidation (true);

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

  // Constrain value within allowed limits, set validation bit and send to DS1390
  Value = dec2bcd(constrain(Value, 1, 12)) | DS1390_MASK_CENTURY;

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

  // Set validation bit
  setValidation (true);

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

  // Set validation bit
  setValidation (true);

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

  // Set validation bit
  setValidation (true);

  // Success
  return true;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        DateTimeToUnix
// Description: Converts DS1390DateTime structure to Unix timestamp - Ignores hundredths of sec.
// Arguments:   DateTime - DS1390DateTime structure with the data
//				      Timezone - Timezone info (-12 to +12, 0 = GMT) of DateTime
// Returns:     Unix timestamp referred to Timezone

unsigned long DS1390::DateTimeToUnix (DS1390DateTime &DateTime, int Timezone)
{
  // Seconds since 00:00:00 - Jan 1, 1970 GMT
  double Unix = 0;

  // Counter
  unsigned int Counter = 0;

  // Unix time starts in 1970 - Assumes current time is after 2000
  DateTime.Year += 30;

  // 12h mode
  if ((getTimeFormat() == DS1390_FORMAT_12H) && (DateTime.AmPm == DS1390_PM))
    DateTime.Hour += 12;
    
  // Correct value for given timezone
  if (Timezone != 0)
  {
    long Correction = (constrain(Timezone, -12, 12) * -3600);
    Unix += Correction; 
  }
  
  // Seconds from 1970 until 1 jan 00:00:00 of the given year
  Unix += DateTime.Year * (86400 * 365); // 31536000 seconds per year

  // Add extra days for leap years
  for (Counter = 0; Counter < DateTime.Year; Counter++)
  {
    if (LEAP_YEAR(Counter))
      Unix += 86400;
  }

  // Add days for given year - Months start from 1
  for (Counter = 1; Counter < DateTime.Month; Counter++)
  {
    // February - Leap year
    if ((Counter == 2) && LEAP_YEAR(DateTime.Year))
      Unix += (86400 * 29); // 29 days
    
    else
      Unix += 86400 * _MonthDuration[Counter - 1];
  }

  // Add days for given month
  Unix += (DateTime.Day - 1) * 86400; // 86400 seconds per day
  Unix += DateTime.Hour * 3600;	// 3600 seconds per hour
  Unix += DateTime.Minute * 60; // 60 seconds per minute
  Unix += DateTime.Second;

  // Return seconds
  return Unix;
}

/* ------------------------------------------------------------------------------------------- */

// Name:        UnixToDateTime
// Description: Converts Unix timestamp to DS1390DateTime structure - Ignores hundredths of sec.
// Arguments:   Unix - Unix timestamp
//				      DateTime - DS1390DateTime structure to store the data
//				      Timezone - Timezone info (-12 to +12, 0 = GMT) of Unix
// Returns:     None

void DS1390::UnixToDateTime (unsigned long Unix, DS1390DateTime &DateTime, int Timezone)
{
  // Variables
  unsigned long UnixTime = Unix;
  unsigned char Year = 0;
  unsigned char Month = 0;
  unsigned char MonthLength = 0;
  unsigned long Days = 0;

  // Correct value for given timezone
  if (Timezone != 0)
    UnixTime += (Timezone * 3600);
  
  // Calculate seconds
  DateTime.Second = UnixTime % 60;

  // Get remaining time in minutes
  UnixTime /= 60;

  // Calculate minutes
  DateTime.Minute = UnixTime % 60;

  // Get remaining time in hours
  UnixTime /= 60;

  // Calculate hours
  DateTime.Hour = UnixTime % 24;

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
  UnixTime /= 24;

  // Get weekday
  DateTime.Wday = ((UnixTime + 4) % 7) + 1;  // Sunday is day 1 

  // Calculate years since 1970
  while((unsigned)(Days += (LEAP_YEAR(Year) ? 366 : 365)) <= UnixTime) 
    Year++;

  // Get absolute value
  DateTime.Year = Year - 30;

  Days -= LEAP_YEAR(Year) ? 366 : 365;

  // Get remaining time in days since Jan 1 of the given year
  UnixTime -= Days;

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
    
    if (UnixTime >= MonthLength) 
      UnixTime -= MonthLength;
    
    else 
      break;
  }

  // Calculate month
  DateTime.Month = Month + 1;

  // Calculate day
  DateTime.Day = UnixTime + 1; 
}

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
