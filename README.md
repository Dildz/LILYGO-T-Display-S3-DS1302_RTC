LILYGO T-Display-S3 DS1302 RTC Module Project
 
Description:

  This code implements a NTP-synchronized real-time clock and displays the time & date on the built-in screen of the LilyGO T-Display-S3 using the TFT_eSPI library.

How it works:
  1. Connects to WiFi using WiFiManager
  2. Synchronizes time with a NTP server every 10min
  3. Maintains local time using the DS1302 RTC module
  4. Displays time/date, sync status, IP and Wi-Fi strength info on the built-in screen

Pin Connections:
 - DS1302 VCC -> 5V
 - DS1302 GND -> GND
 - DS1302 RST -> GPIO3
 - DS1302 DAT -> GPIO13
 - DS1302 CLK -> GPIO12

DS1302 Specifications:
 - Type: RTC
 - Protocol: 3-wire serial
 - Timekeeping Range: Seconds to Year (up to 2100)
 - Clock Accuracy: Determined by external crystal
 - Operating Voltage: 2.0V to 5.5V
 - Data Storage: 31 bytes of static RAM
 - Operating Temperature: -40°C to 85°C

Notes:
 - First build the project in PlatformIO to download the various libraries.
 - In the ~.pio\libdeps\lilygo-t-display-s3\TFT_eSPI\User_Setup_Select.h file, make sure to:
   - **comment out** line 27 (#include <User_Setup.h>) and,
   - **uncomment** line 133 (#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>)
 - Only once the User_Setup_Select.h has been modified should the code be uploaded to the T-Display-S3.
