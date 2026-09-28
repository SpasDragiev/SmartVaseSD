#include <Wire.h>
#include <7semi_SCD40.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>

#define DHTPIN 2
#define DHTTYPE DHT11
#define MOISTURE_POWER 7
#define MOISTURE_PIN A0
#define PH_PIN A1
#define SD_CS 10

//  MOISTURE CALIBRATION
#define DRY_VAL 465
#define WET_VAL 280

//  PH CALIBRATION
#define PH_NEUTRAL_VOLTAGE 2.50
#define PH_ACID_VOLTAGE    2.03

DHT dht(DHTPIN, DHTTYPE);
SCD40 scdSensor;
LiquidCrystal_I2C lcd(0x27, 16, 2);
File dataFile;

int currentScreen = 0;
unsigned long lastScreenChange = 0;
const unsigned long SCREEN_DURATION = 1500;
unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 5000;

float tempSCD = 0, rhSCD = 0;
uint16_t co2 = 0;
int soil = 0, soilPct = 0;
float phValue = 0;

//  FUNCTIONS
int readMoisture() {
  digitalWrite(MOISTURE_POWER, HIGH);
  delay(300);
  int value = analogRead(MOISTURE_PIN);
  digitalWrite(MOISTURE_POWER, LOW);
  return value;
}

int moisturePercent(int raw) {
  int pct = map(raw, DRY_VAL, WET_VAL, 0, 100);
  return constrain(pct, 0, 100);
}

float readPH() {
  int buf[10];
  for (int i = 0; i < 10; i++) {
    buf[i] = analogRead(PH_PIN);
    delay(10);
  }
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        int temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  long sum = 0;
  for (int i = 2; i < 8; i++) sum += buf[i];
  float voltage = (float)sum / 6.0 * 5.0 / 1024.0;
  float slope = (7.0 - 4.0) / (PH_NEUTRAL_VOLTAGE - PH_ACID_VOLTAGE);
  float ph = 7.0 + slope * (voltage - PH_NEUTRAL_VOLTAGE);
  return constrain(ph, 0, 14);
}

void showScreen(int screen) {
  lcd.clear();
  switch (screen) {

    case 0: // Temperature
      lcd.setCursor(0, 0);
      lcd.print(F("Temp: "));
      lcd.print(tempSCD, 1);
      lcd.print(F("C"));
      lcd.setCursor(0, 1);
      if (tempSCD < 0)        lcd.print(F("Plant will frezz"));
      else if (tempSCD < 10)  lcd.print(F("Too cold 4 plant"));
      else if (tempSCD < 18)  lcd.print(F("Bit cold 4 plant"));
      else if (tempSCD < 24)  lcd.print(F("Plant is happy! "));
      else if (tempSCD < 28)  lcd.print(F("Plant feels warm"));
      else if (tempSCD < 33)  lcd.print(F("Too hot 4 plant!"));
      else                    lcd.print(F("Plant in danger!"));
      break;

    case 1: // CO2
      lcd.setCursor(0, 0);
      lcd.print(F("CO2: "));
      lcd.print(co2);
      lcd.print(F(" ppm"));
      lcd.setCursor(0, 1);
      if (co2 < 500)          lcd.print(F("Great 4 growth! "));
      else if (co2 < 800)     lcd.print(F("Plant loves this"));
      else if (co2 < 1000)    lcd.print(F("Still OK 4 plant"));
      else if (co2 < 2000)    lcd.print(F("Open window now "));
      else                    lcd.print(F("Bad 4 ur plant! "));
      break;

    case 2: // Humidity
      lcd.setCursor(0, 0);
      lcd.print(F("Humidity: "));
      lcd.print(rhSCD, 0);
      lcd.print(F("%"));
      lcd.setCursor(0, 1);
      if (rhSCD < 20)         lcd.print(F("Plant is too dry"));
      else if (rhSCD < 40)    lcd.print(F("Needs more humid"));
      else if (rhSCD < 60)    lcd.print(F("Plant feels good"));
      else if (rhSCD < 70)    lcd.print(F("Nice 4 ur plant "));
      else if (rhSCD < 85)    lcd.print(F("Getting too damp"));
      else                    lcd.print(F("Too humid 4 root"));
      break;

    case 3: // Soil
      lcd.setCursor(0, 0);
      lcd.print(F("Soil: "));
      lcd.print(soilPct);
      lcd.print(F("%"));
      lcd.setCursor(0, 1);
      if (soilPct < 10)       lcd.print(F("Water me NOW!!! "));
      else if (soilPct < 25)  lcd.print(F("Plant needs H2O!"));
      else if (soilPct < 50)  lcd.print(F("Soil is doing ok"));
      else if (soilPct < 75)  lcd.print(F("Soil is perfect!"));
      else if (soilPct < 90)  lcd.print(F("Ease up watering"));
      else                    lcd.print(F("Root rot risk!  "));
      break;

    case 4: // pH
      lcd.setCursor(0, 0);
      lcd.print(F("pH: "));
      lcd.print(phValue, 2);
      lcd.print(F("        "));
      lcd.setCursor(0, 1);
      if (phValue < 4.0)      lcd.print(F("Way too acidic! "));
      else if (phValue < 5.5) lcd.print(F("Too acid 4 roots"));
      else if (phValue < 6.5) lcd.print(F("Slightly acidic "));
      else if (phValue < 7.5) lcd.print(F("Perfect 4 plant!"));
      else if (phValue < 8.5) lcd.print(F("Slightly alkalin"));
      else                    lcd.print(F("Too alkaline!   "));
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(MOISTURE_POWER, OUTPUT);
  digitalWrite(MOISTURE_POWER, LOW);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("Starting..."));

  scdSensor.begin();

  if (!SD.begin(SD_CS)) {
    lcd.clear();
    lcd.print(F("SD ERROR"));
    while (1);
  }

  lcd.clear();
  lcd.print(F("System Ready"));
  delay(2000);

  SD.remove("data.txt");   // <-- my guess, see note below

  dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(F("TempC,Humidity,CO2,Soil,SoilPct,pH"));
    dataFile.close();
  }

  showScreen(0);
}

void loop() {
  if (millis() - lastRead >= READ_INTERVAL) {
    lastRead = millis();

    soil    = readMoisture();
    soilPct = moisturePercent(soil);
    phValue = readPH();

    if (!scdSensor.readSingleShot(co2, tempSCD, rhSCD)) {
      Serial.println(F("SCD40 read error"));
    } else {
      Serial.print(F("Temp: "));             Serial.print(tempSCD);
      Serial.print(F(" C | Hum: "));         Serial.print(rhSCD);
      Serial.print(F(" % | CO2: "));         Serial.print(co2);
      Serial.print(F(" ppm | Soil raw: ")); Serial.print(soil);
      Serial.print(F(" | Soil: "));          Serial.print(soilPct);
      Serial.print(F("% | pH: "));           Serial.println(phValue, 2);

      dataFile = SD.open("data.txt", FILE_WRITE);
      if (dataFile) {
        dataFile.print(tempSCD); dataFile.print(F(","));
        dataFile.print(rhSCD);   dataFile.print(F(","));
        dataFile.print(co2);     dataFile.print(F(","));
        dataFile.print(soil);    dataFile.print(F(","));
        dataFile.print(soilPct); dataFile.print(F(","));
        dataFile.println(phValue, 2);
        dataFile.close();
      }
    }
  }

  if (millis() - lastScreenChange >= SCREEN_DURATION) {
    lastScreenChange = millis();
    currentScreen = (currentScreen + 1) % 5;
    showScreen(currentScreen);
  }
}