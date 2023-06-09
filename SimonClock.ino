/**
 * Code for Simon Alarm Clock written by Jacob Smith.
 * 
 * Last updated June 2023
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
const byte SPEAKER_PIN = 8;
const byte LED_PINS[] = {12, 11, 10, 9};
const byte BUTTON_PINS[] = {A0, A1, A2, A3};

// Define button numbers
const byte BUTTON_1 = 1;
const byte BUTTON_2 = 2;
const byte BUTTON_3 = 3;
const byte BUTTON_4 = 4;

// Define clock modes
const byte MODE_DEFAULT = 0;
const byte MODE_SET_ALARM = 1;
const byte MODE_SET_CLOCK = 2;
const byte MODE_PLAY_SIMON = 3;
const byte MODE_ADJUST_SETTINGS = 4;
const byte MODE_ALARM = 5;

// Define note sequences
const uint16_t GAME_TONES[] = {NOTE_G3, NOTE_C4, NOTE_E4, NOTE_G4};
const uint16_t ALARM_NOTES[] = {NOTE_C4, NOTE_F4, NOTE_A5, NOTE_F5, NOTE_C5, NOTE_A5};
const uint16_t VICTORY_NOTES[] = {NOTE_E4, NOTE_G4, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_G5};

// Define other reference variables
const unsigned long ALARM_DURATION_MS = 15000;
const byte BUTTON_COUNT = 4;
const byte MAX_GAME_ROUNDS = 16;


/* ALARM STATE */

/* 
  These arrays store information about the alarm hours, minutes, seconds, and on/off status (in that order)
  hours - alarm hour
  minutes - alarm minute
  seconds - alarm second
  on/off - whether alarm is on or off (0 or 1)
*/
byte alarmSettings[4] = {8, 0, 0, 1}; // numbers representing the current alarm settings
const char* alarmSettingNames[4] = {"hr ", "min", "sec", ""}; //  display names for each alarm setting
byte alarmSettingMaxes[4] = {23, 59, 59, 1}; // max value for each setting

byte alarmSettingIndex = 0; // stores the index to use with the above arrays

// number of rounds to play to dismiss alarm
byte simonAlarmRounds = 7;


/* GAME STATE */

byte gameSequence[MAX_GAME_ROUNDS] = {0};
byte gameIndex = 0;

boolean ringing = false;
boolean awake = false;

byte clockMode;
bool pressedButtons[BUTTON_COUNT] = {false};


/* FUNCTION DECLARATIONS */

// these functions need declarations because they have default parameters
byte incrementWithWrap(byte number, byte max, byte amount = 1);
void printDateTime(short day, short month, short hour, short minute, short second = -1);
bool playSimon(byte maxRounds = MAX_GAME_ROUNDS);


// ========================================
/* SETUP AND MAIN LOOP */
// ========================================

/**
   Set up the Arduino board and initialize Serial communication
*/
void setup()
{
  Serial.begin(9600);


  /* SET UP DISPLAY */

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);


  /* SET UP CLOCK */
  
  rtc.begin();
  setSyncProvider(rtc.get);   // the function to get the time from the RTC

  if(timeStatus() != timeSet)
    while(true)
      Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");

  // set the RTC time and date to the compile time
  time_t compileTime = getCompileTime();
  rtc.set(compileTime);


  /* SET UP ALARM */

  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  rtc.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, 0, 0, 0, 1);
  rtc.setAlarm(DS3232RTC::ALM2_MATCH_DATE, 0, 0, 0, 1);
  rtc.alarm(DS3232RTC::ALARM_1);
  rtc.alarm(DS3232RTC::ALARM_2);
  rtc.alarmInterrupt(DS3232RTC::ALARM_1, false);
  rtc.alarmInterrupt(DS3232RTC::ALARM_2, false);
  rtc.squareWave(DS3232RTC::SQWAVE_NONE);

  // set alarm time to 10 seconds after compile time
  alarmSettings[0] = (compileTime / 3600) % 24;
  alarmSettings[1] = (compileTime / 60) % 60;
  alarmSettings[2] = incrementWithWrap(compileTime % 60, 60, 10);
  updateAlarmTime();


  /* SET UP SIMON GAME */

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

  if (alarmSettings[3] && rtc.alarm(DS3232RTC::ALARM_1)) 
  {
    clockMode = MODE_ALARM;
  }

  byte buttonPress = getButtonPress();

  switch (clockMode)
  {
  case MODE_ALARM:
    lcd.setCursor(0,0);
    lcd.print("MODE: RING");
    handleAlarm();
    clockMode = MODE_DEFAULT;
    break;

  case MODE_SET_ALARM:
    handleSetAlarm(buttonPress);
    break;

  case MODE_SET_CLOCK:
    handleSetClock();
    break;

  case MODE_PLAY_SIMON:
    lcd.setCursor(0,0);
    lcd.print("SIMON");
    playSimon();
    clockMode = MODE_DEFAULT;
    break;

  case MODE_ADJUST_SETTINGS:
    handleAdjustSettings();
    break;

  case MODE_DEFAULT:
    // check if clock mode should change
    if (buttonPress) { clockMode = buttonPress; }
    lcd.setCursor(0,0);
    printDateTime(rtc.get());
    lcd.setCursor(0,1);
    lcd.print("ALM CLK PLAY ADJ");
    break;
  }

  delay(200);
}

// ========================================
/* MENU FUNCTIONS */
// ========================================

void handleAdjustSettings()
{
  // TODO
  clockMode = MODE_DEFAULT;
}

void handleSetClock()
{
  // TODO
  clockMode = MODE_DEFAULT;
}

void handleSetAlarm(byte buttonPress)
{
  lcd.setCursor(0,0);
  lcd.print("SET ALARM       ");
  lcd.setCursor(12,0);
  lcd.print(alarmSettingNames[alarmSettingIndex]);

  // todo use LiquidMenu
  lcd.setCursor(0,1);
  lcd.print(alarmSettingIndex == 0 ? ">" : " ");
  lcd.print(alarmSettings[0]);
  if (alarmSettingIndex == 0) lcd.print("<");
  else lcd.print(alarmSettingIndex == 1 ? ">" : ":");
  lcd.print(alarmSettings[1]);
  if (alarmSettingIndex == 1) lcd.print("<");
  else lcd.print(alarmSettingIndex == 2 ? ">" : ":");
  lcd.print(alarmSettings[2]);
  lcd.print(alarmSettingIndex == 2 ? "<" : " ");
  lcd.print("       ");
  lcd.setCursor(11,1);
  lcd.print(alarmSettingIndex == 3 ? ">" : " ");
  lcd.print(alarmSettings[3] ? "ON" : "OFF");
  if (alarmSettingIndex == 3) lcd.print("<");

  switch (buttonPress)
  {
  case BUTTON_1:
    // exit the menu
    clockMode = MODE_DEFAULT;
    break;

  case BUTTON_2:
    alarmSettingIndex = incrementWithWrap(alarmSettingIndex, 3);
    break;

  case BUTTON_3:
    alarmSettings[alarmSettingIndex] = decrementWithWraparound(alarmSettings[alarmSettingIndex], alarmSettingMaxes[alarmSettingIndex]);
    updateAlarmTime();
    break;

  case BUTTON_4:
    alarmSettings[alarmSettingIndex] = incrementWithWrap(alarmSettings[alarmSettingIndex], alarmSettingMaxes[alarmSettingIndex]);
    updateAlarmTime();
    break;
  
  default:
    break;
  }

}

// ========================================
/* ALARM / TIME FUNCTIONS */
// ========================================

/**
 * @brief Updates alarm time based on the global alarmSettings variable
 */
void updateAlarmTime()
{
  rtc.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, alarmSettings[2], alarmSettings[1], alarmSettings[0], 0);
  rtc.alarm(DS3232RTC::ALARM_1);
}

/**
 * @brief Handles playing the alarm until it is stopped or timeout occurs
 */
void handleAlarm()
{
  unsigned long timeoutMillis = millis() + ALARM_DURATION_MS;

  byte button;
  while (millis() < timeoutMillis)
  {
    button = playAlarmUntilInterrupted();
    delay(500);
    if (button && playSimon(simonAlarmRounds)) return;
  }
}

/**
 * @brief Plays an alarm sequence until timeout occurs or a button is pressed
 * 
 * @return byte 
 */
byte playAlarmUntilInterrupted()
{
  int notesCount = sizeof(ALARM_NOTES) / sizeof(uint16_t);
  byte button;
  for (byte i = 0; i < notesCount; i++)
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
bool playSimon(byte maxRounds = MAX_GAME_ROUNDS)
{
  for (byte round = 0; round < maxRounds; round++)
  {
    lcd.setCursor(0,0);
    for (char i = 0; i < gameIndex; i++) lcd.print("$");
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
    byte expectedButton = gameSequence[i];
    byte actualButton;

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
byte getButtonPress()
{
  for (byte i = 0; i < 4; i++) {
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
  tone(SPEAKER_PIN, NOTE_DS4);
  delay(300);
  tone(SPEAKER_PIN, NOTE_D4);
  delay(300);
  tone(SPEAKER_PIN, NOTE_CS4);
  delay(300);
  for (byte i = 0; i < 10; i++) {
    for (int pitch = -10; pitch <= 10; pitch++) {
      tone(SPEAKER_PIN, NOTE_C4 + pitch);
      delay(5);
    }
  }
  noTone(SPEAKER_PIN);
}

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
 * @brief Prints date and time given a time_t variable
 */
void printDateTime(time_t t)
{
  printDateTime(day(t), month(t), hour(t), minute(t), second(t));
}

/**
 * @brief Prints date and time given each part of the date/time as a byte
 * 
 * @param day number representing day of the month
 * @param month number representing the month
 * @param hour number representing hours
 * @param minute number representing minutes
 * @param second number representing seconds (if negative or not passed, it will not be printed)
 */
void printDateTime(short day, short month, short hour, short minute, short second=-1)
{
  lcd.print(day < 10 ? "0" : "");
  lcd.print(day);
  lcd.print(" ");

  lcd.print(monthShortStr(month));
  lcd.print(" ");

  lcd.print((hour < 10) ? "0" : "");
  lcd.print(hour);
  lcd.print(":");

  lcd.print((minute < 10) ? "0" : "");
  lcd.print(minute);
  lcd.print(":");

  if (second > 0)
  {
    lcd.print((second < 10) ? "0" : "");
    lcd.print(second);
  }
}


// ========================================
/* UTILITY FUNCTIONS */
// ========================================

/**
 * @brief Increments an unsigned number by 1 or a specified amount, wrapping after a specified max
 * 
 * @param number number to increment
 * @param max maximum value before wrapping on increment
 * @param amount (optional) amount by which to increment (default is 1)
 * @return byte the incremented number
 */
byte incrementWithWrap(byte number, byte max, byte amount)
{
  if (number == max) return 0;
  else return number + amount;
}

/**
 * @brief Decrements an unsigned number by 1, wrapping up to max after 0
 * 
 * @param number number to decrement
 * @param max maximum value for the number
 * @return byte the decremented number
 */
byte decrementWithWraparound(byte number, byte max)
{
  if (number == 0) return max;
  else return number - 1;
}

/**
 * @brief Function to return the compile date and time as a time_t value
 */
time_t getCompileTime()
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
