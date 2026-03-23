// Dan Cottam 2025
//
// WiFi stuff:
// https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples/
//
// BMS Library:
// https://github.com/chrissank/JKBMSInterface

#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <IPAddress.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <JKBMSInterface.h>

#include "arduino_secrets.h"

// WiFi auth
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
byte wifiStatus = 0;

// HTTP settings
char server[]   = "dancottam.nl";
int port        = 443;
String httpUser = SECRET_HTTPUSER;
String httpAuth = SECRET_HTTPAUTH;
String apiPath  = "/projects/jkbms/api/v1/sql";
WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, server, port);

// BMS - Serial1 is the UART port on Uno R4 (Serial is USB)
JKBMSInterface bms(&Serial1);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up");

    while (wifiStatus != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        wifiStatus = WiFi.begin(ssid, pass);
        delay(10000);
    }

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // Initialize BMS communication
    Serial1.begin(115200, SERIAL_8N1);
    bms.begin(115200);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost — reconnecting...");
        WiFi.disconnect();
        while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
            Serial.println("Retrying WiFi...");
            delay(5000);
        }
        Serial.println("WiFi reconnected");
    }

    bms.clearData(); // age out previous read so stale data is never re-inserted
    bms.update();

    if (bms.isDataValid()) {

        // Build and send token request
        JsonDocument tokenRequest;
        tokenRequest["user"]     = SECRET_DBUSER;
        tokenRequest["password"] = SECRET_DBPASS;
        tokenRequest["target"]   = SECRET_DBTARGET;
        tokenRequest["db"]       = SECRET_DB;
        String jsonTokenRequest;
        serializeJson(tokenRequest, jsonTokenRequest);

        client.beginRequest();
        client.post(apiPath);
        client.sendBasicAuth(httpUser, httpAuth);
        client.sendHeader("Content-Type", "application/json");
        client.sendHeader("Content-Length", jsonTokenRequest.length());
        client.beginBody();
        client.print(jsonTokenRequest);
        client.endRequest();

        // Parse connection ID and token from response
        String jsonTokenResponse = client.responseBody();
        JsonDocument tokenResponse;
        DeserializationError err = deserializeJson(tokenResponse, jsonTokenResponse);
        if (err) {
            Serial.print("Failed to parse token response: ");
            Serial.println(err.c_str());
            delay(5000);
            return;
        }

        String connectionID    = tokenResponse["data"]["id"].as<String>();
        String connectionToken = tokenResponse["meta"]["token"].as<String>();

        // Build SQL INSERT
        String sql = "INSERT INTO ";
        sql += SECRET_DBTABLE;
        sql += "(date,voltage,current,soc,cycles,power_temp,battery_temp,";
        sql += "cell0_voltage,cell1_voltage,cell2_voltage,cell3_voltage,";
        sql += "cell_voltage_delta,charging_enabled,discharging_enabled,";
        sql += "ischarging,isdischarging) VALUES (CURRENT_TIMESTAMP,";
        sql += String(bms.getVoltage(), 3)          + ",";
        sql += String(bms.getCurrent(), 3)           + ",";
        sql += String(bms.getSOC())                  + ",";
        sql += String(bms.getCycles())               + ",";
        sql += String(bms.getPowerTemp(), 2)         + ",";
        sql += String(bms.getBatteryTemp(), 2)       + ",";
        sql += String(bms.getCellVoltage(0), 3)      + ",";
        sql += String(bms.getCellVoltage(1), 3)      + ",";
        sql += String(bms.getCellVoltage(2), 3)      + ",";
        sql += String(bms.getCellVoltage(3), 3)      + ",";
        sql += String(bms.getCellVoltageDelta(), 3)  + ",";
        sql += String(bms.isChargingEnabled())       + ",";
        sql += String(bms.isDischargingEnabled())    + ",";
        sql += String(bms.isCharging())              + ",";
        sql += String(bms.isDischarging())           + ");";

        JsonDocument insertRequest;
        insertRequest["sql"] = sql;
        String jsonInsertRequest;
        serializeJson(insertRequest, jsonInsertRequest);

        delay(2000);
        Serial.println(jsonInsertRequest);

        // Send INSERT request
        client.beginRequest();
        client.post(apiPath + "/" + connectionID + "/queries?token=" + connectionToken);
        client.sendBasicAuth(httpUser, httpAuth);
        client.sendHeader("Content-Type", "application/json");
        client.sendHeader("Content-Length", jsonInsertRequest.length());
        client.beginBody();
        client.print(jsonInsertRequest);
        client.endRequest();

        int insertStatus = client.responseStatusCode();
        String insertResponse = client.responseBody();
        if (insertStatus < 200 || insertStatus >= 300) {
            Serial.print("Insert failed, status: ");
            Serial.println(insertStatus);
            Serial.println(insertResponse);
        }

        delay(2000);

        // Close SQL session
        client.beginRequest();
        client.del(apiPath + "/" + connectionID + "?token=" + connectionToken);
        client.sendBasicAuth(httpUser, httpAuth);
        client.endRequest();

        int delStatus = client.responseStatusCode();
        client.responseBody(); // consume response to release connection
        if (delStatus < 200 || delStatus >= 300) {
            Serial.print("Session delete failed, status: ");
            Serial.println(delStatus);
        }

        delay(300000);

    } else {
        Serial.println("Waiting for BMS data...");
        delay(5000);
    }
}
