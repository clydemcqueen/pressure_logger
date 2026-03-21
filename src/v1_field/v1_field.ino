#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <LowPower.h>

// Design:
// -- Optimize for low power. E.g., do not use Serial in the loop.
// -- Fatal error in setup(): blink LED2 (red) forever to alert the operator.
// -- Non-fatal error in setup(): blink LED1 (green) a few times to indicate the type of error.
// -- If something goes wrong in loop(), note the problem in the log, and sleep forever.

const int STATUS_RTC_MISSING = 1;
const int STATUS_RTC_LOST_POWER = 2;
const int STATUS_OK = 3;

const int PIN_SENSOR_VOUT = A0;     // MPX5010DP Vout
const int PIN_BATTERY_LBO = 2;      // PowerBoost 1000C LBO (Low Battery Output)
const int PIN_LED1 = 3;             // Logging shield LED1, green
const int PIN_LED2 = 4;             // Logging shield LED2, red
const int PIN_SD_CS = 10;           // SD card chip select

const int LOGGING_INTERVAL_S = 5;   // Logging interval in seconds

const int NUM_SAMPLES = 10;         // Read ADC N times to reduce noise
const float REFERENCE_V = 5.0;      // Reference voltage for ADC

RTC_PCF8523 rtc;
File logFile;
bool monitor_battery = true;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    // Wait a bit for a serial connection, then give up
  }
  
  Serial.println(F("Setup()"));

  // Initialize pins
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_SD_CS, OUTPUT);
  if (monitor_battery) {
    pinMode(PIN_BATTERY_LBO, INPUT_PULLUP); // PowerBoost LBO is open-drain, requires pullup
  }

  // Turn on LED1 to show we're in setup
  digitalWrite(PIN_LED1, HIGH);
  int status = STATUS_OK;

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println(F("RTC missing"));
    status = STATUS_RTC_MISSING;
  } else if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println(F("RTC lost power"));
    status = STATUS_RTC_LOST_POWER;
  }

  // Tell the SD card to call back for the date and time
  SdFile::dateTimeCallback(dateTime);

  // Initialize SD card
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println(F("No SD card, fatal"));
    fatalError();
  }

  // Find a name like LOG_0001.CSV
  char filename[15];
  strcpy(filename, "LOG_0000.CSV");
  // FAT16 limit is 512 files in the root directory, though I am using FAT32
  for (uint16_t i = 0; i < 500; i++) {
    filename[4] = '0' + i/1000;
    filename[5] = '0' + (i/100)%10;
    filename[6] = '0' + (i/10)%10;
    filename[7] = '0' + i%10;
    if (!SD.exists(filename)) {
      break; 
    }
  }

  Serial.print(F("Log file: "));
  Serial.println(filename);

  logFile = SD.open(filename, FILE_WRITE);
  if (!logFile) {
    Serial.print(F("Could not open log file, fatal"));
    fatalError();
  }

  // Write CSV header
  logFile.println(F("timestamp,pressure_kpa"));
  logFile.flush();
  
  // Done with setup
  reportSetupStatus(status);
  Serial.println(F("Loop()"));
}

void loop() {
  if (monitor_battery) {
    if (digitalRead(PIN_BATTERY_LBO) == LOW) {
      // Flush and close the file to prevent corruption
      logFile.print(F("Battery is low, sleep forever\n"));
      logFile.flush();
      logFile.close();
      sleepForever();
    }
  }

  // Calculate pressure in kPa
  // MPX5010DP transfer function: Vout = Vs * (0.09 * P + 0.04) +/- Error
  float pressure_adc = readAnalog(PIN_SENSOR_VOUT);
  float pressure_kpa = ((pressure_adc / 1023.0) - 0.04) / 0.09;
  logData(pressure_kpa);

  // The LowPower library supports discrete sleep intervals up to 8 seconds
  int secondsRemaining = LOGGING_INTERVAL_S;
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

// No SD card or can't write: blink LED2 to alert the operator
void fatalError() {
  while (true) {
    digitalWrite(PIN_LED2, HIGH);
    delay(200);
    digitalWrite(PIN_LED2, LOW);
    delay(200);
  }
}

// Blink LED1 to indicate setup status, leaving the light off
void reportSetupStatus(int status) {
  digitalWrite(PIN_LED1, LOW);
  delay(1000);
  for (int i = 0; i < status; i++) {
    digitalWrite(PIN_LED1, HIGH);
    delay(200);
    digitalWrite(PIN_LED1, LOW);
    delay(200);
  }
}

// Read ADC N times to reduce noise
float readAnalog(int pin) {
  long sumReadings = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sumReadings += analogRead(pin);
    delay(2); // Short delay between samples
  }
  return (float)sumReadings / NUM_SAMPLES;
}

// Write a log entry
void logData(float pressure_kpa) {
  logFile.print(rtc.now().unixtime());
  logFile.print(',');
  logFile.println(pressure_kpa, 4);

  // Flushing the file consumes power and blinks LED_BUILTIN, useful for testing
  logFile.flush();
}

// Get the date and time
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
  *date = FAT_DATE(now.year(), now.month(), now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

// Low battery, sleep forever
void sleepForever() {
  while (true) {
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }
}
