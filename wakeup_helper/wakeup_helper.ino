/*
Final Project - WakeUp Helper Project.

Waking up is not always easy. Sometime you snooze your alaram constantly and wake up
an hour later then planned. In this project I tried to created a solution for scenario:
On a specific time every morning (let's say 08:00), a good old Nokia ringtone will be played.
The only way you can stop this sound is by opening the light in your room.
This will make you get out of bed.
In addition - you will be able to see the current temprature in blynk's app, 
and go through your waking up time in a google sheet (because we love data).

The circuit: 
Light sensor connected to pin number 34.
Speaker connected to pin number 26.
Temprature sensor (DHT22) connected to pin number 15.

Link to a video - https://youtube.com/shorts/LoNaJvxOuvQ
Link to instructables post - https://www.instructables.com/WakingUp-Helper/

Created By:
Or Zinger - 205918725 
*/ 


#define BLYNK_TEMPLATE_ID           "{your_blynk_template_id}"
#define BLYNK_TEMPLATE_NAME         "{your_blynk_template_name}"
#define BLYNK_AUTH_TOKEN            "{your_blynk_auth_key}"
#define BLYNK_PRINT Serial
#define DHTPIN 15
#define DHTTYPE DHT22

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include "pitches.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//------WIFI Details---------------
char ssid[] = "your_wifi_network";
char pass[] = "your_password";
//---------------------------------


int tempo = 180;
int micPin = 26;
int lightPin = 34;
int baseLight = analogRead(lightPin);
unsigned long startTime;
unsigned long wakingTime;

int melody[] = {
  NOTE_E5, 8, NOTE_D5, 8, NOTE_FS4, 4, NOTE_GS4, 4,
  NOTE_CS5, 8, NOTE_B4, 8, NOTE_D4, 4, NOTE_E4, 4,
  NOTE_B4, 8, NOTE_A4, 8, NOTE_CS4, 4, NOTE_E4, 4,
  NOTE_A4, 2,
};

int notes = sizeof(melody) / sizeof(melody[0]) / 2;
int wholenote = (60000 * 4) / tempo;
int divider = 0, noteDuration = 0;
bool nokiaPlaying = false;
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;


void setup()
{
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
}

void loop()
{
  Blynk.run();

  if (nokiaPlaying) {
    playNokia();
  } else {
    stopNokia();
  }
}

BLYNK_WRITE(V0) {
  baseLight = analogRead(lightPin);
  int action = param.asInt();
  startTime = millis();
  if (action == 1) {
    nokiaPlaying = true;
  }
}

BLYNK_WRITE(V1) {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    dht.temperature().getEvent(&event);
  }
  Blynk.virtualWrite(V2, String(event.temperature, 1) + " °C");
}

void playNokia() {
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
    divider = melody[thisNote + 1];
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }
    tone(micPin, melody[thisNote], noteDuration * 0.9);
    delay(noteDuration);
    noTone(micPin);
    delay(10);
    int lightValue = analogRead(lightPin);
    if (abs(lightValue - baseLight) > 500) {
      nokiaPlaying = false;
      wakingTime = (millis() - startTime) / 1000;
      report(wakingTime);
      Blynk.virtualWrite(V0, 0);
      break;
    }
  }
}

void stopNokia() {
  noTone(micPin);
}

void report (int data) {
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client -> setInsecure(); {
      HTTPClient https;
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, String("{your_make.com_webhook}") + data)) {
        Serial.print("[HTTPS] GET...\n");
        int httpCode = https.GET();
        if (httpCode > 0) {
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        }
        else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
      }
      else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }
    delete client;
  }
  else {
    Serial.println("Unable to create client");
  }
  Serial.println();
}
