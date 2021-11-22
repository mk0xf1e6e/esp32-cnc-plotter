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
// current step resolution 1 for full step 2 for half step and ect...
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
  currentStepMode=mode;
  if(mode==1){
    digitalWrite(XYM1,LOW);
    digitalWrite(XYM2,LOW);
    digitalWrite(XYM3,LOW);
  }
  else if(mode==2){
    digitalWrite(XYM1,HIGH);
    digitalWrite(XYM2,LOW);
    digitalWrite(XYM3,LOW);
  }
  else if(mode==4){
    digitalWrite(XYM1,LOW);
    digitalWrite(XYM2,HIGH);
    digitalWrite(XYM3,LOW);
  }
  else if(mode==8){
    digitalWrite(XYM1,HIGH);
    digitalWrite(XYM2,HIGH);
    digitalWrite(XYM3,LOW);
  }
  else if(mode==16){
    digitalWrite(XYM1,HIGH);
    digitalWrite(XYM2,HIGH);
    digitalWrite(XYM3,HIGH);
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
  setSteppersMode(1);
  setEnableSteppers(false);
}
// current position in steps
int32_t xcpStp=0,ycpStp=0,zcpStp=0;
// lower limit in steps
int32_t xLowerLimit=0,yLowerLimit=0,zLowerLimit=0;
// upper limit in steps
int32_t xUpperLimit=3360,yUpperLimit=3360,zUpperLimit=1120;
/*
 * Linear move via stepper motors,with certain duration(in microseconds).
 * */
void moveSteppers(int32_t x,int32_t y,int32_t z,uint64_t motionDelay){
  bool doReturn=false;
  int32_t xcp=xcpStp,ycp=ycpStp,zcp=zcpStp;
  xcpStp+=x*(16/currentStepMode);
  ycpStp+=y*(16/currentStepMode);
  zcpStp+=z;
  if(x!=0&&(xcpStp<xLowerLimit||xcpStp>xUpperLimit)){doReturn=true;}
  if(y!=0&&(ycpStp<yLowerLimit||ycpStp>yUpperLimit)){doReturn=true;}
  if(z!=0&&(zcpStp<zLowerLimit||zcpStp>zUpperLimit)){doReturn=true;}
  if(doReturn){
    xcpStp=xcp;
    ycpStp=ycp;
    zcpStp=zcp;
    return;
  }
  setEnableSteppers(true);
  digitalWrite(XDIR,x>0?LOW:HIGH);
  digitalWrite(YDIR,y>0?LOW:HIGH);
  digitalWrite(ZDIR,z>0?LOW:HIGH);
  x=abs(x);
  y=abs(y);
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
/*
 * step/mm if you multiply this factor by distance it will give you the number of steps.
 * (this factor is for full step)
 * */
uint8_t xStpMm=7,yStpMm=7,zStpMm=7;
/*
 * Here we process the G-code and move the stepper motors.
 * */
void doGCode(String upCode,String params[4]){
  // G codes
  if(upCode=="g00"||upCode=="g01"||upCode=="g28"){
    float x=0,y=0,z=0,f=0,cpx=0,cpy=0,cpz=0;
    cpx=(float)xcpStp/(float)(xStpMm*16);
    cpy=(float)ycpStp/(float)(yStpMm*16);
    cpz=(float)zcpStp/(float)zStpMm;
    if(upCode=="g28"){
      for(uint8_t i=0;i<4;i++){
        if(params[i]=="x")x=-cpx;
        if(params[i]=="y")y=-cpy;
        if(params[i]=="z")z=-cpz;
      }
      if(x==0&&y==0&&z==0){
        x=-cpx;
        y=-cpy;
        z=-cpz;
      }
    }
    else{
      for(int i=0;i<4;++i){
        if(params[i].indexOf('x')!=-1)x=params[i].substring(1).toFloat()-cpx;
        else if(params[i].indexOf('y')!=-1)y=params[i].substring(1).toFloat()-cpy;
        else if(params[i].indexOf('z')!=-1)z=params[i].substring(1).toFloat()-cpz;
        else if(params[i].indexOf('f')!=-1)f=params[i].substring(1).toFloat();
      }
    }
    float amplitude=getAmplitude(x,y,z);
    f=(upCode=="g01")?f==0?lastFeedRate:f:g00FeedRate;
    uint64_t motionDelay=(amplitude*60)/(f/1000000);
    int32_t xStp=roundf(x*xStpMm*currentStepMode),yStp=roundf(y*yStpMm*currentStepMode),zStp=roundf(z*zStpMm);
    if(upCode=="g28"){setSteppersMode(1);}
    moveSteppers(xStp,yStp,zStp,motionDelay);
    if(upCode=="g28"){
      delay(100);
      setEnableSteppers(false);
    }
    lastFeedRate=(upCode=="g01")?f:lastFeedRate;
  }
    // M codes
  else if(upCode=="m17"){setEnableSteppers(true);}
  else if(upCode=="m18"||upCode=="m84"){setEnableSteppers(false);}
    // MS code this is for microstepping
  else if(upCode=="ms1"){setSteppersMode(1);}
  else if(upCode=="ms2"){setSteppersMode(2);}
  else if(upCode=="ms4"){setSteppersMode(4);}
  else if(upCode=="ms8"){setSteppersMode(8);}
  else if(upCode=="ms16"){setSteppersMode(16);}
  else{
    Serial.println(":::Error: Unknown command.");
    return;
  }
  Serial.println("ok");
}
// endregion
// region joystick manager!
TaskHandle_t jmTask;
void joystickManagent(void*parameter){
  pinMode(27,INPUT);
  pinMode(36,INPUT);
  pinMode(39,INPUT);
  for(;;){
    int8_t x=round(((analogRead(36)-2075)/2000));
    int8_t y=round(((analogRead(39)-2075)/2000));
    if(x!=0||y!=0){
      moveSteppers(x,y,0,8000);
      setEnableSteppers(false);
    }
    else if(!digitalRead(27)){
      String params[4]={"","","",""};
      doGCode("g28",params);
      for(;;){
        if(digitalRead(27)){break;}
        delay(1);
      }
    }
    delay(20);
  }
}
// endregion
// region main functions
void setup(){
  xTaskCreatePinnedToCore(joystickManagent,"jm",1000,NULL,1,&jmTask,0);
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
    uint64_t start=micros();
    while(ci==40||ci==59){if(Serial.read()==10||micros()-start>=100000){break;}}
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else upCode+=(char)ci;
  }
  upCode.toLowerCase();
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    uint64_t start=micros();
    while(ci==40||ci==59){if(Serial.read()==10||micros()-start>=100000){break;}}
    if(ci==40||ci==59){
      isEndOfLine=true;
      break;
    }
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[0]+=(char)ci;
  }
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    uint64_t start=micros();
    while(ci==40||ci==59){if(Serial.read()==10||micros()-start>=100000){break;}}
    if(ci==40||ci==59){
      isEndOfLine=true;
      break;
    }
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[1]+=(char)ci;
  }
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    uint64_t start=micros();
    while(ci==40||ci==59){if(Serial.read()==10||micros()-start>=100000){break;}}
    if(ci==40||ci==59){
      isEndOfLine=true;
      break;
    }
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[2]+=(char)ci;
  }
  while(!isEndOfLine){
    int16_t ci=Serial.read();
    uint64_t start=micros();
    while(ci==40||ci==59){if(Serial.read()==10||micros()-start>=100000){break;}}
    if(ci==40||ci==59){
      isEndOfLine=true;
      break;
    }
    if(ci==-1)continue;
    isEndOfLine=ci==10;
    if(ci==10||ci==32)break;
    else params[3]+=(char)ci;
  }
  for(uint8_t i=0;i<4;++i)params[i].toLowerCase();
  doGCode(upCode,params);
}
// endregion
