#include "Arduino.h"
// region basic math functions
float getAmplitude(float x,float y,float z){return sqrtf((x*x)+(y*y)+(z*z));}
// endregion
// region stepper motors
#define EN 2
#define XDIR 32
#define YDIR 17
#define ZDIR 26
#define XSTP 4
#define YSTP 16
#define ZSTP 25
#define XYM1 15
#define XYM2 14
#define XYM3 33
/*
 * if the onboard led is on, then stepper motors are disabled, and vice versa.
 * So HIGH:false is disabled, LOW:true is enabled.
 * */
void setEnableSteppers(bool enable){digitalWrite(EN,enable?LOW:HIGH);}
// current step resolution
uint8_t currentStepMode=1;
/*
 * modes:
 *      0 : full step
 *      1 : half step
 *      2 : quarter step
 *      3 : eighth step
 *      4 : sixteenth step
 * */
void setSteppersMode(uint8_t mode){
  switch(mode){
    case 0:
      digitalWrite(XYM1,LOW);
      digitalWrite(XYM2,LOW);
      digitalWrite(XYM3,LOW);
      currentStepMode=1;
      break;
    case 1:
      digitalWrite(XYM1,HIGH);
      digitalWrite(XYM2,LOW);
      digitalWrite(XYM3,LOW);
      currentStepMode=2;
      break;
    case 2:
      digitalWrite(XYM1,LOW);
      digitalWrite(XYM2,HIGH);
      digitalWrite(XYM3,LOW);
      currentStepMode=4;
      break;
    case 3:
      digitalWrite(XYM1,HIGH);
      digitalWrite(XYM2,HIGH);
      digitalWrite(XYM3,LOW);
      currentStepMode=8;
      break;
    case 4:
      digitalWrite(XYM1,HIGH);
      digitalWrite(XYM2,HIGH);
      digitalWrite(XYM3,HIGH);
      currentStepMode=16;
      break;
  }
}
/*
 * Define all stepper pinMode as OUTPUT & Disable the stepper motors & set xy stepper mode to full step.
 * */
void setupSteppers(){
  pinMode(EN,OUTPUT);
  pinMode(XDIR,OUTPUT);
  pinMode(YDIR,OUTPUT);
  pinMode(ZDIR,OUTPUT);
  pinMode(XSTP,OUTPUT);
  pinMode(YSTP,OUTPUT);
  pinMode(ZSTP,OUTPUT);
  pinMode(XYM1,OUTPUT);
  pinMode(XYM2,OUTPUT);
  pinMode(XYM3,OUTPUT);
  setSteppersMode(0);
  setEnableSteppers(false);
}
// current position in steps
int32_t xcpStp=0,ycpStp=0,zcpStp=0;
/*
 * Linear move via stepper motors,with certain duration(in microseconds).
 * */
void moveSteppers(int32_t x,int32_t y,int32_t z,uint64_t motionDelay){
  xcpStp+=x;
  ycpStp+=y;
  zcpStp+=z;
  setEnableSteppers(true);
  digitalWrite(XDIR,x>0?LOW:HIGH);
  digitalWrite(YDIR,y>0?LOW:HIGH);
  digitalWrite(ZDIR,z>0?LOW:HIGH);
  x=abs(x)*currentStepMode;
  y=abs(y)*currentStepMode;
  z=abs(z);
  uint64_t x_delay=(x==0)?motionDelay*2:motionDelay/(2*x);
  uint64_t y_delay=(y==0)?motionDelay*2:motionDelay/(2*y);
  uint64_t z_delay=(z==0)?motionDelay*2:motionDelay/(2*z);
  uint8_t x_state=LOW,y_state=LOW,z_state=LOW;
  digitalWrite(XSTP,x_state);
  digitalWrite(YSTP,y_state);
  digitalWrite(ZSTP,z_state);
  uint64_t start_micros=esp_timer_get_time(),x_time=esp_timer_get_time(),y_time=esp_timer_get_time(),
      z_time=esp_timer_get_time(),t;
  while(true){
    t=micros();
    if(t-start_micros>=motionDelay)break;
    if(t-x_time>=x_delay){
      x_state=(x_state==LOW)?HIGH:LOW;
      digitalWrite(XSTP,x_state);
      x_time=micros();
    }
    if(t-y_time>=y_delay){
      y_state=(y_state==LOW)?HIGH:LOW;
      digitalWrite(YSTP,y_state);
      y_time=micros();
    }
    if(t-z_time>=z_delay){
      z_state=(z_state==LOW)?HIGH:LOW;
      digitalWrite(ZSTP,z_state);
      z_time=micros();
    }
  }
  digitalWrite(XSTP,LOW);
  digitalWrite(YSTP,LOW);
  digitalWrite(ZSTP,LOW);
}
// endregion
// region G-code processing
float g00FeedRate=1000,lastFeedRate=1000;// mm/min
uint16_t xStpMm=10,yStpMm=10,zStpMm=10;
void doGCode(String upCode,String params[4]){
  // G codes
  if(upCode=="g00"||upCode=="g01"){
    float x=0,y=0,z=0,f=0;
    for(int i=0;i<4;++i){
      if(params[i].indexOf('x')!=-1)x=params[i].substring(1).toFloat()-(float)xcpStp/xStpMm;
      else if(params[i].indexOf('y')!=-1)y=params[i].substring(1).toFloat()-(float)ycpStp/yStpMm;
      else if(params[i].indexOf('z')!=-1)z=params[i].substring(1).toFloat()-(float)zcpStp/zStpMm;
      else if(params[i].indexOf('f')!=-1)f=params[i].substring(1).toFloat();
    }
    float amplitude=getAmplitude(x,y,z);
    f=(upCode=="g01")?f==0?lastFeedRate:f:g00FeedRate;
    uint64_t motionDelay=(amplitude*60)/(f/1000000);
    int32_t xStp=roundf(x*xStpMm),yStp=roundf(y*yStpMm),zStp=roundf(z*zStpMm);
    moveSteppers(xStp,yStp,zStp,motionDelay);
    lastFeedRate=(upCode=="g01")?f:lastFeedRate;
  }
    // M codes
  else if(upCode=="m17"){setEnableSteppers(true);}
  else if(upCode=="m18"||upCode=="m84"){setEnableSteppers(false);}
  else if(upCode=="ms1"){setSteppersMode(1);}
  else if(upCode=="ms2"){setSteppersMode(2);}
  else if(upCode=="ms3"){setSteppersMode(3);}
  else if(upCode=="ms4"){setSteppersMode(4);}
  else if(upCode=="ms5"){setSteppersMode(5);}
  Serial.println("ok");
}
// endregion
// region main functions
void setup(){
  setupSteppers();
  Serial.begin(115200);
}
void loop(){
  String upCode="";
  String params[4]={"","","",""};
  bool isEndOfLine;
  while(!Serial.available())delay(1);
  while(1){
    int16_t ci=Serial.read();
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else upCode+=(char)ci;
  }
  upCode.toLowerCase();
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[0]+=(char)ci;
  }
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[1]+=(char)ci;
  }
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[2]+=(char)ci;
  }
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[3]+=(char)ci;
  }
  for(uint8_t i=0;i<4;++i)params[i].toLowerCase();
  doGCode(upCode,params);
}
// endregion
