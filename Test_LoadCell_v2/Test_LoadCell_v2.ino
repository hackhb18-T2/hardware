#include <HX711_ADC.h>

//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell(D2, D3);

long t;
int watchdog;
float differences[20];
int i; 
bool stable, lastStable;
float compensatedValue, lastValue, cumulatedDifference; 
    
void setup() {
  pinMode(D5, INPUT);
  pinMode(D1, OUTPUT);
  setupScale();
}

void loop() {
  //update scale value every cycle
  updateValueFromScale();
  
  //read smoothed value from data set + current calibration factor
  if (millis() > t + 250) {
    t = millis();
    float currentValue = getValueFromScale();
    int productCount = calcProductCount(currentValue);
    checkTara(); 
  }
}

void setupScale() {
  Serial.begin(115200);
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

  //while (LoadCell.getTareStatus() == false) {
  while (watchdog < 10000) {
    watchdog++;
  }
  watchdog = 0;
  Serial.println("Startup complete");
  ESP.wdtEnable(5000);
 
  tone(D1, 1000); // Send 1KHz sound signal...
  delay(1000);        // ...for 1 sec
  noTone(D1);     // Stop sound...
}

void checkTara() {
  bool TaraButton = digitalRead(D5);
  bool TaraButtonLast, FullButtonLast;
  
  //receive from serial terminal
  if (Serial.available() > 0) {
    float i;
    char inByte = Serial.read();
    if (inByte == 't') {
      LoadCell.tareNoDelay();
      while (LoadCell.getTareStatus() == false) {
        watchdog++;
        yield();
      }
    }
  }

  // Tara by button
  if(TaraButton && !TaraButtonLast){
    LoadCell.tareNoDelay();
    compensatedValue = 0;
    while (LoadCell.getTareStatus() == false) {
      yield();
    }    
  }
  TaraButtonLast = TaraButton;
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

