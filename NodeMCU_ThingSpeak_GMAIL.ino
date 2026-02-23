#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>

/* ================= WIFI ================= */
const char* ssid = "System";
const char* password = "12345678";

/* ============== THINGSPEAK ============== */
const char* server = "api.thingspeak.com";
String apiKey = "U8MRH6HER8D3U2TT";

WiFiClient client;

/* ============== GMAIL SETTINGS ============== */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

#define AUTHOR_EMAIL    "RealJalParichay@gmail.com"
#define AUTHOR_PASSWORD "dheivntnhdmqujuo"
#define RECIPIENT_EMAIL "satyampisote@gmail.com"

SMTPSession smtp;

/* ============== TIMING ================== */
unsigned long lastTSupdate = 0;
const unsigned long tsInterval = 16000;   // ThingSpeak rule

unsigned long lastMailTime = 0;
const unsigned long mailInterval = 1800000; // 30 minutes


/* ============ SENSOR VALUES ============= */
float temp = 0.0;
float hum = 0.0;
float tds = 0.0;
float ec = 0.0;
float turb = 1.0;      
float minerals = 0.0;

/* ================= SETUP ================= */
void setup() {
  Serial.begin(9600);  
  delay(100);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

/* ================= GMAIL FUNCTION ================= */
void sendGmailAlert(float tdsValue) {

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  SMTP_Message message;
  message.sender.name = "Water Quality Monitor";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "⚠️ Water Quality Alert";
  message.addRecipient("User", RECIPIENT_EMAIL);

  String body;
  body += "ALERT! Unsafe Water Detected\n\n";
  body += "TDS       : " + String(tdsValue, 1) + " ppm\n";
  body += "Turbidity :  > 1 NTU\n";
  body += "Status    : UNSAFE\n";
  body += "Location  : Hostel Water Tank\n\n";
  body += "Please take necessary action.";

  message.text.content = body.c_str();

  if (!smtp.connect(&session)) return;
  MailClient.sendMail(&smtp, &message);
}

/*  LOOP  */
void loop() {

  /*  SERIAL RECEIVE  */
  if (Serial.available()) {

    String dataLine = Serial.readStringUntil('\n');
    dataLine.trim();

    // Expected from UNO:
    // temp,hum,tds,ec,turb,minerals

    int i1 = dataLine.indexOf(',');
    int i2 = dataLine.indexOf(',', i1 + 1);
    int i3 = dataLine.indexOf(',', i2 + 1);
    int i4 = dataLine.indexOf(',', i3 + 1);
    int i5 = dataLine.indexOf(',', i4 + 1);

    if (i1 > 0 && i2 > 0 && i3 > 0 && i4 > 0 && i5 > 0) {
      temp     = dataLine.substring(0, i1).toFloat();
      hum      = dataLine.substring(i1 + 1, i2).toFloat();
      tds      = dataLine.substring(i2 + 1, i3).toFloat();
      ec       = dataLine.substring(i3 + 1, i4).toFloat();
      turb     = dataLine.substring(i4 + 1, i5).toFloat();
      minerals = dataLine.substring(i5 + 1).toFloat();
    }
  }

  /* WIFI AUTO‑RECONNECT  */
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    return;
  }

  /*  THINGSPEAK UPDATE */
  if (millis() - lastTSupdate >= tsInterval) {
    lastTSupdate = millis();

    if (client.connect(server, 80)) {

      String url = "/update?api_key=" + apiKey +
                   "&field1=" + String(tds) +
                   "&field2=" + String(turb) +
                   "&field3=" + String(temp) +
                   "&field4=" + String(ec) +
                   "&field5=" + String(minerals) +
                   "&field6=" + String(hum);

      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: api.thingspeak.com\r\n" +
                   "Connection: close\r\n\r\n");

      delay(50);
      client.stop();
    }

    /*  GMAIL ALERT (ONLY IF UNSAFE)  */
    if (tds > 500 && millis() - lastMailTime > mailInterval) {
      sendGmailAlert(tds);
      lastMailTime = millis();
    }
  }
}
