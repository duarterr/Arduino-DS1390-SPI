/* ------------------------------------------------------------------------------------------- */
// ESP_NTPCompare - This example compares DS1390 registers with date and time info from the web
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

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

/* ------------------------------------------------------------------------------------------- */
// Hardware defines
/* ------------------------------------------------------------------------------------------- */

// Peripheral pins
#define PIN_RTC_CS               10

/* ------------------------------------------------------------------------------------------- */
// Software defines
/* ------------------------------------------------------------------------------------------- */

// WiFi settings
const char *ssid     = "ssid";
const char *password = "psw";

// Timezone (-12 +12)
#define TIMEZONE                 -3

/* ------------------------------------------------------------------------------------------- */
// Constructor
/* ------------------------------------------------------------------------------------------- */

// RTC constructor
DS1390 Clock (PIN_RTC_CS);

// Date and time struct - From DS1390 library
DS1390DateTime Time;

// UDP
WiFiUDP UDP;

// NTP client - Result is given by server closest to you, so it's usually in your timezone
NTPClient NTP(UDP, "0.br.pool.ntp.org"); // Brazilian server

/* ------------------------------------------------------------------------------------------- */
// Variables
/* ------------------------------------------------------------------------------------------- */

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

  // Start wifi
  WiFi.begin(ssid, password);

  // Wait for connection
  Serial.printf ("Connecting to WiFi");
  while ( WiFi.status() != WL_CONNECTED ) 
  {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println();
  Serial.println ("Done.");
  Serial.println();

  // Start NTP client
  NTP.begin();     
}

/* ------------------------------------------------------------------------------------------- */
// Loop function
/* ------------------------------------------------------------------------------------------- */

void loop()
{ 
  // Read DS1390 registers
  unsigned long Epoch = Clock.getDateTimeEpoch (TIMEZONE);

  // Convert Epoch to DateTime - Not necessary. Values can be obtained directly using Clock.getDateTimeAll (Time); 
  Clock.EpochToDateTime (Epoch, Time, TIMEZONE);
  
  // Display result
  Serial.println ("DS1390: ");
  
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

  Serial.printf ("Epoch: %d \n", Epoch);  

  // Get NTP time
  NTP.update();

  // Display result
  Serial.println ("NTP: ");

  Clock.EpochToDateTime (NTP.getEpochTime(), Time, TIMEZONE);

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

  Serial.printf ("Epoch: %d \n\n", NTP.getEpochTime());    

  delay (1000);
}

/* ------------------------------------------------------------------------------------------- */
// End of code
/* ------------------------------------------------------------------------------------------- */
