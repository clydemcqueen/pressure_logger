#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

RTC_PCF8523 rtc;

const int PIN_SENSOR_VOUT = A0; // MPX5010DP Vout

const float REFERENCE_VOLTAGE = 5.0;

bool led_on = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  if (rtc.begin()) {
    print_time();
  }
}

void loop() {
  // Heartbeat
  if (led_on) {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  led_on = !led_on;

  // Read sensor
  float rawADC = analogRead(PIN_SENSOR_VOUT);
  
  // Calculate Voltage
  float voltage = (rawADC / 1023.0) * REFERENCE_VOLTAGE;

  // Calculate Pressure in kPa
  // MPX5010DP Transfer Function: Vout = Vs * (0.09 * P + 0.04) +/- Error
  // Rearranging for P: P = (Vout / Vs - 0.04) / 0.09
  // Assuming Vs = 5.0V (same as REFERENCE_VOLTAGE)
  float pressure_kPa = (voltage / REFERENCE_VOLTAGE - 0.04) / 0.09;
  
  Serial.print(F("V: "));
  Serial.print(voltage, 4);
  Serial.print(F(" P(kPa): "));
  Serial.println(pressure_kPa, 4);
  Serial.flush(); 

  // Milliseconds between readings
  delay(250);
}

void print_time() {
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
}