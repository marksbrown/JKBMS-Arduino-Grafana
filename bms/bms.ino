// Dan Cottam 2025
//
// WiFi stuff 
// https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples/ 
//
// BMS Library
// https://github.com/chrissank/JKBMSInterface

// Wifi and HTTP, so we can send data
#include <WiFiS3.h>
#include "WiFiSSLClient.h"
#include "IPAddress.h"
#include <ArduinoHttpClient.h>

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h

// WiFi auth
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
byte wifistatus = 0;     // the WiFi radio's status

// HTTP settings
char server[] = "dancottam.nl";
int port = 443;
String HTTPUSER = SECRET_HTTPUSER;
String HTTPAUTH = SECRET_HTTPAUTH;
String path = "/projects/jkbms/api/v1/sql"; // API endpoint for inserting data
WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, server, port);

// BMS
#include <JKBMSInterface.h>
// Create BMS instance using Serial1 (specific to which Arduino you're on. Just Serial is the USB on my Uno R4)
JKBMSInterface bms(&Serial1);

// So we can communicate with the MaxScale API in JSON
#include <ArduinoJson.h>
String DBTARGET   = SECRET_DBTARGET;
String DB         = SECRET_DB;
String DBTABLE    = SECRET_DBTABLE;
String DBUSER     = SECRET_DBUSER;
String DBPASS     = SECRET_DBPASS;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up");

    // attempt to connect to WiFi network:
    while (wifistatus != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network:
        wifistatus = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    // print your boards IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // Initialize BMS communication (UART ports 0 and 1 on Uno R4)
    Serial1.begin(115200, SERIAL_8N1);
    bms.begin(115200);
}

void loop() {
    // Update BMS data (call this regularly)
    bms.update();

    // Check if we have valid data
    if (bms.isDataValid()) {

        // Create body data for connection token POST request
        JsonDocument TokenRequest;
        TokenRequest["user"] =          SECRET_DBUSER;
        TokenRequest["password"] =      SECRET_DBPASS;
        TokenRequest["target"] =        SECRET_DBTARGET;
        TokenRequest["db"] =            SECRET_DB;
        String jsonTokenRequest;
        serializeJson(TokenRequest, jsonTokenRequest);

        // Send request to create SQL session and get tokens
        client.beginRequest();
        client.post(path);
        client.sendBasicAuth(HTTPUSER, HTTPAUTH); // send the username and password for authentication
        client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
        client.sendHeader("Content-Length", jsonTokenRequest.length());
        client.beginBody();
        client.print(jsonTokenRequest);
        client.endRequest();

        // Pull out id and token from request
        String jsonResponse = client.responseBody();
        JsonDocument Response;
        deserializeJson(Response, jsonResponse);
        String ConnectionID = Response["data"]["id"];
        String ConnectionToken = Response["meta"]["token"];

        // Create body data for data POST request
        JsonDocument InsertRequest;
        InsertRequest["sql"] = "INSERT INTO " + String(SECRET_DBTABLE) + "(date,voltage,current,soc,cycles,power_temp,battery_temp,cell0_voltage,cell1_voltage,cell2_voltage,cell3_voltage,cell_voltage_delta,charging_enabled,discharging_enabled,ischarging,isdischarging) VALUES (CURRENT_TIMESTAMP," + String(bms.getVoltage(), 3) + "," + String(bms.getCurrent(), 3) + "," + String(bms.getSOC()) + "," + String(bms.getCycles()) + "," + String(bms.getPowerTemp(), 2) + "," + String(bms.getBatteryTemp(), 2) + "," + String(bms.getCellVoltage(0), 3) + "," + String(bms.getCellVoltage(1), 3) + "," + String(bms.getCellVoltage(2), 3) + "," + String(bms.getCellVoltage(3), 3) + "," + String(bms.getCellVoltageDelta(), 3) + "," + String(bms.isChargingEnabled()) + "," + String(bms.isDischargingEnabled()) + "," + String(bms.isCharging()) + "," + String(bms.isDischarging()) + ");";
        String jsonInsertRequest;
        serializeJson(InsertRequest, jsonInsertRequest);

        //Wait 2 seconds
        delay(2000);

        Serial.println(jsonInsertRequest);
        // Send request to insert data
        client.beginRequest();
        client.post(path + "/" + ConnectionID + "/queries?token=" + ConnectionToken);
        client.sendBasicAuth(HTTPUSER, HTTPAUTH); // send the username and password for authentication
        client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
        client.sendHeader("Content-Length", jsonInsertRequest.length());
        client.beginBody();
        client.print(jsonInsertRequest);
        client.endRequest();

        // read the status code and body of the response
        int statusCode = client.responseStatusCode();
        String response = client.responseBody();

        //Wait 2 seconds
        delay(2000);

        //End SQL session by deleting it from the server
        client.beginRequest();
        client.del(path + "/" + ConnectionID + "?token=" + ConnectionToken);
        client.sendBasicAuth(HTTPUSER, HTTPAUTH); // send the username and password for authentication
        client.endRequest();

        // read the status code and body of the response
        int delstatusCode = client.responseStatusCode();
        String delresponse = client.responseBody();

        //Wait 5 minutes
        delay(300000);

    }
    else {
        Serial.println("Waiting for BMS data...");
    }

    //Wait 5 seconds
    delay(5000);
}
