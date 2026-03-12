#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <LowPower.h>

// --- Tunables ---
// How often to log data to the SD card, in seconds.
// Supported range: 5 to 60 seconds.
#define LOGGING_INTERVAL_SECONDS 15

// --- Pin Definitions ---
const int PIN_SENSOR_VOUT = A0; // MPX5010DP Vout
const int PIN_BATTERY_LBO = 2;  // PowerBoost 1000C LBO (Low Battery Output)
const int PIN_SD_CS = 10;       // SD Card Chip Select (Adafruit Data Logging Shield)

// --- Constants ---
// AREF voltage (default is 5V, but can be slightly different depending on USB/Battery)
// For max accuracy, measure the actual 5V line and update this, or use the internal 1.1V ref (if applicable)
const float REFERENCE_VOLTAGE = 5.0; 

// --- Globals ---
RTC_PCF8523 rtc; // Adafruit Data Logging shield uses PCF8523
File logFile;
char filename[15];

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    // Wait for serial port to connect, but timeout after 3 seconds for standalone operation
  }
  
  Serial.println(F("Initializing Pressure Logger..."));

  // 1. Initialize Pins
  pinMode(PIN_BATTERY_LBO, INPUT_PULLUP); // PowerBoost LBO is open-drain, requires pullup
  pinMode(PIN_SD_CS, OUTPUT);
  
  // 2. Initialize RTC
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    // Try to proceed anyway, but timestamps will be wrong or missing
  } else if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println(F("RTC is NOT initialized, let's set the time!"));
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // 3. Initialize SD Card
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println(F("Card failed, or not present"));
    // We cannot proceed without an SD card. Sleep forever.
    sleepForever();
  }
  Serial.println(F("card initialized."));

  // 4. Create a new log file
  // Find a name like LOG_0001.CSV
  strcpy(filename, "LOG_0000.CSV");
  for (uint16_t i = 0; i < 10000; i++) {
    filename[4] = '0' + i/1000;
    filename[5] = '0' + (i/100)%10;
    filename[6] = '0' + (i/10)%10;
    filename[7] = '0' + i%10;
    if (!SD.exists(filename)) {
      break; 
    }
  }

  logFile = SD.open(filename, FILE_WRITE);
  if (!logFile) {
    Serial.print(F("Could not create "));
    Serial.println(filename);
    sleepForever();
  }

  Serial.print(F("Logging to: "));
  Serial.println(filename);

  // Write CSV Header
  logFile.println(F("Timestamp,Battery_Low,Raw_ADC,Voltage_V,Pressure_kPa"));
  logFile.flush(); // Ensure header is saved
  
  Serial.println(F("Setup complete. Starting logging loop."));
}

void loop() {
  // 1. Check Battery Status
  bool isBatteryLow = (digitalRead(PIN_BATTERY_LBO) == LOW);
  
  if (isBatteryLow) {
    Serial.println(F("Battery is LOW (LBO=0V). Halting to protect battery."));
    // Flush and close the file to prevent corruption
    if (logFile) {
      logFile.flush();
      logFile.close();
    }
    sleepForever();
  }

  // 2. Read Time
  DateTime now;
  if (rtc.begin()) {
     now = rtc.now();
  }

  // 3. Read Pressure Sensor
  // Read A0 multiple times and average to reduce noise
  const int NUM_SAMPLES = 10;
  long sumReadings = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sumReadings += analogRead(PIN_SENSOR_VOUT);
    delay(2); // Short delay between samples
  }
  float rawADC = (float)sumReadings / NUM_SAMPLES;
  
  // Calculate Voltage
  float voltage = (rawADC / 1023.0) * REFERENCE_VOLTAGE;

  // Calculate Pressure in kPa
  // MPX5010DP Transfer Function: Vout = Vs * (0.09 * P + 0.04) +/- Error
  // Rearranging for P: P = (Vout / Vs - 0.04) / 0.09
  // Assuming Vs = 5.0V (same as REFERENCE_VOLTAGE)
  float pressure_kPa = (voltage / REFERENCE_VOLTAGE - 0.04) / 0.09;
  
  // Basic bounds checking to prevent nonsensical negative values near 0
  if (pressure_kPa < 0.0) {
    pressure_kPa = 0.0;
  }

  // 4. Log Data
  if (logFile) {
    // Timestamp (ISO 8601-ish)
    logFile.print(now.year(), DEC);
    logFile.print('/');
    printFixed(now.month());
    logFile.print('/');
    printFixed(now.day());
    logFile.print('T');
    printFixed(now.hour());
    logFile.print(':');
    printFixed(now.minute());
    logFile.print(':');
    printFixed(now.second());
    logFile.print(',');
    
    // Data
    logFile.print(isBatteryLow ? "YES" : "NO");
    logFile.print(',');
    logFile.print(rawADC, 2);
    logFile.print(',');
    logFile.print(voltage, 4);
    logFile.print(',');
    logFile.println(pressure_kPa, 4);
    
    logFile.flush(); // Force write to SD card immediately
  }
  
  // 5. Print to Serial for debugging (if connected)
  if (Serial) {
     Serial.print(F("V: "));
     Serial.print(voltage, 4);
     Serial.print(F(" P(kPa): "));
     Serial.println(pressure_kPa, 4);
  }

  // 6. Deep Sleep
  // We need to sleep for LOGGING_INTERVAL_SECONDS.
  // The LowPower library supports discrete sleep intervals up to 8 seconds.
  // We'll loop to achieve the desired total sleep time.
  int secondsRemaining = LOGGING_INTERVAL_SECONDS;
  
  // Flush serial before sleeping so we don't truncate messages
  Serial.flush(); 
  
  while (secondsRemaining > 0) {
    if (secondsRemaining >= 8) {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      secondsRemaining -= 8;
    } else if (secondsRemaining >= 4) {
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
      secondsRemaining -= 4;
    } else if (secondsRemaining >= 2) {
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      secondsRemaining -= 2;
    } else if (secondsRemaining >= 1) {
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
      secondsRemaining -= 1;
    }
  }
}

// Helper to print 2-digit numbers with leading zeros

void printFixed(int val) {
  if (val < 10) {
    logFile.print('0');
  }
  logFile.print(val, DEC);
}

// Enter infinite low power sleep
void sleepForever() {
  Serial.println(F("Going to sleep forever."));
  Serial.flush();
  while (true) {
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}
