# Arduino Library for Maxim DS1390 real time clock - SPI mode

## Usage

All date and time values can be stored in a "DS1390DateTime" struct. This struct is defined in the header file.

All parameters that can be passed as arguments to functions expect to receive values defined in the header file. Ex: To disable the trickle charger, call the function "setTrickleChargerMode" and pass DS1390_TCH_DISABLE as argument. 

## Notes

A 200ms (min) delay is required after boot to read/write device memory (tRST).

Epoch related functions assume year is higher than 2000.

Century and Hundredths of Seconds registers are ignored in Epoch related functions

Works with DS1391 aswell.

Alarm-related functions not implemented yet.

## Credits

Renan R. Duarte <duarte.renan@hotmail.com>

## Known bugs
In 12h format, the device do not change the AM/PM bit neither increments the day of the week and day counters. Everything works in 24h mode.

Reading Hundredths of Seconds too often makes the device loose accuracy.

## Website

You can find the latest version of the library at
https://github.com/duarterr/Arduino-DS1390-SPI

## License

This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, 1but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
