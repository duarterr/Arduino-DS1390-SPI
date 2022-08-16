/* ------------------------------------------------------------------------------------------- */
// BasicEpoch - This example sets and gets DS1390 registers using an Epoch timestamp
// Author:  Renan R. Duarte
// E-mail:  duarte.renan@hotmail.com
// Date:    October 14, 2019
//
// Released into the public domain
/* ------------------------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------------------------- */
// Libraries
/* ------------------------------------------------------------------------------------------- */

#include "DS1390_SPI.h" // https://github.com/duarterr/Arduino-DS1390-SPI

/* ------------------------------------------------------------------------------------------- */
// Software defines
/* ------------------------------------------------------------------------------------------- */

// Timezone (-12 +12)
#define TIMEZONE                -3

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

// Epoch timestamp
unsigned long Epoch = 0;

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

  // Set DS1390 time
  Epoch = 1044072123; // 01:02:03 - 01/02/2003 - GMT -3

  Clock.setDateTimeEpoch (Epoch, TIMEZONE);
}

/* ------------------------------------------------------------------------------------------- */
// Loop function
/* ------------------------------------------------------------------------------------------- */

void loop()
{
  // Get Epoch time from DS1390
  unsigned long Epoch = Clock.getDateTimeEpoch (TIMEZONE);
  Serial.printf ("Epoch: %d \n", Epoch);

  // Convert to DateTime
  Clock.epochToDateTime (Epoch, Time, TIMEZONE);

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

  Serial.printf ("%02d/%02d/20%02d - %02d:%02d:%02d \n", Time.Day, Time.Month, Time.Year, Time.Hour, Time.Minute, Time.Second);
  //Serial.printf ("AM/PM: %d \n", Time.AmPm);

  delay (1000);
}

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
