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
//      	- Epoch related functions assume year is higher than 2000.
//      	- Works with DS1391 aswell.
//			- Alarm-related functions not implemented yet
//
// Knwon bugs:  - In 12h format, the device do not change the AM/PM bit neither increments
//              the day of the week and day counters. Everything works in 24h mode
//
//              - Reading Hundredths of Seconds too often makes the device loose accuracy
//
// Changelog:	v1.0 - 09/10/2019 - First release
//				v1.1 - 10/10/2019 - Century bit is now used for validation
//				v1.2 - 14/10/2019 - Bug fixes and Epoch timestamp related functions
//
// Released into the public domain
/* ------------------------------------------------------------------------------------------- */

#ifndef DS1390_SPI_h
#define DS1390_SPI_h

/* ------------------------------------------------------------------------------------------- */
// Includes
/* ------------------------------------------------------------------------------------------- */

#include "Arduino.h"
#include "SPI.h"

/* ------------------------------------------------------------------------------------------- */
// Defines
/* ------------------------------------------------------------------------------------------- */

// DS1390 SPI clock speed
#define DS1390_SPI_CLOCK        4000000

// Trickle charger modes
#define DS1390_TCH_DISABLE      0x00  // Disabled
#define DS1390_TCH_250_NO_D     0xA5  // 250 Ohms without diode
#define DS1390_TCH_250_D        0x29  // 250 Ohms with diode
#define DS1390_TCH_2K_NO_D      0xA6  // 2 kOhms without diode
#define DS1390_TCH_2K_D         0xAA  // 2 kOhms with diode
#define DS1390_TCH_4K_NO_D      0xA7  // 4 kOhms without diode
#define DS1390_TCH_4K_D         0xAB  // 4 kOhms with diode

// Date formates
#define DS1390_FORMAT_24H       0     // 24h format
#define DS1390_FORMAT_12H       1     // 12h format

#define DS1390_AM               0     // AM
#define DS1390_PM               1     // PM

// DS1390 register addresses - Read
#define DS1390_ADDR_READ_HSEC   0x00  // Hundredths of Seconds
#define DS1390_ADDR_READ_SEC    0x01  // Seconds
#define DS1390_ADDR_READ_MIN    0x02  // Minutes
#define DS1390_ADDR_READ_HRS    0x03  // Hours
#define DS1390_ADDR_READ_WDAY   0x04  // Day of the week (1 = Sunday)
#define DS1390_ADDR_READ_DAY    0x05  // Day
#define DS1390_ADDR_READ_MON    0x06  // Month/Century
#define DS1390_ADDR_READ_YRS    0x07  // Year
#define DS1390_ADDR_READ_AHSEC  0x08  // Alarm Hundredths of Seconds
#define DS1390_ADDR_READ_ASEC   0x09  // Alarm Seconds
#define DS1390_ADDR_READ_AMIN   0x0A  // Alarm Minutes
#define DS1390_ADDR_READ_AHRS   0x0B  // Alarm Hours
#define DS1390_ADDR_READ_ADAT   0x0C  // Alarm Day/Date
#define DS1390_ADDR_READ_CFG    0x0D  // Control - Used as SRAM in the DS1390
#define DS1390_ADDR_READ_STS    0x0E  // Status
#define DS1390_ADDR_READ_TCH    0x0F  // Trickle charger

// DS1390 register addresses - Write
#define DS1390_ADDR_WRITE_HSEC  0x80  // Hundredths of Seconds
#define DS1390_ADDR_WRITE_SEC   0x81  // Seconds
#define DS1390_ADDR_WRITE_MIN   0x82  // Minutes
#define DS1390_ADDR_WRITE_HRS   0x83  // Hours
#define DS1390_ADDR_WRITE_WDAY  0x84  // Day of the week (1 = Sunday)
#define DS1390_ADDR_WRITE_DAY   0x85  // Day
#define DS1390_ADDR_WRITE_MON   0x86  // Month/Century
#define DS1390_ADDR_WRITE_YRS   0x87  // Year
#define DS1390_ADDR_WRITE_AHSEC 0x88  // Alarm Hundredths of Seconds
#define DS1390_ADDR_WRITE_ASEC  0x89  // Alarm Seconds
#define DS1390_ADDR_WRITE_AMIN  0x8A  // Alarm Minutes
#define DS1390_ADDR_WRITE_AHRS  0x8B  // Alarm Hours
#define DS1390_ADDR_WRITE_ADAT  0x8C  // Alarm WDay/Day
#define DS1390_ADDR_WRITE_CFG   0x8D  // Control - Used as SRAM in the DS1390
#define DS1390_ADDR_WRITE_STS   0x8E  // Status
#define DS1390_ADDR_WRITE_TCH   0x8F  // Trickle charger

// DS1390 register bit masks
#define DS1390_MASK_AMPM        0x20  // AM/PM bit
#define DS1390_MASK_FORMAT      0x40  // 12h/24h format bit
#define DS1390_MASK_CENTURY     0x80  // Century bit
#define DS1390_MASK_AMX         0x80  // Alarm  bit (x = 1-4)
#define DS1390_MASK_DYDT        0x40  // Alarm day/date bit

// Leap year calulator
#define LEAP_YEAR(Y)            (((1970+(Y))>0) && !((1970+(Y))%4) && (((1970+(Y))%100) || !((1970+(Y))%400)))

/* ------------------------------------------------------------------------------------------- */
// Structures
/* ------------------------------------------------------------------------------------------- */

// DS1390 date and time fields
typedef struct  
{ 
  unsigned char Hsecond = 0;  // Hundredths of Seconds   
  unsigned char Second = 0;   // Seconds
  unsigned char Minute = 0;   // Minutes 
  unsigned char Hour = 0;     // Hours 
  unsigned char Wday = 0;     // Day of the week (1 = Sunday)
  unsigned char Day = 0;      // Day
  unsigned char Month = 0;    // Month/Century
  unsigned char Year = 0;     // Year
  unsigned char AmPm = 0;     // AmPm flag - This field is set to 0 if 24h format is active
  
} DS1390DateTime;

/* ------------------------------------------------------------------------------------------- */
// DS1390 class
/* ------------------------------------------------------------------------------------------- */

class DS1390
{
  public:
    // Constructor
    DS1390 (unsigned int PinCs);

    // Time format related functions
    unsigned char getTimeFormat ();
    bool setTimeFormat (unsigned char Format);
	
	// Data validation related functions
	bool getValidation ();
    void setValidation (bool Value); 

    // Date and time related functions
    void getDateTimeAll(DS1390DateTime &DateTime);
    void setDateTimeAll(DS1390DateTime &DateTime);
    unsigned char getDateTimeHSeconds ();
    void setDateTimeHSeconds (unsigned char Value);    
    unsigned char getDateTimeSeconds ();
    bool setDateTimeSeconds (unsigned char Value);
    unsigned char getDateTimeMinutes ();
    bool setDateTimeMinutes (unsigned char Value);     
    unsigned char getDateTimeHours ();
    bool setDateTimeHours (unsigned char Value);         
    unsigned char getDateTimeWday ();
    bool setDateTimeWday (unsigned char Value); 
    unsigned char getDateTimeDay ();
    bool setDateTimeDay (unsigned char Value); 
    unsigned char getDateTimeMonth ();
    bool setDateTimeMonth (unsigned char Value); 
    unsigned char getDateTimeYear ();
    bool setDateTimeYear (unsigned char Value);     
    unsigned char getDateTimeAmPm ();
    bool setDateTimeAmPm (unsigned char Value);  
	unsigned long getDateTimeEpoch (int Timezone);
	void setDateTimeEpoch(unsigned long Epoch, int Timezone);
                    
    // Trickle charger related functions
    unsigned char getTrickleChargerMode ();
    bool setTrickleChargerMode (unsigned char Mode);
	
	// Epoch timestamp related functions
	unsigned long DateTimeToEpoch (DS1390DateTime &DateTime, int Timezone);
	void EpochToDateTime (unsigned long Epoch, DS1390DateTime &DateTime, int Timezone);
	
  private:  
    // CS pin mask
    unsigned int _PinCs;

    // DateTime buffer
    DS1390DateTime _DateTimeBuffer;

    // Duration of months of the year
    const unsigned char _MonthDuration[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    

    // Device memory related functions
    void writeByte (unsigned char Address, unsigned char Data);
    unsigned char readByte (unsigned char Address); 
    
    // Data conversion related functions
    unsigned char dec2bcd (unsigned char DecValue); 
    unsigned char bcd2dec (unsigned char BCDValue);    
};

#endif

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
