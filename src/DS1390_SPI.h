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

// Code version
#define DS1390_CODE_NAME		"DS1390_SPI"
#define DS1390_CODE_VERSION		"1.4"

// DS1390 SPI clock speed
#define DS1390_SPI_CLOCK        4000000

// Trickle charger modes
#define DS1390_TCH_DISABLE      0x00  // Disabled
#define DS1390_TCH_250_NO_D     0xA5  // 250 Ohms without diode
#define DS1390_TCH_250_D        0xA9  // 250 Ohms with diode
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
#define DS1390_MASK_OSF     	0x80  // Oscillator stop flag bit
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
  uint8_t Hsecond = 0;  // Hundredths of Seconds   
  uint8_t Second = 0;   // Seconds
  uint8_t Minute = 0;   // Minutes 
  uint8_t Hour = 0;     // Hours 
  uint8_t Wday = 0;     // Day of the week (1 = Sunday)
  uint8_t Day = 0;      // Day
  uint8_t Month = 0;    // Month/Century
  uint8_t Year = 0;     // Year
  uint8_t Century = 0;  // Century - Logic 0 if year <= 99 or logic if year > 99
  uint8_t AmPm = 0;     // AmPm flag - This field is set to 0 if 24h format is active
  
} DS1390DateTime;

/* ------------------------------------------------------------------------------------------- */
// DS1390 class
/* ------------------------------------------------------------------------------------------- */

class DS1390
{
  public:
    // Constructor
    DS1390 (uint16_t PinCs);

    // Initializer
    void begin ();

    // Time format related functions
    uint8_t getTimeFormat ();
    bool setTimeFormat (uint8_t Format);
	
	// Data validation related functions
	bool getValidation ();
    void setValidation (); 

    // Date and time related functions
    void getDateTimeAll(DS1390DateTime &DateTime);
    void setDateTimeAll(DS1390DateTime &DateTime);
    uint8_t getDateTimeHSeconds ();
    void setDateTimeHSeconds (uint8_t Value);    
    uint8_t getDateTimeSeconds ();
    bool setDateTimeSeconds (uint8_t Value);
    uint8_t getDateTimeMinutes ();
    bool setDateTimeMinutes (uint8_t Value);     
    uint8_t getDateTimeHours ();
    bool setDateTimeHours (uint8_t Value);         
    uint8_t getDateTimeWday ();
    bool setDateTimeWday (uint8_t Value); 
    uint8_t getDateTimeDay ();
    bool setDateTimeDay (uint8_t Value); 
    uint8_t getDateTimeMonth ();
    bool setDateTimeMonth (uint8_t Value); 
    uint8_t getDateTimeYear ();
    bool setDateTimeYear (uint8_t Value);   
    uint8_t getDateTimeCentury ();
    bool setDateTimeCentury (uint8_t Value); 	
    uint8_t getDateTimeAmPm ();
    bool setDateTimeAmPm (uint8_t Value);  
	uint32_t getDateTimeEpoch (int Timezone);
	void setDateTimeEpoch(uint32_t Epoch, int Timezone);
                    
    // Trickle charger related functions
    uint8_t getTrickleChargerMode ();
    bool setTrickleChargerMode (uint8_t Mode);
	
	// Epoch timestamp related functions
	uint32_t dateTimeToEpoch (DS1390DateTime &DateTime, int Timezone);
	void epochToDateTime (uint32_t Epoch, DS1390DateTime &DateTime, int Timezone);
	
  private:  
    // CS pin mask
    uint16_t _PinCs;

    // DateTime buffer
    DS1390DateTime _DateTimeBuffer;

    // Duration of months of the year
    const uint8_t _MonthDuration[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    

    // Device memory related functions
    void writeByte (uint8_t Address, uint8_t Data);
    uint8_t readByte (uint8_t Address); 
    
    // Data conversion related functions
    static uint8_t dec2bcd (uint8_t DecValue);
    static uint8_t bcd2dec (uint8_t BCDValue);
};

#endif

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
