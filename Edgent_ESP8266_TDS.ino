//// Thesis project 2021-2022 code by Phan Quoc Trung (Designing Water Quality Monitoring System Through IOT)
// Fill-in information from your Blynk Template here
#define BLYNK_TEMPLATE_ID "TMPL8vGLTjsa"
#define BLYNK_DEVICE_NAME "Water quality 2"

#define BLYNK_FIRMWARE_VERSION        "0.1.0"
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
#define APP_DEBUG
#define USE_NODE_MCU_BOARD
#include "BlynkEdgent.h"
int buzzer=5; //D1
int warnLevel=500;
BlynkTimer timer;
int timerID1,timerID2;
int button=0; //D3
boolean buttonState=HIGH;
boolean runMode=1;//ON/OFF ALARM
boolean warnState=0;
//-------------------------------Instantiate TDS INPUT----------------------------------//
#define TdsSensorPin A0
#define VREF 3.3      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,temperature = 25;
//-------------------------------------------------------------------------------------//
WidgetLED led(V0);

void setup()
{
  Serial.begin(115200);
  delay(100);
  pinMode(button,INPUT_PULLUP);
  pinMode(buzzer,OUTPUT);
  pinMode(TdsSensorPin,INPUT);
  digitalWrite(buzzer,LOW); 
  BlynkEdgent.begin();
  timerID1 = timer.setInterval(1000L,handleTimerID1);
}

void loop() {
  BlynkEdgent.run();
  timer.run();
  if(digitalRead(button)==LOW){
    if(buttonState==HIGH){
      buttonState=LOW;
      runMode=!runMode;
      Serial.println("Run mode: " + String(runMode));
      Blynk.virtualWrite(V4,runMode);
      delay(200);
    }
  }else{
    buttonState=HIGH;
  }
}

void handleTimerID1(){
   static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=((133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5)-11.0; //convert voltage value to tds value
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
   }
  Blynk.virtualWrite(V1,tdsValue);
  if(led.getValue()) {
    led.off();
  } else {
    led.on();
  }
  if(runMode==1){
    if(tdsValue>warnLevel){
      if(warnState==0){
        warnState=1;
        Blynk.logEvent("warning", String("WARNING! HIGH TDS LEVEL! TDS VALUE=" + String(tdsValue)+" Polluted water!"));
        timerID2 = timer.setTimeout(60000L,handleTimerID2);
      }
      digitalWrite(buzzer,HIGH);
      Blynk.virtualWrite(V3,HIGH);
      Serial.println("ALARM ON!");
    }else{
      digitalWrite(buzzer,LOW);
      Blynk.virtualWrite(V3,LOW);
      Serial.println("ALARM OFF!");
    }
  }else{
    digitalWrite(buzzer,LOW);
    Blynk.virtualWrite(V3,LOW);
    Serial.println("ALARM OFF!");
  }
}

int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}

void handleTimerID2(){
  warnState=0;
}
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2,V4);
}
BLYNK_WRITE(V2) {
  warnLevel = param.asInt();
}
BLYNK_WRITE(V4) {
  runMode = param.asInt();
}
