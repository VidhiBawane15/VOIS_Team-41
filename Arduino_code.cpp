#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

/* ---------- DHT11 CONFIG ---------- */
#define DHTPIN A2
#define DHTTYPE DHT11

/* ---------- TDS CONFIG ---------- */
#define TdsSensorPin A0
#define VREF 5.0
#define SCOUNT 15

/* ---------- TURBIDITY CONFIG ---------- */
#define TurbidityPin A1
#define TURBIDITY_OFFSET 860

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

/* ---------- VARIABLES ---------- */
int analogBuffer[SCOUNT];
int bufferIndex = 0;

float tdsValue = 0;
float turbidityNTU = 0;
float temperature = 25.0;
float humidity = 0.0;

float totalMinerals = 0;
float electricalConductivity = 0;

unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
int displayPage = 0;

/* ---------- MEDIAN FILTER (FIXED) ---------- */
int getMedianNum(int bArray[], int len) {
  int temp[len];
  for (int i = 0; i < len; i++) temp[i] = bArray[i];

  for (int i = 0; i < len - 1; i++) {
    for (int j = 0; j < len - i - 1; j++) {
      if (temp[j] > temp[j + 1]) {
        int t = temp[j];
        temp[j] = temp[j + 1];
        temp[j + 1] = t;
      }
    }
  }
  return temp[len / 2];
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  dht.begin();

  lcd.print("Water Quality");
  delay(1500);
  lcd.clear();
}

void loop() {

  /* ---- DHT ---- */
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;

  /* ---- TDS sampling ---- */
  analogBuffer[bufferIndex] = analogRead(TdsSensorPin);
  bufferIndex = (bufferIndex + 1) % SCOUNT;

  if (millis() - lastSensorUpdate >= 1000) {
    lastSensorUpdate = millis();

    float avgVoltage = getMedianNum(analogBuffer, SCOUNT) * VREF / 1024.0;
    float compCoeff = 1.0 + 0.02 * (temperature - 25.0);
    float compVoltage = avgVoltage / compCoeff;

    tdsValue = (133.42 * compVoltage * compVoltage * compVoltage
               - 255.86 * compVoltage * compVoltage
               + 857.39 * compVoltage) * 0.5;
    if (tdsValue < 0) tdsValue = 0;

    int raw = analogRead(TurbidityPin);
    int corrected = raw - TURBIDITY_OFFSET;
    if (corrected < 0) corrected = 0;

    turbidityNTU = corrected * 0.25;
    if (turbidityNTU < 1) turbidityNTU = 1;

    totalMinerals = tdsValue;
    electricalConductivity = tdsValue / 0.5;

    /* ---- CSV OUTPUT (DO NOT CHANGE) ---- */
    Serial.print(temperature); Serial.print(",");
    Serial.print(humidity); Serial.print(",");
    Serial.print(tdsValue); Serial.print(",");
    Serial.print(electricalConductivity); Serial.print(",");
    Serial.print(turbidityNTU); Serial.print(",");
    Serial.println(totalMinerals);
  }

  /* ---- LCD ROTATION ---- */
  if (millis() - lastDisplayUpdate >= 2000) {
    lastDisplayUpdate = millis();
    lcd.clear();

    if (displayPage == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Temp:");
      lcd.print(temperature, 0);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("Hum:");
      lcd.print(humidity, 0);
      lcd.print("%");
    }
    else if (displayPage == 1) {
      lcd.setCursor(0, 0);
      lcd.print("TDS:");
      lcd.print(tdsValue, 0);
      lcd.print("ppm");
      lcd.setCursor(0, 1);
      lcd.print("EC:");
      lcd.print(electricalConductivity, 0);
      lcd.print("uS");
    }
    else if (displayPage == 2) {
      lcd.setCursor(0, 0);
      lcd.print("Turbidity");
      lcd.setCursor(0, 1);
      lcd.print(turbidityNTU, 1);
      lcd.print(" NTU");
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print("Minerals:");
      lcd.print(totalMinerals, 0);
      lcd.print("Mg L");
    }

    displayPage++;
    if (displayPage > 3) displayPage = 0;
  }
}
