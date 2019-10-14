#include "DS1390_SPI.h"

/* ------------------------------------------------------------------------------------------- */
// Hardware defines
/* ------------------------------------------------------------------------------------------- */

// Peripheral pins
#define PIN_RTC_CS               10

/* ------------------------------------------------------------------------------------------- */
// Constructor
/* ------------------------------------------------------------------------------------------- */

// RTC constructor
DS1390 Clock (PIN_RTC_CS);

// Date and time struct - From DS1390 library
DS1390DateTime Time;

/* ------------------------------------------------------------------------------------------- */
// Initialization function
/* ------------------------------------------------------------------------------------------- */

void setup()
{
  Serial.begin(74480);
  while (!Serial);

  Serial.println();
  Serial.print("Code date: ");
  Serial.print(__DATE__);
  Serial.print(" - ");
  Serial.println(__TIME__);

  /* ----------------------------------------------------------------------------------------- */

  // Delay for DS1390 boot (mandatory)
  delay (200);

  // Checkif memory was lost recently
  if (Clock.getValidation () == true)
    Serial.println ("Memory content is valid.");
  else
    Serial.println ("Memory content was recently lost! Please update date and time values.");
    
  // Configure trickle carger (250 ohms with one series diode)
  Clock.setTrickleChargerMode (DS1390_TCH_250_D);

  // Set time format - 24h
  //Clock.setTimeFormat(DS1390_FORMAT_24H);
  
  // Get current format to confirm configuration
  if (Clock.getTimeFormat() == DS1390_FORMAT_24H)
    Serial.println ("Format: 24h");
  else
    Serial.println ("Format: 12h");

//  // Define time structure
//  Time.Hsecond = 0;
//  Time.Second = 30;
//  Time.Minute = 55;
//  Time.Hour = 14;
//  Time.AmPm = DS1390_PM;
//  Time.Wday = 4;
//  Time.Day = 10;
//  Time.Month = 10;
//  Time.Year = 19;
//
//  // Set all date and time registers at once
//  Clock.setDateTimeAll (Time);

//  // Set each register individually
//  Clock.setDateTimeHSeconds (Time.Hsecond); 
//  Clock.setDateTimeSeconds (Time.Second);
//  Clock.setDateTimeMinutes (Time.Minute);  
//  Clock.setDateTimeHours (Time.Hour);      
//  Clock.setDateTimeWday (Time.Wday);
//  Clock.setDateTimeDay (Time.Day);
//  Clock.setDateTimeMonth (Time.Month);
//  Clock.setDateTimeYear (Time.Year);
//  Clock.setDateTimeAmPm (Time.AmPm);  
}

/* ------------------------------------------------------------------------------------------- */
// Loop function
/* ------------------------------------------------------------------------------------------- */

void loop()
{ 
  // Read all time registers at once
//  Clock.getDateTimeAll (Time);

  // Read each register individually
  //Time.Hsecond = Clock.getDateTimeHSeconds (); 
  Time.Second = Clock.getDateTimeSeconds ();
  Time.Minute = Clock.getDateTimeMinutes ();  
  Time.Hour = Clock.getDateTimeHours ();      
  Time.Wday = Clock.getDateTimeWday ();
  Time.Day = Clock.getDateTimeDay ();
  Time.Month = Clock.getDateTimeMonth ();
  Time.Year = Clock.getDateTimeYear ();
  //Time.AmPm = Clock.getDateTimeAmPm ();

  switch (Time.Wday)
  {
    case 1:
      Serial.printf ("Sun, ");
      break;
    case 2:
      Serial.printf ("Mon, "); 
      break;
    case 3:
      Serial.printf ("Tue, "); 
      break;
    case 4:
      Serial.printf ("Wed, ");
      break; 
    case 5:
      Serial.printf ("Thu, ");
      break;
    case 6:
      Serial.printf ("Fry, "); 
      break; 
    case 7:
      Serial.printf ("Sat, ");
      break;                               
  }

  Serial.printf ("%02d/%02d/20%02d - %02d:%02d:%02d \n", Time.Day, Time.Month, Time.Year, Time.Hour, Time.Minute, Time.Second);
  //Serial.printf ("AM/PM: %d \n", Time.AmPm);

  unsigned long Unix = Clock.DateTimeToUnix (Time, -3);
  Serial.printf ("Unix: %d \n", Unix);

  Clock.UnixToDateTime (Unix, Time, -3);

  switch (Time.Wday)
  {
    case 1:
      Serial.printf ("Sun, ");
      break;
    case 2:
      Serial.printf ("Mon, "); 
      break;
    case 3:
      Serial.printf ("Tue, "); 
      break;
    case 4:
      Serial.printf ("Wed, ");
      break; 
    case 5:
      Serial.printf ("Thu, ");
      break;
    case 6:
      Serial.printf ("Fry, "); 
      break; 
    case 7:
      Serial.printf ("Sat, ");
      break;                               
  }

  Serial.printf ("%02d/%02d/20%02d - %02d:%02d:%02d \n", Time.Day, Time.Month, Time.Year, Time.Hour, Time.Minute, Time.Second);
  //Serial.printf ("AM/PM: %d \n", Time.AmPm);

  delay (1000);
}

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
