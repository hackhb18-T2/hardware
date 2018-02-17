#include <HX711_ADC.h>
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

// device data
int product_weight;

// testing
long randNumber;

//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell(D2, D3);

//Scale
long t;
int watchdog;
float differences[20];
int i, lastProductCount; 
bool stable, lastStable;
float compensatedValue, lastValue, cumulatedDifference; 
    
void setup() {
  //network communication
  Serial.begin(115200);                          // set serial baud rate
  Serial.println();
  Serial.println("Setup network communication...");

  wifi_init();                                   // Connect to wifi, get MAC + IP

  https_get_product_weight();                    // get product weight from server

  Serial.println("Setup completed");
  Serial.println("");

  //Scale
  pinMode(D5, INPUT);
  pinMode(D1, OUTPUT);
  setupScale();

  https_post_quantity(0);
}

void loop() {
  //update scale value every cycle
  updateValueFromScale();
  
  //read smoothed value from data set + current calibration factor
  if (millis() > t + 250) {
    t = millis();
    float currentValue = getValueFromScale();
    int productCount = calcProductCount(currentValue);
    if(lastProductCount != productCount) {
      https_post_quantity(productCount);
    }
    lastProductCount = productCount;     
  }
}

void setupScale() {
  Serial.println("LoadCell.begin...");
  LoadCell.begin();
  
  long stabilisingtime = 1000; // tare preciscion can be improved by adding a few seconds of stabilising time
  ESP.wdtDisable();
  Serial.println("LoadCell.start...");
  LoadCell.start(stabilisingtime);
  
  Serial.println("LoadCell.setCalFactor...");
  LoadCell.setCalFactor(24.5); // user set calibration factor (float)
  Serial.println("LoadCell.tareNoDelay...");
  LoadCell.tareNoDelay(); 

  while (watchdog < 10000) {
    watchdog++;
  }
  watchdog = 0;
  Serial.println("Startup complete");
  ESP.wdtEnable(5000);
}

void updateValueFromScale() {
  //update() should be called at least as often as HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS
  //longer delay in scetch will reduce effective sample rate (be carefull with delay() in loop)
  LoadCell.update();
}

float getValueFromScale() {
    float value = LoadCell.getData();
    Serial.print("#Curr:");
    Serial.print(value);
    return value;
}

int calcProductCount(float value) {
  float productWeight = 630;
  float boxWeight = 0;

  //measure stability of value
  float difference = value - lastValue;
  if (difference < 10 && difference > -10){
    stable = 1;
  }
  else{
    stable = 0;
  }
  lastValue = value;

  //write difference of measurement to array
  differences[i] = difference;
    
  //increment i
  i++;
  if(i == 20){
    i = 0;
  }

  //if stable becomes active
  if(stable == 1 && lastStable == 0){
    //cumulate difference in array
    for (int j = 0; j <= 19; j++){
       cumulatedDifference += differences[j];
    }
    //if cumulated difference is big enough
    if ((cumulatedDifference > (productWeight / 2)) || (cumulatedDifference < (-productWeight / 2))){
      //add difference to compensated value
      compensatedValue += cumulatedDifference;
    }
  }
  lastStable = stable;
  cumulatedDifference = 0;
    
  int productCount = round((compensatedValue - boxWeight) / productWeight);

  Serial.print(" #Fixed:");
  Serial.print(compensatedValue);
  Serial.print(" #Stable:");
  Serial.print(stable);
  Serial.print(" #Count:");
  Serial.println(productCount);
  
  return productCount;
}

// ***************************  HTTPS stuff  ***************************
void https_get_product_weight() {
    String url = "/devices/" + String(deviceID) + "/product_weight/?format=json";
    
    String result = https_get(url);

    Serial.println(result);
    product_weight = result.toInt();
}

void https_post_quantity(int quantity) {
    String url; 
    String data = "last_weight=" + String(quantity);

    tone(D1, 1000); // Send 1KHz sound signal...
    delay(500);        // ...for 1 sec
    noTone(D1);     // Stop sound...
    
    url = "/devices/" + String(deviceID) + "/weight";
    if(https_post(url, data)) {
      tone(D1, 1000); // Send 1KHz sound signal...
      delay(1500);        // ...for 1 sec
      noTone(D1);     // Stop sound...
    }
}

String https_get(String url) {
    String http_string;
    String line;

    // ***************************  GET  ***************************
    Serial.println("************* GET *************");
    Serial.println("[HTTPS] GET begin...");
    WiFiClientSecure client;
    
    Serial.printf("[HTTPS] connecting to %s\n", host);
    if (!client.connect(host, httpsPort)) {                    // open connection
      Serial.println("[HTTPS] connection failed!");
      return "";      
    }
    Serial.println("[HTTPS] connection successful!");

    Serial.println("[HTTPS] checking certificate");
    if (!client.verify(fingerprint, host)) {                   // check certificate SHA1 fingerprint
        Serial.println("[HTTPS] certificate doesn't match");
        return "";
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
        return "";
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
      //Serial.print(line);
    }
  
    Serial.println();
    Serial.println("[HTTPS] connection closed");

    line.replace("{", "");
    line.replace("}", "");
    int colon_index = line.indexOf(":");
    String value = line.substring(colon_index + 1);

    Serial.print("[HTTPS] GET - value of first json key: ");
    Serial.println(value);

    return value;
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


// *********************** other *************************
void wifi_init() {
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
}
