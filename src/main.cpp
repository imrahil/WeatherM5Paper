#include <M5EPD.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson.git
#include <NTPClient.h>   //https://github.com/taranais/NTPClient
#include "Orbitron_Medium_20.h"
#include "Orbitron44.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <app_secrets.h>

M5EPD_Canvas canvas(&M5.EPD);

const char *ssid = WLAN_SSID;
const char *password = WLAN_PASS;
String api_town = "Ennetmoos";
String api_country = "CH";
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q=" + api_town + "," + api_country + "&units=metric&APPID=";
const String key = WEATHER_KEY;

StaticJsonDocument<1024> doc;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

void initWiFi() {
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi ..");
  int count_try = 0;

  while (WiFi.status() != WL_CONNECTED && count_try < WIFI_MAX_TRY)
  {
    Serial.print(".");
    delay(1000);

    count_try++;
  }

  Serial.println();

  if (WiFi.status() == WL_NO_SSID_AVAIL) {
    Serial.println("Impossible to establish WiFi connection!");
    WiFi.disconnect();
  } else {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void setup(void)
{
  M5.begin();
  M5.EPD.SetRotation(90);
  M5.EPD.Clear(true);
  M5.RTC.begin();
  M5.SHT30.Begin();

  canvas.createCanvas(540, 960);
  canvas.useFreetypeFont(false);
  canvas.setFreeFont(&Orbitron_Medium_25);
  canvas.drawJpgFile(SD, "/back.jpg");
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);

  initWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +1 = 3600
    // GMT +8 = 28800
    // GMT -1 = -3600
    // GMT 0 = 0
    timeClient.setTimeOffset(3600);
  }

  delay(100);
}

void getData()
{
  String temperature = "--";
  String humidity = "--";
  String town = "--";
  String pressure = "--";
  String windSpeed = "--";

  // Check the current connection status
  if ((WiFi.status() == WL_CONNECTED))
  {
    HTTPClient http;
    String payload = ""; // whole json

    http.begin(endpoint + key); // Specify the URL
    int httpCode = http.GET();  // Make the request

    // check for the returning code
    if (httpCode > 0)
    {
      payload = http.getString();

      char input[1000];
      payload.toCharArray(input, 1000);
      deserializeJson(doc, input);

      JsonObject main = doc["main"];

      temperature = main["temp"].as<String>();
      humidity = main["humidity"].as<String>();
      pressure = main["pressure"].as<String>();

      town = doc["name"].as<String>();
      windSpeed = doc["wind"]["speed"].as<String>();
    }

    http.end(); // Free the resources
  }

  M5.SHT30.UpdateData();

  float temHere = M5.SHT30.GetTemperature();
  float humHere = M5.SHT30.GetRelHumidity();

  canvas.setFreeFont(&Orbitron_Bold_44);
  canvas.drawString(temperature.substring(0, 4), 340, 260);
  canvas.drawString(String(temHere).substring(0, 4), 180, 260);

  canvas.setFreeFont(&Orbitron_Bold_66);
  canvas.drawString(humidity, 348, 580);
  canvas.drawString(String((int)humHere), 190, 580);

  canvas.setFreeFont(&Orbitron_Medium_25);
  canvas.drawString(town, 122, 784);
  canvas.drawString(pressure + " hPa", 164, 846);
  canvas.drawString(windSpeed + " m/s", 122, 816);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    initWiFi();
  }

  Serial.println("downloading data...");
  getData();

  if (WiFi.status() == WL_CONNECTED) {
    while (!timeClient.update())
    {
      timeClient.forceUpdate();
    }

    // The formattedDate comes with the following format:
    // 2018-05-28T16:00:13Z
    // We need to extract date and time
    formattedDate = timeClient.getFormattedDate();
    Serial.println(formattedDate);

    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);

    timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
    String current = timeStamp.substring(0, 5);

    canvas.drawString(current, 140, 884);
  }

  canvas.drawString(String(M5.getBatteryVoltage() / 1000.00), 430, 884);

  canvas.pushCanvas(0, 0, UPDATE_MODE_A2);

  Serial.println("going into sleep mode...");
  delay(2000);
  M5.shutdown(600);
}
