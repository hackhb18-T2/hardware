#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// wifi
const char* ssid = "LEDE-N";
const char* password = "tplinkac1750";

// https 
const char* host = "hackhb18-t2-api.herokuapp.com";   // heroku
const int httpsPort = 443;
const char* fingerprint = "08 3B 71 72 02 43 6E CA ED 42 86 93 BA 7E DF 81 C4 BC 62 30"; // heroku
     
// device information
const int deviceID = 1;


// other
String url;
String http_string;

// testing
long randNumber;




void setup() {
    Serial.begin(115200);                          // set serial baud rate
    Serial.println();
    Serial.println("Setup...");

    Serial.print("Wifi:");
    WiFi.mode(WIFI_STA);                           // "station" mode = connects to access point
    WiFi.begin(ssid, password);                    // set ssid and password

    while (WiFi.status() != WL_CONNECTED) {        // wait until wifi is connected
      delay(500);
      Serial.print(".");
    }            
    Serial.println(" connected!");

    String mac_Address = WiFi.macAddress();        // get microcontroller MAC address
    mac_Address.replace(":","");
    Serial.println(mac_Address);
    Serial.println(WiFi.localIP());                // get local IP address
    

    // testing
    randomSeed(analogRead(0));
  
    Serial.println("Setup completed");
    Serial.println("--------");
    Serial.println("");
}




void loop() {
     
     // *************  HTTPS *************
    WiFiClientSecure client;
    
    https_open();


    // *************  GET *************  
    //url = "/";
    //https_get(url);

    //url = "/devices/" + String(deviceID) + "/weight";
    //https_get(url);

    
    // *************  POST ************* 
    String data = "pst=temperature>" + String(random(0,100)) +"||humidity>" + String(random(0,100)) + "||data>text";
    
    Serial.println("[HTTPS] POST: ");
    url = "/";
    http_string = String("POST ") + url + " HTTP/1.1\r\n" +
                  "Host: " + host +
                  "Accept: */*\r\n" + 
                  "Content-Type: application/x-www-form-urlencoded\r\n" +
                  "Content-Length: " + data.length() + "\r\n" +
                  "\r\n" + 
                  data;
                  
    Serial.println(http_string);              
    
    client.println(http_string);
    
    
    
    if (client.connected()) { 
      client.stop();  // DISCONNECT FROM THE SERVER
    }
    
         
    Serial.println();
    delay(5000);
}

// ***************************  HTTPS stuff  ***************************

void https_open() {

    Serial.println("[HTTPS] begin...");
    WiFiClientSecure client;
    
    Serial.printf("[HTTPS] connecting to %s\n", host);
    if (!client.connect(host, httpsPort)) {                    // open connection
      Serial.println("[HTTPS] connection failed!");
      return;      
    }
    Serial.println("[HTTPS] connection successful!");


    Serial.println("[HTTPS] checking certificate");
    if (!client.verify(fingerprint, host)) {                   // check certificate SHA1 fingerprint
        Serial.println("[HTTPS] certificate doesn't match");
        return;
    }
    Serial.println("[HTTPS] certificate matches");
 
}


void https_get(String url) {
    WiFiClientSecure client;
    
    Serial.print("[HTTPS] requesting URL: ");
    Serial.println(url);
            
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +     // send GET information
                 "Host: " + host + "\r\n" +
                 "User-Agent: ESP8266HTTPSClient\r\n" +
                 "Connection: close\r\n\r\n");
        
    Serial.println("[HTTPS] request sent");
    while (client.connected()) {                              // read HTTP answer
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line == "\r") {
        Serial.println("[HTTPS] headers received");
        break;
      }
    }
    
    String line = client.readStringUntil('\n');
    if (line.startsWith("{\"state\":\"success\"")) {
      Serial.println("[HTTPS] client successfull!");
    } 
    else {
      Serial.println("[HTTPS] client has failed");
    }
    Serial.print("[HTTPS] reply was:");
    Serial.println(line);
    Serial.println("[HTTPS] closing connection");
}

