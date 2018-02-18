#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// wifi
const char* ssid = "LEDE-N";
const char* password = "tplinkac1750";

// https 
const char* host = "hackhb18-t2-api.herokuapp.com";   // heroku
const int httpsPort = 443;
const char* fingerprint = "08 3B 71 72 02 43 6E CA ED 42 86 93 BA 7E DF 81 C4 BC 62 30"; // heroku

// device information
const int deviceID = 4;

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
    String url;

    //url = "/";
    //https_get(url);

    // ***** post *****
    //String data = "last_weight=" + String(random(0,100));
    //url = "/devices/" + String(deviceID) + "/weight";
    //https_post(url, data);

    // ***** get *****
    url = "/devices/" + String(deviceID) + "/weight/?format=json";
    https_get(url);
         
    Serial.println();
    delay(5000);
}

// ***************************  HTTPS stuff  ***************************

void https_get(String url) {
    String http_string;
    String line;

    // ***************************  GET  ***************************
    Serial.println("************* GET *************");
    Serial.println("[HTTPS] GET begin...");
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
    
    Serial.println("[HTTPS] GET request: ");
    Serial.println("");
    http_string = String("GET ") + url + " HTTP/1.1\r\n" +     // build GET string
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n" +
                 "\r\n";
            
    Serial.print(http_string);
    client.print(http_string);                                 // send GET string
    Serial.println("[HTTPS] GET request sent");

    unsigned long timeout = millis();
    while (client.available() == 0) {                          // wait for server response
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }

    while (client.connected()) {                               // read server response header
      line = client.readStringUntil('\n');
      if (line == "\r") {
              Serial.print(line);

        Serial.println("[HTTPS] headers received");
        break;
      }
    }
     
    Serial.println("[HTTPS] GET response:");
    while(client.available()){                                 // read server response
      line = client.readStringUntil('\r');
      Serial.print(line);
    }
  
    Serial.println();
    Serial.println("[HTTPS] connection closed");

    line.replace("{", "");
    line.replace("}", "");


    
    
}


byte https_post(String url, String data) {
    byte result;
    String http_string;

    // *************************** POST ***************************
    Serial.println("*************  POST *************");
    Serial.println("[HTTPS] POST begin...");
    WiFiClientSecure client;
    
    Serial.printf("[HTTPS] connecting to %s\n", host);
    if (!client.connect(host, httpsPort)) {                    // open connection
      Serial.println("[HTTPS] connection failed!");
      return 0;      
    }
    Serial.println("[HTTPS] connection successful!");

    Serial.println("[HTTPS] checking certificate");
    if (!client.verify(fingerprint, host)) {                   // check certificate SHA1 fingerprint
        Serial.println("[HTTPS] certificate doesn't match");
        return 0;
    }
    Serial.println("[HTTPS] certificate matches");
   
    Serial.println("[HTTPS] POST request: ");
    http_string = String("POST ") + url + "/ HTTP/1.1\r\n" +   // build POST string
                  "Host: " + host + "\r\n" +
                  //"Accept: */*\r\n" + 
                  "Content-Type: application/x-www-form-urlencoded\r\n" +
                  "Content-Length: " + data.length() + "\r\n" +
                  "\r\n" + 
                  data;
    Serial.println("");              
    Serial.println(http_string);              
    Serial.println("");
    client.print(http_string);                                  // send POST string
    Serial.println("[HTTPS] POST request sent");

    unsigned long timeout = millis();                           
    while (client.available() == 0) {                           // wait for server response
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return 0;
      }
    }
     
    while (client.connected()) {                               // read server response header
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("[HTTPS] headers received");
        break;
      }
    }

    String line = client.readStringUntil('\n');   
    if (line.startsWith("{\"status\":\"success\"")) {         // check if server response body indicates success
      Serial.println("[HTTPS] POST successful!!!");
      result = 1;
    } 
    else {
      Serial.println("[HTTPS] POST failed");
      result = 0;
    }    
     
    if (client.connected()) {                                 // close connection
      client.stop();  
      Serial.println("[HTTPS] connection closed");
    }
    
    return result;
}
