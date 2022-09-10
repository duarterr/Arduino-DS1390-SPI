/* ------------------------------------------------------------------------------------------- */
// Basic - This example sets and gets DS1390 registers using a DateTime structure
// Author:  Renan R. Duarte
// E-mail:  duarte.renan@hotmail.com
// Date:    October 17, 2019
//
// Released into the public domain
/* ------------------------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------------------------- */
// Libraries
/* ------------------------------------------------------------------------------------------- */

#include "DS1390_SPI.h" // https://github.com/duarterr/Arduino-DS1390-SPI

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

/* ------------------------------------------------------------------------------------------- */
// Variables
/* ------------------------------------------------------------------------------------------- */

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
  Serial.printf ("%s library v%s \n", DS1390_CODE_NAME, DS1390_CODE_VERSION);

  /* ----------------------------------------------------------------------------------------- */

  // Initialize hardware
  Clock.begin();

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

  // Define time structure - 01:02:03 - 01/02/2003 - Saturday
  Time.Hsecond = 0;
  Time.Second = 3;
  Time.Minute = 2;
  Time.Hour = 1;
  // Time.AmPm = DS1390_PM; // Not used in 24h format
  Time.Wday = 7;
  Time.Day = 1;
  Time.Month = 2;
  Time.Year = 2003;

  // Set all date and time registers at once
  Clock.setDateTimeAll (Time);

  // OR

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
//  // Read all time registers at once
//  Clock.getDateTimeAll (Time);

  // OR

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

  // Display result
  Serial.printf ("DateTime: ");

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
      Serial.printf ("Fri, ");
      break;
    case 7:
      Serial.printf ("Sat, ");
      break;
  }

  Serial.printf ("%02d/%02d/%04d - %02d:%02d:%02d \n", Time.Day, Time.Month,
    Time.Year, Time.Hour, Time.Minute, Time.Second);
  //Serial.printf ("AM/PM: %d \n", Time.AmPm); // Not used

  delay (1000);
}

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
