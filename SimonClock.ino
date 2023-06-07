/**
 * Code for Simon Alarm Clock written by Jacob Smith.
 * 
 * Last updated December 2022
 * 
 * Hardware Used:
 * Elegoo/Arduino Uno R3, DS3231 RTC (real-time clock), LCD display (Elegoo LCD1602), 
 * 10K potentiometer, Passive buzzer, Switches (4), LEDs (4)
 * 
 * CONNECTIONS:
 * 
 * RTC
 * Arduino A4 -> RTC SDA
 * Arduino A5 -> RTC SCL
 * 
 * LCD
 * Arduino 2 -> LCD D7
 * Arduino 3 -> LCD D6
 * Arduino 4 -> LCD D5
 * Arduino 5 -> LCD D4
 * Arduino 6 -> LCD E
 * Arduino 7 -> LCD RS
 * LCD R/W -> ground
 * LCD VSS pin -> ground
 * LCD VCC pin -> 5V
 * LCD A -> ground (through 10K resistor)
 * LCD V0 -> Potentiometer
 * Potentiometer -> 5V & ground
 * 
 * LED's, Switches, Buzzer
 * Arduino 9, 10, 11, 12 -> LED 1, 2, 3, 4 (through 10K resistors)
 * Arduino A3, A2, A1, A0 -> Switch 1, 2, 3, 4
 * Arduino 8 -> Buzzer
 * Buzzer -> 5V
 */

/**  
  Credit for example code sources (used as starting point) as follows:

  ~~ SIMON ~~
  Simon Game for Arduino
  Copyright (C) 2016, Uri Shaked
  Released under the MIT License.

  ///////////////////////////

  ~~ LIQUID CRYSTAL DISPLAY HELLO WORLD ~
  http://www.arduino.cc/en/Tutorial/LiquidCrystalHelloWorld

  ///////////////////////////

  ~~ ALARM CLOCK ~~
  Arduino DS3232RTC Library
  https://github.com/JChristensen/DS3232RTC
  Copyright (C) 2018 by Jack Christensen and licensed under
  GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

 */


// ========================================
/* INCLUDES */
// ========================================

#include <LiquidCrystal.h>
#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/
#include "pitches.h"


// ========================================
/* GLOBALS */
// ========================================


/* OBJECTS */

// define the RTC (realtime clock) component
DS3232RTC rtc;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int LCD_RS = 7, LCD_E = 6, LCD_D4 = 5, LCD_D5 = 4, LCD_D6 = 3, LCD_D7 = 2;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


/* CONSTANTS */

// Define pin numbers for LEDs, buttons and speaker
const uint8_t SPEAKER_PIN = 8;
const byte LED_PINS[] = {12, 11, 10, 9};
const byte BUTTON_PINS[] = {A0, A1, A2, A3};

// Define note sequences
const int GAME_TONES[] = {NOTE_G3, NOTE_C4, NOTE_E4, NOTE_G5};
const int ALARM_NOTES[] = {NOTE_C4, NOTE_F4, NOTE_A5, NOTE_F5, NOTE_C5, NOTE_A5};
const int VICTORY_NOTES[] = {NOTE_E4, NOTE_G4, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_G5};

// Define other reference variables
const unsigned long ALARM_DURATION_MS = 3000;
const uint8_t BUTTON_COUNT = 4;
const uint8_t MAX_GAME_ROUNDS = 5;
const uint8_t DEFAULT_MODE = 0;
const uint8_t ALARM_MODE = 5;


/* GAME STATE */

byte gameSequence[MAX_GAME_ROUNDS] = {0};
byte gameIndex = 0;

boolean ringing = false;
boolean awake = false;

uint8_t clockMode;
bool pressedButtons[BUTTON_COUNT] = {false};


// ========================================
/* SETUP AND MAIN LOOP */
// ========================================

/**
   Set up the Arduino board and initialize Serial communication
*/
void setup()
{
  Serial.begin(9600);

  // SET UP DISPLAY
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // SET UP CLOCK
  rtc.begin();
  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  rtc.setAlarm(DS3232RTC::ALM1_MATCH_DATE, 0, 0, 0, 1);
  rtc.setAlarm(DS3232RTC::ALM2_MATCH_DATE, 0, 0, 0, 1);
  rtc.alarm(DS3232RTC::ALARM_1);
  rtc.alarm(DS3232RTC::ALARM_2);
  rtc.alarmInterrupt(DS3232RTC::ALARM_1, false);
  rtc.alarmInterrupt(DS3232RTC::ALARM_2, false);
  rtc.squareWave(DS3232RTC::SQWAVE_NONE);

  setSyncProvider(rtc.get);   // the function to get the time from the RTC

  if(timeStatus() != timeSet)
    while(true)
      Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");

  // set the RTC time and date to the compile time
  rtc.set(compileTime());

  // set Alarm 1 to occur at 5 seconds after every minute
  rtc.setAlarm(DS3232RTC::ALM1_MATCH_SECONDS, 5, 0, 0, 1);
  // clear the alarm flag
  rtc.alarm(DS3232RTC::ALARM_1);

  // SET UP SIMON GAME
  for (byte i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  pinMode(SPEAKER_PIN, OUTPUT);
  // The following line primes the random number generator.
  // It assumes pin A0 is floating (disconnected):
  // randomSeed(analogRead(A0));
  randomSeed(rtc.get());
}


/**
   The main loop
*/
void loop()
{

  if (millis() % 100 == 0 && rtc.alarm(DS3232RTC::ALARM_1)) 
    clockMode = ALARM_MODE;

  // check if mode should switch if alarm isn't triggered
  if (clockMode != ALARM_MODE)
  {
    uint8_t buttonPress = getButtonPress();
    if (buttonPress)
    {
      clockMode = buttonPress;
    }
  }

  switch (clockMode)
  {
  case ALARM_MODE:
    lcd.setCursor(0,0);
    lcd.print("ALARM RINGING");
    handleAlarm();
    clockMode = DEFAULT_MODE;
    break;

  case DEFAULT_MODE:
    lcd.setCursor(0,0);
    lcd.print("Simon is cool");
    printDateTime();
    break;
  }

  // delay(100);                  // no need to bombard the RTC continuously
}


// ========================================
/* ALARM FUNCTIONS */
// ========================================

void handleAlarm()
{
  unsigned long timeoutMillis = millis() + ALARM_DURATION_MS;

  uint8_t button;
  while (millis() < timeoutMillis)
  {
    button = playAlarmUntilInterrupted();
    delay(500);
    if (button && playSimon()) return;
  }
}

uint8_t playAlarmUntilInterrupted()
{
  int notesCount = 6;
  uint8_t button;
  for (uint8_t i = 0; i < notesCount; i++)
  {
    tone(SPEAKER_PIN, ALARM_NOTES[i]);
    delay(150);
    noTone(SPEAKER_PIN);
    button = getButtonPress();
    if (button) return button;
  }
  return 0;
}


// ========================================
/* SIMON FUNCTIONS */
// ========================================

/**
 * @brief Plays the Simon game and returns whether the player won
 * 
 * @return true - win
 * @return false - lose
 */
bool playSimon()
{
  for (uint8_t round = 0; round < MAX_GAME_ROUNDS; round++)
  {
    // Add a random color to the end of the sequence
    gameSequence[gameIndex] = random(0, 4);
    gameIndex++;

    // Play current sequence
    for (int i = 0; i < gameIndex; i++) 
    {
      byte currentLed = gameSequence[i];
      lightLedAndPlayTone(currentLed);
      delay(50);
    }
    if (!checkUserSequenceUntilTimeout()) {
      Serial.print("Game over! your score: ");
      Serial.println(gameIndex - 1);
      gameIndex = 0;
      delay(200);
      playSadNotes();
      return false;
    }

    delay(300);

    // if (gameIndex > 0) {
    //   playVictorySound();
    //   delay(300);
    // }
  }
  playVictoryNotes();
  ringing = false;
  awake = false;
  gameIndex = 0;

  return true;
}

/**
 * @brief Get the user's input and compare it with the expected sequence.
 */
bool checkUserSequenceUntilTimeout()
{
  for (int i = 0; i < gameIndex; i++) {
    uint8_t expectedButton = gameSequence[i];
    uint8_t actualButton;

    unsigned long timeoutMillis = millis() + 3000;

    while (millis() < timeoutMillis) 
    {
      actualButton = getButtonPress();
      if (actualButton)
      {
        actualButton--;
        break;
      }
      else actualButton = -1;
    }

    lightLedAndPlayTone(actualButton);

    if (expectedButton != actualButton) {
      return false;
    }
  }

  return true;
}


// ========================================
/* IO FUNCTIONS */
// ========================================

/**
 * @brief Returns the number (not index) 
 * of the button being pressed, or 0 if none are pressed. 
 * 
 * If multiple are pressed, the lowest numbered one will be returned.
 */
uint8_t getButtonPress()
{
  for (uint8_t i = 0; i < 4; i++) {
    byte buttonPin = BUTTON_PINS[i];
    if (digitalRead(buttonPin) == LOW) {
      return i+1;
    }
  }
  return 0;
}

/**
 * @brief Lights the given LED and plays a suitable tone
 * 
 * @param ledIndex - index of the LED to be lit
 */
void lightLedAndPlayTone(byte ledIndex)
{
  digitalWrite(LED_PINS[ledIndex], HIGH);
  tone(SPEAKER_PIN, GAME_TONES[ledIndex]);
  delay(300);
  digitalWrite(LED_PINS[ledIndex], LOW);
  noTone(SPEAKER_PIN);
}

/**
 * @brief Plays a victorious sound
 */
void playVictoryNotes()
{
  int notesCount = 6;

  for (int i = 0; i < notesCount; i++)
  {
    tone(SPEAKER_PIN, VICTORY_NOTES[i]);
    delay(150);
    noTone(SPEAKER_PIN); 
  }
}

/**
 * @brief Plays sad notes with a trill at the end
 */
void playSadNotes()
{
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
}


// ========================================
/* TIME FUNCTIONS */
// ========================================

/**
 * @brief Prints date and time using Serial object and << operators
 * 
 * @param t time_t object representing the time to print
 */
void serialPrintDateTime(time_t t)
{
  Serial << ((day(t) < 10) ? "0" : "") << _DEC(day(t));
  Serial << monthShortStr(month(t)) << _DEC(year(t)) << ' ';
  Serial << ((hour(t) < 10) ? "0" : "") << _DEC(hour(t)) << ':';
  Serial << ((minute(t) < 10) ? "0" : "") << _DEC(minute(t)) << ':';
  Serial << ((second(t) < 10) ? "0" : "") << _DEC(second(t));
  Serial << endl;
}

/**
 * @brief Prints date and time to the lcd screen second row
 */
void printDateTime()
{
  time_t t = rtc.get();
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

/**
 * @brief Function to return the compile date and time as a time_t value
 */
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
