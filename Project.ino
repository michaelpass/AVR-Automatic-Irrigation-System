#include "Adafruit_seesaw.h"
#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS3231.h>

RTClib myRTC;

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Adafruit_seesaw ss;

uint16_t threshhold = 500;


void setup() {
  Serial.begin(9600);

  Wire.begin();

  lcd.begin(16,2);
  
  if (!ss.begin(0x36)) {
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1);
  }
  
}

void loop() {

  DateTime now = myRTC.now();

  uint16_t capread = ss.touchRead(0);

  lcd.setCursor(0,0);
  // lcd.print("                "); // Clear line
  lcd.setCursor(0,0);
  lcd.print("Moisture: ");

  lcd.print(capread, DEC);
  lcd.print("   ");

  lcd.setCursor(0,1);

  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  lcd.print(now.second(), DEC);
  
  delay(1000);
 
}
