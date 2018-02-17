#include <HX711_ADC.h>

//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell(D8, D7);

long t;

void setup() {
  Serial.begin(115200);
  Serial.println("LoadCell.begin...");
  LoadCell.begin();

  
  long stabilisingtime = 1000; // tare preciscion can be improved by adding a few seconds of stabilising time
  ESP.wdtDisable();
  Serial.println("LoadCell.start...");
  LoadCell.start(stabilisingtime);
  
  Serial.println("LoadCell.setCalFactor...");
  LoadCell.setCalFactor(24.5); // user set calibration factor (float)
   
  Serial.println("Startup + tare is complete");
  ESP.wdtEnable(5000);
}

void loop() {
  //update() should be called at least as often as HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS
  //longer delay in scetch will reduce effective sample rate (be carefull with delay() in loop)
  LoadCell.update();

  //get smoothed value from data set + current calibration factor
  if (millis() > t + 250) {
    float i = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(i);
    t = millis();
  }

  //receive from serial terminal
  if (Serial.available() > 0) {
    float i;
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  //check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}
