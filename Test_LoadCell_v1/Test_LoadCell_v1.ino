#include <HX711_ADC.h>

//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell(D8, D7);

long t;
float differences[20];
int i; 
bool stable, lastStable;
float compensatedValue, lastValue, cumulatedDifference; 
    
void setup() {
  pinMode(D5, INPUT);
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
    printDebugData();
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
   
  Serial.println("Startup + tare is complete");
  ESP.wdtEnable(5000); 
}

void checkTara() {
  bool TaraButton = digitalRead(D5);
  bool TaraButtonLast, FullButtonLast;
  
  //receive from serial terminal
  if (Serial.available() > 0) {
    float i;
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // Tara by button
  if(TaraButton && !TaraButtonLast){
    LoadCell.tareNoDelay();
    compensatedValue = 0;    
  }
  TaraButtonLast = TaraButton;

  //check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}

void updateValueFromScale() {
  //update() should be called at least as often as HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS
  //longer delay in scetch will reduce effective sample rate (be carefull with delay() in loop)
  LoadCell.update();

float getValueFromScale() {
    float value = LoadCell.getData();
    return value;
}

int calcProductCount(float value) {
  float productWeight = 303;
  float boxWeight = 2050;

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
    if ((cumulatedDifference > (ProductWeight / 2)) || (cumulatedDifference < (-ProductWeight / 2))){
      //add difference to compensated value
      compensatedValue += cumulatedDifference;
    }
  }
  lastStable = stable;
  cumulatedDifference = 0;
    
  int productCount = round((compensatedValue - boxWeight) / productWeight);
  return productCount;
  
void printDebugData() {    
  Serial.print("#Curr:");
  Serial.print(value);
  Serial.print(" #Fixed:");
  Serial.print(compensatedValue);
  Serial.print(" #Stable:");
  Serial.print(stable);
  Serial.print(" #Count:");
  Serial.println(ProductCount);
}

