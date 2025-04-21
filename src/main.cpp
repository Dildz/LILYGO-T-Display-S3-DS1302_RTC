/*************************************************************
******************* INCLUDES & DEFINITIONS *******************
**************************************************************/

// Board libraries
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <TFT_eSPI.h>

// Pin definitions
#include "pins.h"


// RTC Module connections
#define RST  PIN_GPIO_3
#define DAT  PIN_GPIO_13
#define CLK  PIN_GPIO_12

// Initialize RtcDS1302
ThreeWire myWire(DAT, CLK, RST); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

// Initialize TFT display
TFT_eSPI lcd = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&lcd);

// NTP settings
const char* ntpServer = "pool.ntp.org";
//#################### EDIT THIS SECTION ###################
const long gmtOffset = 2; // adjust for your timezone
const int dstOffset = 0;  // adjust for daylight saving
//##########################################################

// Timing variables
unsigned long previousDisplayMillis = 0;
unsigned long previousSyncMillis = 0;
const long displayInterval = 1000; // 1 second display update
const long syncInterval = 600000;  // 10 minutes (600,000ms) NTP sync
String lastResponseTime = "";      // store the last response string

// Sync tracking
enum SyncState {
  SYNC_FAILED,
  SYNC_SUCCESS
};
SyncState rtcSyncState = SYNC_FAILED; // start in failed state until first successful sync

// Get time as a string
String getTimeString() {
  RtcDateTime now = Rtc.GetDateTime();
  char buf[9];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", 
           now.Hour(), now.Minute(), now.Second());
  return String(buf);
}

// Get date as a string
String getDateString() {
  RtcDateTime now = Rtc.GetDateTime();
  char buf[11];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d",
           now.Day(), now.Month(), now.Year());
  return String(buf);
}

// Get Wi-Fi signal strength as a string
String getWifiRSSI() {
  long rssi = WiFi.RSSI();
  return String(rssi) + "dBm";
}

// Signal quality colour indicators
uint16_t getWifiQualityColour(long rssi) {
  if (WiFi.status() != WL_CONNECTED) return TFT_DARKGREY; // dark grey if disconnected
  if (rssi >= -50) return TFT_GREEN;       // excellent - green
  else if (rssi >= -70) return TFT_YELLOW; // okay - yellow
  else return TFT_RED;                     // poor - red
}

// IP address as a string
String ipString = "";


/*************************************************************
********************** HELPER FUNCTIONS **********************
**************************************************************/

// Function to sync time with NTP server
void syncWithNTP() {
  configTime(gmtOffset * 3600, dstOffset * 3600, ntpServer);
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5000)) { // Wait up to 5 seconds
    lastResponseTime = "compiled";
    rtcSyncState = SYNC_FAILED;
    return;
  }

  // Create new time object
  RtcDateTime newTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );

  // Set the new time
  Rtc.SetDateTime(newTime);
  
  // Update success state and message
  lastResponseTime = getTimeString();
  rtcSyncState = SYNC_SUCCESS;
}

// Function to draw the display
void drawDisplay() {
  sprite.fillSprite(TFT_BLACK);

  // Draw time in the middle (font 7)
  sprite.drawString(getTimeString(), 50, 70, 7); // 00:00:00
  
  // Display RTC sync state label
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.drawString("RTC:", 4, 4);

  // RTC sync state
  sprite.setTextColor(rtcSyncState == SYNC_SUCCESS ? TFT_GREEN : TFT_RED, TFT_BLACK);
  sprite.drawString(rtcSyncState == SYNC_SUCCESS ? "SYNCED" : "FAILED", 40, 4);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.drawString("> " + lastResponseTime, 82, 4);

  // Display date (DD/MM/YYYY)
  sprite.drawString("DATE: " + getDateString(), 4, 16);

  // Display IP address & Wi-Fi strength with quality indicator
  String rssiString;
  long rssi = 0; // initialize rssi variable

  if (WiFi.status() == WL_CONNECTED) {
    rssi = WiFi.RSSI(); // get actual RSSI value
    rssiString = "RSSI: " + String(rssi) + "dBm";
    sprite.drawString("IP:   " + ipString, 190, 4);
  }
  else {
    rssiString = "WiFi: DISCONNECTED";
    sprite.drawString("IP:   DISCONNECTED", 190, 4);
  }

  sprite.drawString(rssiString, 190, 16);
  sprite.fillCircle(190 + sprite.textWidth(rssiString) + 10, 20, 4, getWifiQualityColour(rssi)); // pass actual rssi value

  // Push to display
  sprite.pushSprite(0,0);
}


/*************************************************************
*********************** MAIN FUNCTIONS ***********************
**************************************************************/

// SETUP
void setup() {
  // Initialize hardware
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  // Initialize display
  lcd.init();
  lcd.setRotation(3);
  analogWrite(PIN_LCD_BL, 80); // set brightness 80/255
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(0x7BCF, TFT_BLACK); // converted from #787878
  sprite.createSprite(320, 170);

  // Display initial message
  lcd.println("\nConnecting to Wi-Fi - please wait...\n");

  // Create an instance of WiFiManager
  WiFiManager wifiManager;
  wifiManager.setConnectTimeout(10);
  wifiManager.setConnectRetries(3);
  wifiManager.setConfigPortalTimeout(0);
  
  // Callback for config portal
  wifiManager.setAPCallback([](WiFiManager *wm) {
    lcd.println("AP unreachable or not yet configured.\n\n");
    lcd.println("A Wi-Fi network has been created:\n");
    lcd.println("SSID:     T-Display-S3\n");
    lcd.println("Password: 123456789\n\n");
    lcd.println("Connect and navigate to: 192.168.4.1\n");
    lcd.println("in a browser to setup your Wi-Fi.\n\n");
  });

  // Callback for a saved config
  wifiManager.setSaveConfigCallback([]() {
    lcd.println("Configuration saved, rebooting...");
    delay(2000);
    ESP.restart();
  });

  // This will block until connected to saved network or after config
  wifiManager.autoConnect("T-Display-S3", "123456789");

  // Wi-Fi connected :)
  lcd.println("WiFi Connected! :)\n");
  lcd.print("SSID: ");
  lcd.println(WiFi.SSID());
  lcd.print("IP: ");
  lcd.println(WiFi.localIP());
  delay(2000);
  
  // Time sync message
  lcd.println("\nSyncing time...");

  // Initialize RTC
  Rtc.Begin();

  // Initial NTP sync with retry logic
  bool syncSuccess = false;
  int retryCount = 0;
  const int maxRetries = 3;

  while (!syncSuccess && retryCount < maxRetries) {
    lcd.printf("Attempt %d of %d\n", retryCount + 1, maxRetries);
    syncWithNTP();
    
    if (rtcSyncState == SYNC_SUCCESS) {
      syncSuccess = true;
    } else {
      retryCount++;
      delay(2000); // wait 2 seconds before retrying
    }
  }

  // RTC message
  lcd.println("\nValidating RTC module...\n");
  delay(1000);

  // Validate RTC
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid()) {
    lcd.println("\nRTC lost confidence in the DateTime!\nUsing compiled time");
    Rtc.SetDateTime(compiled);
    rtcSyncState = SYNC_FAILED;
    lastResponseTime = "compiled";
  }
  else {
    if (rtcSyncState == SYNC_SUCCESS) {
      lcd.println("\nRTC synced with NTP time server");
      lcd.println(lastResponseTime);
    }
    else {
      lcd.println("\nRTC has valid time but NTP sync failed");
    }
  }

  if (Rtc.GetIsWriteProtected()) {
    lcd.println("\nRTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    lcd.println("\nRTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }
  delay(2000);

  lcd.println("\nStarting main display...");
  delay(2000);
}

// MAIN LOOP
void loop() {
  unsigned long currentMillis = millis();

  // Check WiFi status and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastReconnectAttempt = 0;
    if (currentMillis - lastReconnectAttempt >= 10000) { // try every 10 seconds
      lastReconnectAttempt = currentMillis;
      WiFi.reconnect();
      
      // Update IP string if reconnection succeeds
      if (WiFi.status() == WL_CONNECTED) {
        ipString = WiFi.localIP().toString();
      }
    }
  }
  
  // Update display every second
  if (currentMillis - previousDisplayMillis >= displayInterval) {
    previousDisplayMillis = currentMillis;
    drawDisplay();
  }
  
  // Sync with NTP every 10 minutes
  if (currentMillis - previousSyncMillis >= syncInterval) {
    previousSyncMillis = currentMillis;
    syncWithNTP();
  }
}
