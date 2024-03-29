#include "Adafruit_seesaw.h"
#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS3231.h>
#include <EEPROM.h>

#define LEFTBUTTON 8
#define MENUBUTTON 9
#define RIGHTBUTTON 10
#define VALVE 7
#define WATERLEVEL A0
#define WATERLED 6
#define BUZZER 13

RTClib myRTC;

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Adafruit_seesaw ss;

uint16_t threshold = 500;

int menuPage = 0;
unsigned long previousUpdate = 0;
unsigned long previousWaterLow = 0;
unsigned char mode = 0;
unsigned char timeIntervalCounter = 0;
unsigned long nextWaterTime = 0;
unsigned long lastWatering = 0;
unsigned char numReadsBelowThresh = 0;
bool isInMenu = false;

void storeSettings(){
  unsigned char lastMode;
  EEPROM.get(0, lastMode); // Get the previous
  if(lastMode != mode) EEPROM.put(0, mode); // Only store if different

  uint16_t lastThresh;
  EEPROM.get(1, lastThresh);
  if(lastThresh != threshold) EEPROM.put(1, threshold);

  unsigned char lastInterval;
  EEPROM.get(3, lastInterval);
  if(lastInterval != timeIntervalCounter) EEPROM.put(3, timeIntervalCounter);

}

void printLowWaterLevel(){

  if((millis() - previousWaterLow) > 1000){

    previousWaterLow = millis();

    lcd.setCursor(0, 0);

    lcd.print("Water Level Low ");

    lcd.setCursor(0, 1);

    lcd.print("Replace Water!  ");

  }

}

void checkWaterLevel(){

  // Check water level
  int waterLevel = analogRead(WATERLEVEL);
  if(waterLevel > 5){
    if(!isInMenu) printLowWaterLevel(); // Printing while in menu causes display problems, because menu doesn't update every second like other screens.
    digitalWrite(WATERLED, HIGH); // No water
    tone(BUZZER, 1000);
    delay(500);
    digitalWrite(WATERLED, LOW);
    noTone(BUZZER);
    delay(500);

  } else{
    digitalWrite(WATERLED, LOW); // Water present
    noTone(BUZZER);
  }
}

void restoreSettings(){

// It's possible when first starting up Arduino that EEPROM values haven't been written to. They default to 0xff per byte.
  unsigned char lastMode;
  EEPROM.get(0, lastMode); // Get the previous
  if(lastMode != 0xff) mode = lastMode;

  uint16_t lastThresh;
  EEPROM.get(1, lastThresh);
  if(lastThresh != 0xffff) threshold = lastThresh;

  unsigned char lastInterval;
  EEPROM.get(3, lastInterval);
  if(lastInterval != 0xff) timeIntervalCounter = lastInterval;

}

void logWatering(){

  DateTime now = myRTC.now();
  
  uint8_t month = now.month();
  uint8_t day = now.day();
  uint16_t year = now.year();
  uint8_t hour = now.hour();
  uint8_t minute = now.minute();
  uint8_t second = now.second();

  uint8_t lastMonth;
  EEPROM.get(4, lastMonth);
  uint8_t lastDay;
  EEPROM.get(5, lastDay);
  uint16_t lastYear;
  EEPROM.get(6, lastYear);
  uint8_t lastHour;
  EEPROM.get(8, lastHour);
  uint8_t lastMinute;
  EEPROM.get(9, lastMinute);
  uint8_t lastSecond;
  EEPROM.get(10, lastSecond);

  if(month != lastMonth) EEPROM.put(4, month);
  if(day != lastDay) EEPROM.put(5, day);
  if(year != lastYear) EEPROM.put(6, year);
  if(hour != lastHour) EEPROM.put(8, hour);
  if(minute != lastMinute) EEPROM.put(9, minute);
  if(second != lastSecond) EEPROM.put(10, second);
  

}

bool waterPlant(){

  if(millis() - lastWatering > 10000){ // Have at least 10 seconds in between waterings

    lcd.clear();

    lcd.print("Watering...");

    digitalWrite(VALVE, LOW);
    digitalWrite(VALVE, HIGH);
    delay(3000); // Valve needs at least around 3 seconds to give a minimum watering time.
    digitalWrite(VALVE, LOW);

    nextWaterTime = millis() + timeIntervalToMillis(); // timeIntervalToMillis() gives the amount of additional time to water next.
    lastWatering = millis();

    lcd.clear();

    return true;
  }

  return false;

}

String millisToString(unsigned long time){
// Function takes in an amount of time in milliseconds and returns 
  
  String hours;
  unsigned int numHours = time / 3600000;
  hours = String(numHours);
  if (numHours < 10) hours = "0" + hours;
  time = time - (numHours * 3600000); // Get remaining amount of time to calculate minutes

  String minutes;
  unsigned int numMinutes = time / 60000;
  minutes = String(numMinutes);
  if (numMinutes < 10) minutes = "0" + minutes;
  time = time - (numMinutes * 60000); // Get remaining amount of time to calculate number of seconds.

  String seconds;
  unsigned int numSeconds = time / 1000;
  seconds = String(numSeconds);
  if (numSeconds < 10) seconds = "0" + seconds;

  return hours + ":" + minutes + ":" + seconds;

}

void printLastWatering(){

  uint8_t lastMonth;
  EEPROM.get(4, lastMonth);
  uint8_t lastDay;
  EEPROM.get(5, lastDay);
  uint16_t lastYear;
  EEPROM.get(6, lastYear);
  uint8_t lastHour;
  EEPROM.get(8, lastHour);
  uint8_t lastMinute;
  EEPROM.get(9, lastMinute);
  uint8_t lastSecond;
  EEPROM.get(10, lastSecond);

  lcd.setCursor(0, 0);

  if(lastMonth == 0xff || lastDay == 0xff || lastYear == 0xffff || lastHour == 0xff || lastMinute == 0xff || lastSecond == 0xff){
    lcd.print("Never Watered   ");
  } else{

    if(lastMonth < 10) lcd.print('0'); // Date formats should have at least 2 digits. Print leading zero if less than 10.
    lcd.print(lastMonth, DEC);
    lcd.print('/');
    if(lastDay < 10) lcd.print('0');
    lcd.print(lastDay, DEC);
    lcd.print('/');
    lcd.print(lastYear, DEC);

    lcd.setCursor(0, 1);
    if(lastHour < 10) lcd.print('0');
    lcd.print(lastHour);
    lcd.print(':');
    if(lastMinute < 10) lcd.print('0');
    lcd.print(lastMinute);
    lcd.print(':');
    if(lastSecond < 10) lcd.print('0');
    lcd.print(lastSecond);
    lcd.print("  (Last)   ");
  }
}

void printMoisture() {

  if (millis() - previousUpdate > 1000) {

    previousUpdate = millis();

    uint16_t capread = ss.touchRead(0);

    lcd.setCursor(0, 0);
    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");

    lcd.print(capread, DEC);
    lcd.print("   ");

    lcd.setCursor(0, 1);
    lcd.print("Threshold: ");
    lcd.print(threshold);
    lcd.print("      ");
  }
}

void printMoistureOff() {

  if (millis() - previousUpdate > 1000) {

    previousUpdate = millis();

    uint16_t capread = ss.touchRead(0);

    lcd.setCursor(0, 0);
    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");

    lcd.print(capread, DEC);
    lcd.print("      ");

    lcd.setCursor(0, 1);
    lcd.print("Watering Off    ");
  }
}

void printMoistureAndTime(){
  if (millis() - previousUpdate > 1000) {

    previousUpdate = millis();

    uint16_t capread = ss.touchRead(0);

    lcd.setCursor(0, 0);
    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");

    lcd.print(capread, DEC);
    lcd.print("      ");

    lcd.setCursor(0, 1);
    lcd.print("Next: ");
    lcd.print(millisToString(nextWaterTime - millis())); // Print time remaining until next watering.
    lcd.print("       ");
    }
}

void printTimeAndDate() {

  // Only update display every 1000 seconds
  if (millis() - previousUpdate > 1000) {

    previousUpdate = millis();

    DateTime now = myRTC.now();

    // Print date
    lcd.setCursor(0, 0);
    if (now.month() < 10) lcd.print('0');
    lcd.print(now.month(), DEC);
    lcd.print('/');
    if (now.day() < 10) lcd.print('0');
    lcd.print(now.day(), DEC);
    lcd.print('/');
    lcd.print(now.year(), DEC);
    lcd.print("       ");

    // Print time
    lcd.setCursor(0, 1);
    if (now.hour() < 10) lcd.print('0');
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    if (now.minute() < 10) lcd.print('0');
    lcd.print(now.minute(), DEC);
    lcd.print(':');
    if (now.second() < 10) lcd.print('0');
    lcd.print(now.second(), DEC);
    lcd.print("       ");
  }
}

void clearSecondLine(){
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void printTimeInterval(){
  
  lcd.setCursor(0, 1);
  
  switch (timeIntervalCounter){
    case 0:
      lcd.print("1 min");
      break;
    case 1:
      lcd.print("15 mins");
      break;
    case 2:
      lcd.print("30 mins");
      break;
    case 3:
      lcd.print("1 hour");
      break;
    case 4:
      lcd.print("2 hours");
      break;
    case 5:
      lcd.print("6 hours");
      break;
    case 6:
      lcd.print("12 hours");
      break;
    case 7:
      lcd.print("1 day");
      break;

  }
}

unsigned long timeIntervalToMillis(){
  
  switch (timeIntervalCounter){
    case 0:
      return 60000; // 1 min
    case 1:
      return 900000; // 15 mins
    case 2:
      return 1800000; // 30 mins
    case 3:
      return 3600000; // 1 hr
    case 4:
      return 7200000; // 2 hr
    case 5:
      return 21600000; // 6 hrs
    case 6:
      return 43200000; // 12 hrs
    case 7:
      return 86400000; // 1 day
    }

  return 0;

}


// void(* resetFunc) (void) = 0; // Function for resetting Arduino. Necessary if next watering time is beyond what an unsigned long and millis() can capture.


void setup() {
  Serial.begin(9600);

  Wire.begin();

  lcd.begin(16, 2);

  if (!ss.begin(0x36)) {
    Serial.println("ERROR! seesaw not found");
    while (1) delay(1);
  }

  pinMode(LEFTBUTTON, INPUT);
  pinMode(MENUBUTTON, INPUT);
  pinMode(RIGHTBUTTON, INPUT);
  pinMode(VALVE, OUTPUT);
  pinMode(WATERLEVEL, INPUT);
  pinMode(WATERLED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  restoreSettings(); // Restore previous settings from EEPROM

  nextWaterTime = millis() + timeIntervalToMillis(); // If restarted in timer mode, we reset the watering timer. This line prevents integer underflow when calculating times.
}

void loop() {


  int leftButton, menuButton, rightButton;

  leftButton = digitalRead(LEFTBUTTON);
  menuButton = digitalRead(MENUBUTTON);
  rightButton = digitalRead(RIGHTBUTTON);

  if (menuPage == 0) {

    if(mode == 0){
      printMoisture();
    } else if (mode == 1) {
      printMoistureAndTime();
    } else if (mode == 2){
      printMoistureOff();
    }  

    if (leftButton == HIGH) {
      // Wait for button release
      while (leftButton == HIGH) {
        leftButton = digitalRead(LEFTBUTTON);
        if(mode == 0){
          printMoisture();
        } else if (mode == 1){
          printMoistureAndTime();
        } else if (mode == 2){
          printMoistureOff();
        }

        checkWaterLevel();
      }

      menuPage = -1; // Go to RTC screen
      if (previousUpdate > 1000) previousUpdate -= 1001;  // Update display right away.
      lcd.clear();
    }

    if (rightButton == HIGH){
      // Wait for button release
      while (rightButton == HIGH){
        rightButton = digitalRead(RIGHTBUTTON);
        if(mode == 0){
          printMoisture();
        } else if (mode == 1){
          printMoistureAndTime();
        } else if (mode == 2){
          printMoistureOff();
        }

        checkWaterLevel();
      }

      menuPage = 1;
      if (previousUpdate > 1000) previousUpdate -= 1001;
      lcd.clear();
    }

  } else if (menuPage == -1) {

    printTimeAndDate();

    if (rightButton == HIGH) {
      // Wait for button release
      while (rightButton == HIGH) {
        rightButton = digitalRead(RIGHTBUTTON);
        printTimeAndDate();
        checkWaterLevel();
      }

      menuPage = 0; // Return to main screen
      if (previousUpdate > 1000) previousUpdate -= 1001;
      lcd.clear();
    }
  } else if (menuPage == 1){

    printLastWatering();

    if(leftButton == HIGH){
      // Wait for button release
      while(leftButton == HIGH){
        leftButton = digitalRead(LEFTBUTTON); // Since last watering is static, we do not need to print new information while waiting on button release.
        printLastWatering();
        checkWaterLevel();
      }

      menuPage = 0;
      if (previousUpdate > 1000) previousUpdate -= 1001;
      lcd.clear();

    }


  }



  // Menu should be accessible everywhere
  if (menuButton == HIGH) {

    isInMenu = true;

    // Wait for button release
    while (menuButton == HIGH) {
      menuButton = digitalRead(MENUBUTTON);
      checkWaterLevel();
    }

    lcd.clear();
    lcd.print("Select Mode:");
    lcd.setCursor(0, 1);
    if(mode == 0){
      lcd.print("Automatic");
    } else if (mode == 1){
        lcd.print("Timer");
    } else if (mode == 2){
        lcd.print("Off");
    }

    bool selectionMade = false;

    while(!selectionMade){

      leftButton = digitalRead(LEFTBUTTON);
      menuButton = digitalRead(MENUBUTTON);
      rightButton = digitalRead(RIGHTBUTTON);

      checkWaterLevel();

      if(leftButton == HIGH){ // Since there are only two options at the moment, button direction doesn't matter.
        // Wait for button release
        while(leftButton == HIGH){ // Wait for both buttons to be released
          leftButton = digitalRead(LEFTBUTTON);
        }

        if(mode == 0){
          mode = 2;
        } else if (mode == 1){
          mode = 0;
        } else if (mode == 2){
          mode = 1;
        }

        lcd.setCursor(0,1);
        lcd.print("                "); // Clear line
        lcd.setCursor(0,1);
        if(mode == 0){
          lcd.print("Automatic");
        } else if (mode == 1){
          lcd.print("Timer");
        } else if (mode == 2){
          lcd.print("Off");
        }

      }

      if(rightButton == HIGH){ // Since there are only two options at the moment, button direction doesn't matter.
        // Wait for button release
        while(rightButton == HIGH){ // Wait for both buttons to be released
          rightButton = digitalRead(RIGHTBUTTON);
          checkWaterLevel();
        }

        if(mode == 0){
          mode = 1;
        } else if (mode == 1){
          mode = 2;
        } else if (mode == 2){
          mode = 0;
        }

        lcd.setCursor(0,1);
        lcd.print("                "); // Clear line
        lcd.setCursor(0,1);
        if(mode == 0){
          lcd.print("Automatic");
        } else if (mode == 1){
          lcd.print("Timer");
        } else if (mode == 2){
          lcd.print("Off");
        }

      }
      

      if(menuButton == HIGH){
        // Wait for button release
        while(menuButton == HIGH){
          menuButton = digitalRead(MENUBUTTON);
          checkWaterLevel();
        }

        lcd.clear();

        if(mode == 0){ // Automatic mode chosen
          lcd.print("Threshold:");
          lcd.setCursor(0, 1);
          lcd.print(threshold);

          while(!selectionMade){

            leftButton = digitalRead(LEFTBUTTON);
            menuButton = digitalRead(MENUBUTTON);
            rightButton = digitalRead(RIGHTBUTTON);

            checkWaterLevel();

            if(leftButton == HIGH){
              // Wait for button release
              while(leftButton == HIGH){
                leftButton = digitalRead(LEFTBUTTON);
              }

              if(threshold >= 310) threshold -= 10;
            }

            if(rightButton == HIGH){
              // Wait for button release
              while(rightButton == HIGH){
                rightButton = digitalRead(RIGHTBUTTON);
                checkWaterLevel();
              }

              if(threshold <= 990) threshold += 10;
            }

            lcd.setCursor(0, 1);
            lcd.print(threshold);
            lcd.print("   ");

            if(menuButton == HIGH){
              // Wait for button release
              while(menuButton == HIGH){
                menuButton = digitalRead(MENUBUTTON);
                checkWaterLevel();
              }
              selectionMade = true; // Selection made. Can now exit menu and return to main display.
              storeSettings(); // Store current settings in the EEPROM in lowest bytes.
              lcd.clear();
            }



          }

        } else if (mode == 1){
          // Timer mode chosen
          lcd.print("Time Interval:");
          printTimeInterval();
          while(!selectionMade){
            
            leftButton = digitalRead(LEFTBUTTON);
            menuButton = digitalRead(MENUBUTTON);
            rightButton = digitalRead(RIGHTBUTTON);

            checkWaterLevel();

            if(leftButton == HIGH){
              // Wait for button release
              while(leftButton == HIGH){
                leftButton = digitalRead(LEFTBUTTON);
                checkWaterLevel();
              }
              if(timeIntervalCounter > 0) timeIntervalCounter--;
              clearSecondLine();
              printTimeInterval();
            }

            if(rightButton == HIGH){
              // Wait for button release
              while(rightButton == HIGH){
                rightButton = digitalRead(RIGHTBUTTON);
                checkWaterLevel();
              }
              if(timeIntervalCounter < 7) timeIntervalCounter++;
              clearSecondLine();
              printTimeInterval();
            }

            if(menuButton == HIGH){
              // Wait for button release
              while(menuButton == HIGH){
                menuButton = digitalRead(MENUBUTTON);
                checkWaterLevel();
              }
              selectionMade = true;
              storeSettings(); // Store current settings in EEPROM.
              nextWaterTime = millis() + timeIntervalToMillis(); // 
              lcd.clear();
            }
          }
        } else if (mode == 2){ // User has selected Off.
          selectionMade = true;
          storeSettings();
          lcd.clear();
        }
      }
    }

    isInMenu = false;
  }

  // Check for watering condition
  if(mode == 0){

    uint16_t capread = ss.touchRead(0); // Get soil moisture
    
    if(capread < threshold){
      numReadsBelowThresh++;
    } else{
      numReadsBelowThresh = 0; // If the reading is above, reset number of consecutive reads.
    }

    if(numReadsBelowThresh > 99){
      if(waterPlant()) logWatering();
    }// At least 100 consecutive reads of below threshold moisture have occured.
    // Note: We use this to avoid errant one-off readings to ensure that sensor is indeed below threshold. 

  } else if (mode == 1){ // Mode is timer
    if(millis() > nextWaterTime) {
      if(waterPlant()) logWatering(); 
    } // Time has exceeded water time. Time to water plant.
  } else {
    // Mode is Off
    // Do nothing if Off.
  }

  checkWaterLevel();

}


