#include <RTClib.h>

// Compile and run this once to set the time on the RTC

RTC_PCF8523 rtc;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial monitor

  if (!rtc.begin()) {
    Serial.println("No RTC");
    while (1);
  }

  Serial.println("Set RTC to current time");
  
  // Manually set to a specific time
  // rtc.adjust(DateTime(2026, 3, 17, 13, 21, 33));

  // Set the RTC to the date & time this sketch was compiled, adjusted for timezone
  DateTime local(F(__DATE__), F(__TIME__));
  rtc.adjust(DateTime(local.unixtime() + 7 * 3600));
  
  Serial.println("Current time from RTC (GMT):");
}

void loop() {
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  delay(3000);
}
