#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <MQTT.h>
#include "Adafruit_CCS811.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DS3231_SCL 1 //---- Переназначаем стандартный пин для Wire.h
#define DS3231_SDA 3 //---- Переназначаем стандартный пин для Wire.h
LiquidCrystal_I2C lcd(0x27, 24, 2); //---- Адрес адаптера, количество символов, количество строк

const char *ssid = "dd-wrt";
const char *password = "";

Adafruit_CCS811 ccs;
WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect() {
    Serial.print("checking wifi...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }

    Serial.print("\nconnecting...");
    while (!client.connect("arduino", "try", "try")) {
        Serial.print(".");
        delay(1000);
    }

    Serial.println("\nconnected!");

    client.subscribe("/hello");
    // client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
    Serial.println("incoming: " + topic + " - " + payload);
}

void setup() {
    Wire.begin(DS3231_SDA, DS3231_SCL); //---- Запускаем I2C на нужных пинах

    Serial.begin(115200);
    Serial.println("Booting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
    Serial.println("Ready - lvl");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    /*
     * MQTT
     * */

    client.begin("192.168.1.135", net);
    client.onMessage(messageReceived);

    connect();

    /*
     * CCS811
     * */

/*    if(!ccs.begin()){
        Serial.println("Failed to start sensor! Please check your wiring.");
        while(1);
    }

    //calibrate temperature sensor
    while(!ccs.available());
    float temp = ccs.calculateTemperature();
    ccs.setTempOffset(temp - 25.0);*/
}

void loop() {
    ArduinoOTA.handle();

    client.loop();

    if (!client.connected()) {
        connect();
    }



    if(ccs.available()){
        float temp = ccs.calculateTemperature();
        if(!ccs.readData()){
            const char* co2 = reinterpret_cast<const char*>(ccs.geteCO2());

            client.publish("/co2", String(ccs.geteCO2()));
            Serial.print("/co2");
            Serial.println(String(ccs.geteCO2()));

            client.publish("/tvoc", String(ccs.getTVOC()));
            Serial.print("/tvoc");
            Serial.println(String(ccs.getTVOC()));

            client.publish("/temp", String(temp));
            Serial.print("/temp");
            Serial.println(String(temp));

            lcd.begin();
            lcd.setCursor(0, 0); // 1 строка
            lcd.print("co2: ");
            lcd.print(co2);
            lcd.setCursor(0, 1); // 2 строка
            lcd.print("засада");
        }
        else{
            Serial.println("ERROR!");
            while(1);
        }
    }
//    delay(500);

    /*
    // publish a message roughly every second.
    if (millis() - lastMillis > 1000) {
        lastMillis = millis();
        client.publish("/hello", "world");
    }*/
}
