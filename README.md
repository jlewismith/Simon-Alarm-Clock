# Simon Alarm Clock

This is the source code for an alarm clock that forces you to play the Simon game to prove you're awake before it stops ringing.

## Hardware Used
Elegoo/Arduino Uno R3, DS3231 RTC (real-time clock), LCD display (Elegoo LCD1602), 
10K potentiometer, Passive buzzer, Switches (4), LEDs (4)

## Circuit Connections

### RTC
```
Arduino A4 -> RTC SDA
Arduino A5 -> RTC SCL
```

### LCD
```
Arduino 2 -> LCD D7
Arduino 3 -> LCD D6
Arduino 4 -> LCD D5
Arduino 5 -> LCD D4
Arduino 6 -> LCD E
Arduino 7 -> LCD RS
LCD R/W -> ground
LCD VSS pin -> ground
LCD VCC pin -> 5V
LCD A -> ground (through 10K resistor)
LCD V0 -> Potentiometer
Potentiometer -> 5V & ground
```

### LED's, Switches, Buzzer
```
Arduino 9, 10, 11, 12 -> LED 1, 2, 3, 4 (through 10K resistors)
Arduino A3, A2, A1, A0 -> Switch 1, 2, 3, 4
Arduino 8 -> Buzzer
Buzzer -> 5V
```
 
