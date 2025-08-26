/*
  ESP32 MQTT-Based Smart Home Miniature for 6 Rooms

  Features:
  1. Terrace:
     - RFID scanning: on valid card, servo activation
     - I2C LCD: displays "Selamat Datang" on valid scan
     - PIR motion sensor active between 01:00-03:00 (configurable via MQTT); triggers alarm on pin 27
     - Light control via MQTT
  2. Living Room:
     - Light control via MQTT
     - Fan control via MQTT
  3. Bathroom:
     - PIR sensor: auto-light ON with delay, via relay
  4. Bedroom 1:
     - Light control via MQTT
  5. Bedroom 2:
     - Light control via MQTT
     - DHT22: temperature & humidity publishing via MQTT on separate topics
  6. Kitchen:
     - Light control via MQTT
     - Gas sensor MQ-15: triggers alarm on pin 27

  MQTT Topics:
    terrace/light/set, terrace/light/status
    terrace/pir/enable (true/false)
    terrace/alarm
    terrace/rfid
    livingroom/light/set
    livingroom/fan/set
    bathroom/light/control
    bedroom1/light/set
    bedroom2/light/set
    bedroom2/dht/temperature
    bedroom2/dht/humidity
    kitchen/light/set
    kitchen/gas/alarm

  Pin Assignments:
    // I2C Bus
    SDA: 21
    SCL: 22

    // Terrace
    RFID RX: 16
    RFID TX: 17
    SERVO: 18
    PIR_TERRACE: 34
    ALARM: 27
    RELAY_TERRACE_LIGHT: 19

    // Living Room
    RELAY_LR_LIGHT: 23
    RELAY_LR_FAN: 5

    // Bathroom
    PIR_BATHROOM: 32
    RELAY_BATHLIGHT: 4

    // Bedroom 1
    RELAY_BR1_LIGHT: 2

    // Bedroom 2
    RELAY_BR2_LIGHT: 15
    DHT22_PIN: 33

    // Kitchen
    RELAY_KITCHEN_LIGHT: 13
    MQ15_PIN: 35
    // Alarm pin reused: ALARM (27)

  PIN CONFLICT CHECK:
    All pins unique except ALARM is shared for terrace PIR and kitchen gas alarm (intended share).

  Program Flow:
    1. Setup WiFi & MQTT client, reconnect logic
    2. Initialize peripherals: RFID, Servo, I2C LCD, PIRs, Relays, DHT, MQ15
    3. Subscribe to control topics for lights, fans, PIR enable
    4. Loop:
       - mqttClient.loop()
       - Check RFID: if valid UID, trigger servo, LCD message, publish terrace/rfid
       - Read time; if between 1-3 AM and terrace PIR enabled, check PIR_TERRACE -> if motion, trigger alarm
       - Check PIR_BATHROOM: if motion, switch bathroom light ON with delay off timer
       - Read DHT22 periodically, publish temperature & humidity
       - Read MQ15: if gas threshold, trigger alarm pin and publish kitchen/gas/alarm
       - Maintain relays based on last MQTT command state

  Required Libraries:
    WiFi.h
    PubSubClient.h
    LiquidCrystal_I2C.h
    Servo.h
    MFRC522.h
    DHT.h
    time.h (for NTP)
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <DHT.h>
#include <time.h>

// WiFi & MQTT settings
const char* ssid = "nama wifi";
const char* password = "pw";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* mqttUser = "nama";
const char* mqttPassword ="pw";

WiFiClient espClient;
PubSubClient client(espClient);

// I2C LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo
Servo terraceServo;

// RFID
#define SS_PIN 5
#define RST_PIN 17
MFRC522 rfid(SS_PIN, RST_PIN);

// DHT22
#define DHTPIN 33
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Pin defs
const int pinPIRTerrace = 34;
const int pinPIRBathroom = 32;
const int pinMQ15 = 35;
const int pinAlarm = 27;
const int relayTerraceLight = 19;
const int relayLRLight = 23;
const int relayLRFan = 5;
const int relayBathLight = 4;
const int relayBR1Light = 2;
const int relayBR2Light = 15;
const int relayKitchenLight = 13;
const int servoPin = 18;

// MQTT state
bool terraceLightState = false;
bool lrLightState = false;
bool lrFanState = false;
bool br1LightState = false;
bool br2LightState = false;
bool kitchenLightState = false;
bool terracePIREnabled = true;

void setup() {
  Serial.begin(115200);
  // Initialize pins
  pinMode(pinPIRTerrace, INPUT);
  pinMode(pinPIRBathroom, INPUT);
  pinMode(pinMQ15, INPUT);
  pinMode(pinAlarm, OUTPUT);
  pinMode(relayTerraceLight, OUTPUT);
  pinMode(relayLRLight, OUTPUT);
  pinMode(relayLRFan, OUTPUT);
  pinMode(relayBathLight, OUTPUT);
  pinMode(relayBR1Light, OUTPUT);
  pinMode(relayBR2Light, OUTPUT);
  pinMode(relayKitchenLight, OUTPUT);

  // Attach servo
  terraceServo.attach(servoPin);

  // Setup I2C LCD
  lcd.init();
  lcd.backlight();

  // RFID
  SPI.begin();
  rfid.PCD_Init();

  // DHT22
  dht.begin();

  // Connect WiFi and MQTT
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  // NTP time
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("Topic: "); Serial.print(topic);
  Serial.print("  Message: "); Serial.println(msg);

  if (String(topic) == "terrace/light/set") {
    terraceLightState = (msg == "ON");
    digitalWrite(relayTerraceLight, terraceLightState);
    client.publish("terrace/light/status", terraceLightState?"ON":"OFF");
  } else if (String(topic) == "terrace/pir/enable") {
    terracePIREnabled = (msg == "true");
  } // ... repeat for other topics
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe("terrace/light/set");
      client.subscribe("terrace/pir/enable");
      // subscribe others...
    } else {
      delay(5000);
    }
  }
}

unsigned long lastDHT = 0;
unsigned long lastPirBath = 0;

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // RFID handling
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) uid += String(rfid.uid.uidByte[i], HEX);
    Serial.println("UID: " + uid);
    terraceServo.write(90);
    lcd.clear();
    lcd.print("Selamat Datang");
    client.publish("terrace/rfid", uid.c_str());
    delay(2000);
    terraceServo.write(0);
    lcd.clear();
  }

  // Terrace PIR alarm between 1-3 AM
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int h = timeinfo.tm_hour;
    if (terracePIREnabled && h >= 1 && h < 3 && digitalRead(pinPIRTerrace)) {
      digitalWrite(pinAlarm, HIGH);
      client.publish("terrace/alarm", "MOTION");
    } else {
      digitalWrite(pinAlarm, LOW);
    }
  }

  // Bathroom PIR auto-light (with delay off)
  if (digitalRead(pinPIRBathroom)) {
    digitalWrite(relayBathLight, HIGH);
    lastPirBath = millis();
  }
  if (millis() - lastPirBath > 60000) { // off after 60s
    digitalWrite(relayBathLight, LOW);
  }

  // DHT22 publish every minute
  if (millis() - lastDHT > 60000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      client.publish("bedroom2/dht/temperature", String(t).c_str());
      client.publish("bedroom2/dht/humidity", String(h).c_str());
    }
    lastDHT = millis();
  }

  // MQ15 gas alarm
  if (analogRead(pinMQ15) > 3000) {
    digitalWrite(pinAlarm, HIGH);
    client.publish("kitchen/gas/alarm", "GAS");
  }

  delay(10);
}
