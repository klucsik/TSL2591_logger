//fénymérő
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#define sensor1 5
#define sensor2 4
#define sensor3 3
#define RTCpin 6
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);




#define led 8 //visszajelző led (IR led amit csak kamerán keresztül lehet látni pl)
#define Vin A0  //ellátás mérése (3.7 Voltos akkuk párhuzamosan kötve az ellátás)
#define pwr 9 //az önlekapcsoló motor pinje, highra kell húzni hogy leállítsa a fénymérőt

//óra
#include <DS3232RTC.h>
#include <Time.h>
 static time_t tLast;
 time_t t;
//SD
#include <SPI.h>
#include <SD.h>

File myFile;




void setup() {
  Serial.begin(9600);
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
  delay(100);
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
  delay(100);
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);


  if (!SD.begin(10)) {
    while (1) {
      Serial.println("SD error");
      digitalWrite(led, HIGH);
      delay(1000);
      digitalWrite(pwr, HIGH);
      Serial.println("shutdown");
      if (SD.begin(10)) return;
    }
  }

  char filename[] = "LUX01_00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (! SD.exists(filename)) {
      //meg nyitjuk a file-t írásra ezekután myFileként fogunk hivatkozni rá.
      myFile = SD.open(filename, FILE_WRITE);
      Serial.println("SDopened:");
      Serial.println(filename);
      digitalWrite(led, LOW);
      break;
    }
  }
  //header megírása a fájlhoz
  myFile.println(F("Year;Month;Day;Hour;Minute;Second;lux1;gain1;timing1;fullrangesnesnor1;IRsensor1;lux2;gain2;timing2;fullrangesnesnor2;IRsensor2;lux3;gain3;timing3;fullrangesnesnor3;IRsensor3;Vin"));


  pinMode(pwr, OUTPUT);
  digitalWrite(pwr, LOW);



  //belső óra beállítás
  digitalWrite(RTCpin, HIGH); //minden i2c eszközt külön kapcsolunk ki be a buszra. (az SDA vonal van vágva)
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) {
    Serial.println(F("RTC err"));  // the function to get the time from the RTC
    digitalWrite(led, HIGH);
    while (1) {
      Serial.println("RTC error");
      digitalWrite(led, HIGH);
      delay(1000);
      if (timeStatus() != timeSet) return;
    }
  }
  digitalWrite(RTCpin, LOW);
}









void loop() {


  myFile.print(year());
  myFile.print(";");
  myFile.print(month());
  myFile.print(";");
  myFile.print(day());
  myFile.print(";");
  myFile.print(hour());
  myFile.print(";");
  myFile.print( minute() );
  myFile.print(";");
  myFile.print( second() );
  myFile.print(";");

  
  
  //fénymérők lekérdezése, kiíratása
  byte g = 0;
  while (g < 3) {

    Serial.print(g);
    Serial.print(";");
    //a megfelelő szenzor aktiválása
    switch (g) {
      case 0:
        digitalWrite(sensor1, HIGH);
        digitalWrite(sensor2, LOW);
        digitalWrite(sensor3, LOW);

        break;
      case 1:
        digitalWrite(sensor1, LOW);
        digitalWrite(sensor2, HIGH);
        digitalWrite(sensor3, LOW);
        break;
      case 2:

        digitalWrite(sensor1, LOW);
        digitalWrite(sensor2, LOW);
        digitalWrite(sensor3, HIGH);
        break;

    }
    delay(10);
    //aktiválás vége
    if (tsl.begin()) {

      configureSensor();

      uint32_t lum = tsl.getFullLuminosity();
      uint16_t ir, full;
      ir = lum >> 16;
      full = lum & 0xFFFF;
      float lux = tsl.calculateLux(full, ir);
      tsl.disable();
      //eredmények kiíratása
      Serial.print(lux, 8);
      Serial.print(";");
      Serial.print(tsl.getGain());
      Serial.print(";");
      Serial.print((tsl.getTiming() + 1) * 100, DEC);
      Serial.print(";");
      Serial.print(full);
      Serial.print(";");
      Serial.print(ir);
      Serial.print(";");
      Serial.println();

      myFile.print(lux, 8);
      myFile.print(";");
      myFile.print(tsl.getGain());
      myFile.print(";");
      myFile.print((tsl.getTiming() + 1) * 100, DEC);
      myFile.print(";");
      myFile.print(full);
      myFile.print(";");
      myFile.print(ir);
      myFile.print(";");



      //  delay(2000); //szervizelni lehessen
    }
    else { //ha nem indul ell a szezor


      Serial.print("err");
      Serial.print(";");
      Serial.print("err");
      Serial.print(";");
      Serial.print("err");
      Serial.print(";");
      Serial.print("err");
      Serial.print(";");
      Serial.print("err");
      Serial.print(";");
      Serial.println();

      myFile.print("err");
      myFile.print(";");
      myFile.print("err");
      myFile.print(";");
      myFile.print("err");
      myFile.print(";");
      myFile.print("err");
      myFile.print(";");
      myFile.print("err");
      myFile.print(";");





    }
    g++; //kövi fénymérő
  } //fénymérők vége

  //akkuk állapotának ellenőrzése, kiíratása
  Serial.println();
  Serial.print("VOLTLESZ _ ");
  int Voltage = readVcc();

  Serial.println(Voltage);
  myFile.print(Voltage);
  myFile.println();
  
  //a rendszer önkikapcsolása ha lemerült az akku
  if (Voltage < 3500) { //310=~3.22 Volt
    digitalWrite(pwr, HIGH);
    Serial.println("shutdown");
 }

  //kártyára írás
  myFile.flush();
  digitalWrite(led, HIGH);
  delay(85); // villantunk
  digitalWrite(led, LOW);

  //a rendszer önkikapcsolása ha lemerült az akku

  //ciklus vége
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

/**************************************************************************/
/*
    Configures the gain and integration time for the TSL2591
*/
/**************************************************************************/
void configureSensor(void)
{
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  //  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  // tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  tsl.setGain(TSL2591_GAIN_MAX);
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  //  tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  /* Display the gain and integration time for reference sake */
  // Serial.println("------------------------------------");
  // Serial.print  ("Gain:         ");
  tsl2591Gain_t gain = tsl.getGain();
  switch (gain)
  {
    case TSL2591_GAIN_LOW:
      //      Serial.println("1x (Low)");
      break;
    case TSL2591_GAIN_MED:
      //   Serial.println("25x (Medium)");
      break;
    case TSL2591_GAIN_HIGH:
      //    Serial.println("428x (High)");
      break;
    case TSL2591_GAIN_MAX:
      //     Serial.println("9876x (Max)");
      break;
  }
  //  Serial.print  ("Timing:       ");
  // Serial.print((tsl.getTiming() + 1) * 100, DEC);
  // Serial.println(" ms");
  // Serial.println("------------------------------------");
  //  Serial.println("");
}




