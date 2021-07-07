// Arduino DS3232RTC Library
// https://github.com/JChristensen/DS3232RTC
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
// DS3231/DS3232 Alarm Example Sketch #1
//
// Set Alarm 1 to occur once a minute at 5 seconds after the minute.
// Detect the alarm by polling the RTC alarm flag.
/*
  LiquidCrystal Library - Hello World

  Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
  library works with all LCD displays that are compatible with the
  Hitachi HD44780 driver. There are many of them out there, and you
  can usually tell them by the 16-pin interface.

  This sketch prints "Hello World!" to the LCD
  and shows the time.

  The circuit:
   LCD RS pin to digital pin 12
   LCD Enable pin to digital pin 11
   LCD D4 pin to digital pin 5
   LCD D5 pin to digital pin 4
   LCD D6 pin to digital pin 3
   LCD D7 pin to digital pin 2
   LCD R/W pin to ground
   LCD VSS pin to ground
   LCD VCC pin to 5V
   10K resistor:
   ends to +5V and ground
   wiper to LCD VO pin (pin 3)

  Library originally added 18 Apr 2008
  by David A. Mellis
  library modified 5 Jul 2009
  by Limor Fried (http://www.ladyada.net)
  example added 9 Jul 2009
  by Tom Igoe
  modified 22 Nov 2010
  by Tom Igoe
  modified 7 Nov 2016
  by Arturo Guadalupi

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/LiquidCrystalHelloWorld

*/

// include the library code:
#include <LiquidCrystal.h>
#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//
// Hardware:
// Arduino Uno, DS3231 RTC.
// Connect RTC SDA to Arduino pin A4.
// Connect RTC SCL to Arduino pin A5.
//
// Jack Christensen 16Sep2017

#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/

/**
   Simon Game for Arduino

   Copyright (C) 2016, Uri Shaked

   Released under the MIT License.
*/
#include "pitches.h"

/* Constants - define pin numbers for LEDs,
   buttons and speaker, and also the game tones: */
const byte ledPins[] = {12, 11, 10, 9};
const byte buttonPins[] = {A0, A1, A2, A3};
#define SPEAKER_PIN 8

#define MAX_GAME_LENGTH 5

const int gameTones[] = { NOTE_G3, NOTE_C4, NOTE_E4, NOTE_G5};

/* Global variables - store the game state */
byte gameSequence[MAX_GAME_LENGTH] = {0};
byte gameIndex = 0;

boolean ringing = false;
boolean awake = false;

/**
   Set up the Arduino board and initialize Serial communication
*/
void setup()
{
  Serial.begin(9600);
  // SET UP CLOCK
  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);

  // set the RTC time and date to the compile time
  RTC.set(compileTime());

  // set Alarm 1 to occur at 5 seconds after every minute
  RTC.setAlarm(ALM1_MATCH_SECONDS, 5, 0, 0, 1);
  // clear the alarm flag
  RTC.alarm(ALARM_1);

  Serial << millis() << " Start ";
  printDateTime(RTC.get());
  Serial << endl;

  // SET UP DISPLAY
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Simon Alarm Clk");

  // SET UP SIMON GAME
  for (byte i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(SPEAKER_PIN, OUTPUT);
  // The following line primes the random number generator.
  // It assumes pin A0 is floating (disconnected):
  randomSeed(analogRead(A0));
}

/**
   Lights the given LED and plays a suitable tone
*/
void lightLedAndPlayTone(byte ledIndex)
{
  digitalWrite(ledPins[ledIndex], HIGH);
  tone(SPEAKER_PIN, gameTones[ledIndex]);
  delay(300);
  digitalWrite(ledPins[ledIndex], LOW);
  noTone(SPEAKER_PIN);
}

/**
   Plays the current sequence of notes that the user has to repeat
*/
void playSequence()
{
  for (int i = 0; i < gameIndex; i++) {
    byte currentLed = gameSequence[i];
    lightLedAndPlayTone(currentLed);
    delay(50);
  }
}

/**
    Waits until the user pressed one of the buttons,
    and returns the index of that button
*/
byte readButtons()
{
  while (true) {
    for (byte i = 0; i < 4; i++) {
      byte buttonPin = buttonPins[i];
      if (digitalRead(buttonPin) == LOW) {
        return i;
      }
    }
    delay(1);
  }
}

boolean checkForButtonPress()
{
  for (byte i = 0; i < 4; i++) {
    byte buttonPin = buttonPins[i];
    if (digitalRead(buttonPin) == LOW) {
      return true;
    }
  }
  return false;
}

/**
  Play the game over sequence, and report the game score
*/
void gameOver() {
  Serial.print("Game over! your score: ");
  Serial.println(gameIndex - 1);
  gameIndex = 0;
  delay(200);

  // Play a Wah-Wah-Wah-Wah sound
  tone(SPEAKER_PIN, NOTE_DS5);
  delay(300);
  tone(SPEAKER_PIN, NOTE_D5);
  delay(300);
  tone(SPEAKER_PIN, NOTE_CS5);
  delay(300);
  for (byte i = 0; i < 10; i++) {
    for (int pitch = -10; pitch <= 10; pitch++) {
      tone(SPEAKER_PIN, NOTE_C5 + pitch);
      delay(5);
    }
  }
  noTone(SPEAKER_PIN);
  delay(500);
}

/**
   Get the user's input and compare it with the expected sequence.
*/
bool checkUserSequence()
{
  for (int i = 0; i < gameIndex; i++) {
    byte expectedButton = gameSequence[i];
    byte actualButton = readButtons();
    lightLedAndPlayTone(actualButton);
    if (expectedButton != actualButton) {
      return false;
    }
  }

  return true;
}

/**
   Plays a hooray sound whenever the user finishes a level
*/
void playVictorySound()
{
  tone(SPEAKER_PIN, NOTE_E4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_E5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_C5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_D5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G5);
  delay(150);
  noTone(SPEAKER_PIN);
}

void playAlarm()
{
  tone(SPEAKER_PIN, NOTE_C4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_F4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_A5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_F5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_C5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_A5);
  delay(150);
  noTone(SPEAKER_PIN);
}

void serialPrintDateTime(time_t t)
{
  Serial << ((day(t) < 10) ? "0" : "") << _DEC(day(t));
  Serial << monthShortStr(month(t)) << _DEC(year(t)) << ' ';
  Serial << ((hour(t) < 10) ? "0" : "") << _DEC(hour(t)) << ':';
  Serial << ((minute(t) < 10) ? "0" : "") << _DEC(minute(t)) << ':';
  Serial << ((second(t) < 10) ? "0" : "") << _DEC(second(t));
  Serial << endl;
}

void printDateTime(time_t t)
{
  // set the cursor to column 0, row 1
  // (note: row 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print((day(t) < 10) ? "0" : "");
  lcd.print(day(t));
  lcd.print(" ");

  lcd.print(monthShortStr(month(t)));
  lcd.print(" ");

  lcd.print((hour(t) < 10) ? "0" : "");
  lcd.print(hour(t));
  lcd.print(":");

  lcd.print((minute(t) < 10) ? "0" : "");
  lcd.print(minute(t));
  lcd.print(":");

  lcd.print((second(t) < 10) ? "0" : "");
  lcd.print(second(t));
}


// function to return the compile date and time as a time_t value
time_t compileTime()
{
  const time_t FUDGE(10);    //fudge factor to allow for upload time, etc. (seconds, YMMV)
  const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char compMon[4], *m;

  strncpy(compMon, compDate, 3);
  compMon[3] = '\0';
  m = strstr(months, compMon);

  tmElements_t tm;
  tm.Month = ((m - months) / 3 + 1);
  tm.Day = atoi(compDate + 4);
  tm.Year = atoi(compDate + 7) - 1970;
  tm.Hour = atoi(compTime);
  tm.Minute = atoi(compTime + 3);
  tm.Second = atoi(compTime + 6);

  time_t t = makeTime(tm);
  return t + FUDGE;        //add fudge factor to allow for compile time
}

void playSimon()
{
  // Add a random color to the end of the sequence
  gameSequence[gameIndex] = random(0, 4);
  gameIndex++;
  if (gameIndex >= MAX_GAME_LENGTH) {
    playVictorySound();
    ringing = false;
    awake = false;
    gameIndex = 0;
  }

  playSequence();
  if (!checkUserSequence()) {
    gameOver();
  }

  delay(300);

  // if (gameIndex > 0) {
  //   playVictorySound();
  //   delay(300);
  // }

}

/**
   The main loop
*/
void loop()
{
  lcd.setCursor(0, 0);
  lcd.print("Simon Alarm Clk");
  printDateTime(RTC.get());
  if ( RTC.alarm(ALARM_1) )    // check alarm flag, clear it if set
  {
    lcd.setCursor(0, 0);
    lcd.print("ALARM_1");
    ringing = true;
  }
  if ( RTC.alarm(ALARM_2) )    // check alarm flag, clear it if set
  {
    lcd.setCursor(0, 0);
    lcd.print("ALARM_2");
    ringing = true;
  }

  if (checkForButtonPress())
  {
    awake = true;
  }

  if (ringing && !awake)
  {
    playAlarm();
  }

  if (ringing && awake)
  {
    playSimon();
  }

  delay(100);                  // no need to bombard the RTC continuously
}