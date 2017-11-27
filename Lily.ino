#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include <ESP8266WiFi.h>


// Set these to run example.
#include <FirebaseArduino.h>
#define FIREBASE_HOST "yourprojectname.firebaseio.com"
#define FIREBASE_AUTH "your_AUTH"
#define WIFI_SSID "xxxx"
#define WIFI_PASSWORD "xxxx"

// distance sensor.
#define trigPinColor D7
#define echoPinColor D8

#define trigPinWhite D5
#define echoPinWhite D6

long durationColor = 0, durationWhite = 0;
int distanceColor = 0, distanceWhite = 0;

int espId = ESP.getChipId();

// DHT Sensor
#include "DHT.h"
#define DHTPinColor D3
#define DHTPinWhite D4
#define DHTTYPE DHT11   // DHT 11
// Initialize DHT sensor.
DHT dhtColor(DHTPinColor, DHTTYPE);
DHT dhtWhite(DHTPinWhite, DHTTYPE);

// Sensor variables
float hColor = 0;
float tColor = 0;
float fColor = 0;

float hWhite = 0;
float tWhite = 0;
float fWhite = 0;
// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

LiquidCrystal_I2C lcd(0x27, 20, 4); // SCL-D1 SDA-D2

#include <Servo.h>
Servo servoKapak, servoTambur;
#define servo1Pin D9 // kapak RX
#define servo2Pin D0 // tambur

int lp = 0;
String sepet = "";
void setup() {

  Serial.begin(9600);
  delay(10);
  dhtColor.begin();
  dhtWhite.begin();

  servoKapak.attach(servo1Pin);
  servoTambur.attach(servo2Pin);
  servoKapak.write(90);
  servoTambur.write(90);

  pinMode(trigPinColor, OUTPUT);
  pinMode(echoPinColor, INPUT);

  pinMode(trigPinWhite, OUTPUT);
  pinMode(echoPinWhite, INPUT);

  WiFi.disconnect();
  delay(100);
  // connect to wifi.

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  lcd.init();
  lcd.backlight();
  lcd.begin(20, 4);
  lcd.setCursor(5, 0);
  lcd.print("Welcome...");
  delay(1000);
  lcd.clear();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

}

//int n = 0;

void loop() {

  Serial.print("ESP Chip ID: ");
  Serial.println(espId);

  Serial.print("Renkli sepeti çalışma durumu: ");
  Serial.println(Firebase.getString("RunColor"));
  delay(100);
  Serial.print("Beyaz sepeti çalışma durumu: ");
  Serial.println(Firebase.getString("RunWhite"));

  if (Firebase.getString("RunColor") == "true") {
    sepet = "color";
    runMachine(sepet);
    sepet = "";
  } else {
    sepet = "";
  }

  if (Firebase.getString("RunWhite") == "true") {
    sepet = "white";
    runMachine(sepet);
    sepet = "";
  } else {
    sepet = "";
  }

  readDHTColor();
  readDHTWhite();

  calcDistColor();
  calcDistWhite();


  delay(100);
  printData();

  sendJSONData(); // sending JSON data to Firebase

  delay(5000);

}

void runMachine(String basket) {
  int kapak = 0, tambur = 0;
  String state = "";
  //Serial.println(Firebase.getString("RunColor"));
  if (basket == "color") { // fireabase de çalıştır true ise
    kapak = 20; tambur = 85;
    state = "RunColor";
    Serial.println("Renkliler yikanacak");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Colors will be");  // colors will be washed
    lcd.setCursor(0, 1);
    lcd.print("washed...");
  } else if (basket == "white") {
    kapak = 160; tambur = 85;
    state = "RunWhite";
    Serial.println("Beyazlar yikanacak");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Whites will be");
    lcd.setCursor(0, 1);
    lcd.print("washed...");

  }
  delay(1500); // 1sec bekle.

  // kirlilerin bosaltilmasi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Emptying laundry...");
  //lcd.setCursor(0, 1);
  //lcd.print("laundry...");
  Serial.println("Kirliler boşaltılıyor...");
  servoKapak.write(kapak); // renkli kapağı için 0 dereceye ayarlanmalı
  servoTambur.write(tambur); // tamburu yavaşça çevir
  delay(7000); // 7 sec run.

  // kapak kapatilmasi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cover closed.");
  Serial.println("Kapak kapatıldı. Yıkama için hazırlanıyor...");
  servoKapak.write(90); // renkli sepetin kapagi kapat
  servoTambur.write(90); // tamburu durdur.
  delay(2000);

  // deterjan alinmasi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Taking detergent...");
  delay(2000);

  // yikama baslamasi
  //servoTambur.write(0); // tamburu çevir yıkama başlasın.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Washing");
  lcd.setCursor(0, 1);
  lcd.print("in progress...");
  Serial.println("Çamşırlar yıkanıyor...");
  servoTambur.write(0); // tamburu çevir yıkama başlasın.
  delay(10000); // 10sec yıkama yap.
  servoTambur.write(180); // tamburu çevir yıkama başlasın.

  delay(10000);
  servoTambur.write(90); // tamburu durdur
  delay(250);

  // durulama islemi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rinsing...");
  servoTambur.write(80); // tamburu yavas dondur.

  delay(5000); // 5sec
  servoTambur.write(90); // tamburu durdur
  delay(250);

  // sıkma yapılıyor.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Squeezing...");
  servoTambur.write(180); // tamburu yavas dondur.
  delay(8000); // 8sec

  // yikama tamamlandi
  servoTambur.write(90); // tamburu durdur.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Washing completed.");
  Serial.println("Yıkama işlemi tamamlandı.");
  delay(1000);
  Firebase.setString(state, "false");
  //Firebase.setBool("Tambur", false);
}
void sendJSONData() {
  // Color..................
  StaticJsonBuffer<200> jsonBufferColor;
  JsonObject& rootColor = jsonBufferColor.createObject();

  rootColor["temperature"] = tColor;
  rootColor["humidity"] = hColor;
  rootColor["distance"] = distanceColor;
  rootColor["basket"] = "color";
  // append a new value to /logDHT
  String nameColor = Firebase.push("/devices/color", rootColor);

  // White..................
  StaticJsonBuffer<200> jsonBufferWhite;
  JsonObject& rootWhite = jsonBufferWhite.createObject();

  rootWhite["temperature"] = tWhite;
  rootWhite["humidity"] = hWhite;
  rootWhite["distance"] = distanceWhite;
  rootWhite["basket"] = "white";
  // append a new value to /logDHT
  String nameWhite = Firebase.push("/devices/white", rootWhite);

  Serial.println(distanceColor);
  Serial.println(distanceWhite);

  // handle error.
  if (Firebase.failed()) {
    Serial.print("Firebase Pushing /sensor/dht failed:");
    Serial.println(Firebase.error());
    return;

  } else {
    Serial.print("Firebase Pushed /sensor/dht ");
    Serial.print(nameColor);
    Serial.print(" - ");
    Serial.println(nameWhite);
  }
}

void calcDistColor () {
  digitalWrite(trigPinColor, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinColor, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinColor, LOW);
  durationColor = pulseIn(echoPinColor, HIGH);

  distanceColor = 100 - (((((durationColor * 0.034) / 2) - 5) / 50) * 100);
  if (distanceColor < 0) {
    distanceColor = 0;
  }
  if (distanceColor > 100) {
    distanceColor = 100;
  }
  //Serial.println(distanceColor);
}

void calcDistWhite () {
  digitalWrite(trigPinWhite, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPinWhite, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinWhite, LOW);
  durationWhite = pulseIn(echoPinWhite, HIGH);

  distanceWhite = 100 - (((((durationWhite * 0.034) / 2) - 5) / 50) * 100);
  if (distanceWhite < 0) {
    distanceWhite = 0;
  }
  if (distanceWhite > 100) {
    distanceWhite = 100;
  }
  //Serial.println(distanceWhite);
}
void readDHTColor() {
  hColor = dhtColor.readHumidity();
  delay(50);
  // Read temperature as Celsius (the default)
  tColor = dhtColor.readTemperature();
  delay(50);
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //fColor = dhtColor.readTemperature(true);
  // Check if any reads failed and exit early (to try again).

  //delay(100); //********************

  if (isnan(hWhite) || isnan(tWhite) || isnan(fWhite) || isnan(hColor) || isnan(tColor) || isnan(fColor)) {
    Serial.println("Failed to read from DHT sensor!");
    hColor = 0; tColor = 0; fColor = 0;
  } else {
    Serial.print("Humidity: ");
    Serial.print(hColor);
    Serial.print(" Temperature: ");
    Serial.print(tColor);
    Serial.println(" *C ");
    //Serial.println(fColor);
  }

}

void readDHTWhite() {
  hWhite = dhtWhite.readHumidity();
  delay(50);
  // Read temperature as Celsius (the default)
  tWhite = dhtWhite.readTemperature();
  delay(50);
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //fWhite = dhtWhite.readTemperature(true);
  // Check if any reads failed and exit early (to try again).

  //delay(100); //********************
  if (isnan(hWhite) || isnan(tWhite) || isnan(fWhite) || isnan(hColor) || isnan(tColor) || isnan(fColor)) {
    Serial.println("Failed to read from DHT sensor!");
    hWhite = 0; tWhite = 0; fWhite = 0;
  } else {
    Serial.print("Humidity: ");
    Serial.print(hWhite);
    Serial.print(" Temperature: ");
    Serial.print(tWhite);
    Serial.println(" *C ");
    //Serial.println(fWhite);
  }
}

void printData() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("White");
  lcd.setCursor(0, 1);
  lcd.print("Tmp:");
  lcd.print((int)tWhite);
  lcd.print("C");
  lcd.setCursor(0, 2);
  lcd.print("Hmd:");
  lcd.print((int)hWhite);
  lcd.print("%");
  lcd.setCursor(0, 3);
  lcd.print("Load:");
  lcd.print(distanceWhite);
  lcd.print("%");

  //delay(50); // ****************

  lcd.setCursor(11, 0);
  lcd.print("Color");
  lcd.setCursor(10, 1);
  lcd.print("Tmp:");
  lcd.print((int)tColor);
  lcd.print("C");
  lcd.setCursor(10, 2);
  lcd.print("Hmd:");
  lcd.print((int)hColor);
  lcd.print("%");
  lcd.setCursor(10, 3);
  lcd.print("Load:");
  lcd.print(distanceColor);
  lcd.print("%");
}

