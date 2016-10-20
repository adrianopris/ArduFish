#include <Servo.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

Servo feeder;
byte feederPin = 9; // Feeder is connected at this pin
byte lightPin = 11; // Aquarium light pin
byte ledPin = 13; // Internal LED

// Feeder data
byte feedOFF = 0; // Turn feeder to standby position
byte feedON = 180; // Turn feeder 180 degree to feed fishes
byte feedShake = 90; // Shake feed container in order to dislocate feed
byte feedInMorningHour = 9;
byte feedInEveningHour = 20;
byte morningFeedDone = 0; // Flag to keep morning feeding status
byte eveningFeedDone = 0; // Flag to keep evening feeding status
int countFeeding = 0; // Keep evidence how many times your fishes has eaten.

byte i = 0; // Do not display more than one time the message no fiding until now

// Lighting data
int turnOnLightHours[] = {8, 17};
int turnOffLightHours[] = {13, 21};
byte lightStatus = 0; // OFF or ON
int lightFadingInSeconds = 600; // Turn light ON and OFF with a fade effect (10 minutes)
int lightIntensity = 250; // Max is 255 (8 bits resolution)

void setup() {

  Serial.begin(9600);
  //Serial.print(i);
  if (!rtc.begin()) {

    Serial.print("Couldn't find RTC");
    while (1); // Wait for RTC to be ready
  }
  // Force RTC adjustment
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (!rtc.isrunning()) {
    Serial.print("RTC is NOT running!");
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  pinMode(ledPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
}

void loop() {
  DateTime now = rtc.now();


  blinkLED();
Serial.println("isTimeToSwitchLightsON: ");
Serial.print(isTimeToSwitchLightsON());
  // Turn light ON
  if (in_array(turnOnLightHours, now.hour()) && 0 == lightStatus) {
    //if (0 == lightStatus) {
    Serial.println("Aprinde lumina");
    turnLightON(lightFadingInSeconds);
    Serial.println("Lumina aprinsa");
    lightStatus = 1;
  }
  delay(500);  // For stability

  // Turn light OFF
  if (in_array(turnOffLightHours, now.hour()) && 1 == lightStatus) {
    //if (1 == lightStatus) {
    Serial.println("Stinge lumina");
    turnLightOFF(lightFadingInSeconds);
    Serial.println("Lumina stinsa");
    lightStatus = 0;
  }
  delay(500); // For stability

  // Morning Fish Feed
  if (feedInMorningHour == now.hour() && morningFeedDone != 1) {
    Serial.println("Is morning feeding time. Your fishes are feeded right now!");
    feedNow();
    Serial.println("Fishes has been feeded in morning. They can joy now entire day! Yeey!");
    countFeeding++;
    morningFeedDone = 1; // Assure that feeder feed fishes just once in morning
    eveningFeedDone = 0; // Assure that feeder will feed fishes in evening
  }

  // Evening Fish Feed
  if (feedInEveningHour == now.hour() && eveningFeedDone != 1) {
    Serial.println("Is evening feeding time. Your fishes are feeded right now!");
    feedNow();
    Serial.println("Fishes has been feeded in evening. They are very happy! Yeey!");
    countFeeding++;
    eveningFeedDone = 1; // Assure that feeder feed fishes just once in evening
    morningFeedDone = 0; // Let feeder to feed fishes next morning
  }

  if ((morningFeedDone == 1 || eveningFeedDone == 1) && i < 1) {

    String yourFishesWereFeeded = "Your fishes were feeded ";
    yourFishesWereFeeded.concat(countFeeding);
    yourFishesWereFeeded.concat(" times!");
    Serial.println(yourFishesWereFeeded);

    printNextFeedingTime();

    Serial.println("______________"); // Introduce a break line

  } else if (!countFeeding && i < 1) {
    Serial.println("Your fishes were not feed until now!");

    printNextFeedingTime();

    Serial.println("______________"); // Introduce a break line
  }
  i++; // Stop repeat messages
} // [-] loop()

// Control if system is ON
void blinkLED() {
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);              // wait for a second
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
  delay(2000);
}

void turnLightON(int fadeTimeFrame) {
  int k = 0;
  while (k <= lightIntensity) {
    analogWrite(lightPin, k);
    delay(int((fadeTimeFrame / lightIntensity) * 1000)); // Fading in specified time frame
    k++;
  }
}

void turnLightOFF(int fadeTimeFrame) {
  int k = lightIntensity;
  while (k >= 0) {
    analogWrite(lightPin, k);
    delay(int((fadeTimeFrame / lightIntensity) * 1000)); // Fading in specified time frame
    k--;
  }
}

void feedNow() {
  feeder.attach(feederPin);
  delay(10);

  byte feederPos = feedOFF;
  // Go to feeding position
  while (feederPos < feedON) {
    feeder.write(feederPos);
    delay(15);
    feederPos++;
  }

  // Shacke a bit the food recipient
  if (feederPos == feedON) {
    feederShake(feederPos, 3);
  }

  // Go back to standby
  while (feederPos > feedOFF) {
    feeder.write(feederPos);
    delay(15);
    feederPos--;
  }

  feeder.detach(); // Release feeder engine
  i = 0; // Reset counter in order to update Serial info
}

void feederShake(byte feederPos, byte shakeTimes) {
  byte n = 0;
  while (n < shakeTimes) {
    feeder.write(feederPos - feedShake);
    delay(500);
    feeder.write(feederPos); // Previous position (max)
    delay(500);
    n++;
  }
}

void printNextFeedingTime() {
  DateTime now = rtc.now();

  // Print hours left until feeding
  String hoursLeft = "Next feed will be over ";
  if (now.hour() < feedInMorningHour) {
    hoursLeft.concat(feedInMorningHour - now.hour());
  } else {
    hoursLeft.concat(feedInEveningHour - now.hour());
  }
  hoursLeft.concat(" hours.");

  Serial.println(hoursLeft);
}

boolean isTimeToSwitchLightsON() {
  DateTime now = rtc.now();

  for (int k = 0; k <= sizeof(turnOnLightHours) / sizeof(int); k++) {
    for (int j = 0; j <= sizeof(turnOffLightHours) / sizeof(int); j++) {
      if (turnOnLightHours[k] <= now.hour() && turnOffLightHours[k] > now.hour()) {
        return true;
      }
    }
  }

  // Light must be OFF
  return false;
}

/// Helpers
boolean in_array(int array[], int element) {
  for (int k = 0; k <= sizeof(array) / sizeof(int); k++) {

    if (array[k] == element) {
      return true;
    }
  }
  return false;
}

