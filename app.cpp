#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

/* -------- DHT11 CONFIG -------- */
#define DHTPIN 2
#define DHTTYPE DHT11

/* -------- TDS CONFIG -------- */
#define TdsSensorPin A0
#define VREF 5.0
#define SCOUNT 30

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

/* -------- VARIABLES -------- */
int analogBuffer[SCOUNT];
int analogBufferIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25.0;
float humidity = 0.0;

/* -------- FUNCTION PROTOTYPE -------- */
int getMedianNum(int bArray[], int iFilterLen);

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  dht.begin();

  lcd.setCursor(0, 0);
  lcd.print("Water Monitor");
  lcd.setCursor(0, 1);
  lcd.print("DHT11 + TDS");
  delay(2000);
  lcd.clear();
}

void loop() {
  /* -------- READ DHT11 -------- */
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT11 Error");
    lcd.clear();
    lcd.print("DHT11 Error");
    delay(2000);
    return;
  }

  /* -------- READ TDS (UNCHANGED) -------- */
  analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
  analogBufferIndex++;
  if (analogBufferIndex >= SCOUNT)
    analogBufferIndex = 0;

  averageVoltage = getMedianNum(analogBuffer, SCOUNT) * VREF / 1024.0;

  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;

  tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
             - 255.86 * compensationVoltage * compensationVoltage
             + 857.39 * compensationVoltage) * 0.5;

  /* -------- SERIAL OUTPUT -------- */
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C | Hum: ");
  Serial.print(humidity);
  Serial.print(" % | TDS: ");
  Serial.print(tdsValue);
  Serial.println(" ppm");

  /* -------- LCD DISPLAY -------- */
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C H:");
  lcd.print(humidity, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("TDS:");
  lcd.print(tdsValue, 0);
  lcd.print("ppm");

  delay(2000);   // DHT11 safe delay
}

/* -------- MEDIAN FILTER (UNCHANGED) -------- */
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];

  for (int i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];

  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        int temp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = temp;
      }
    }
  }
  return bTab[iFilterLen / 2];
}
