#include "Arduino.h"
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
void setEnableSteppers(bool enable){
  digitalWrite(EN,enable?LOW:HIGH);
}
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
      break;
    case 1:
      digitalWrite(XYM1,HIGH);
      digitalWrite(XYM2,LOW);
      digitalWrite(XYM3,LOW);
      break;
    case 2:
      digitalWrite(XYM1,LOW);
      digitalWrite(XYM2,HIGH);
      digitalWrite(XYM3,LOW);
      break;
    case 3:
      digitalWrite(XYM1,HIGH);
      digitalWrite(XYM2,HIGH);
      digitalWrite(XYM3,LOW);
      break;
    case 4:
      digitalWrite(XYM1,HIGH);
      digitalWrite(XYM2,HIGH);
      digitalWrite(XYM3,HIGH);
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
/*
 * Linear move via stepper motors,with certain duration(in microseconds).
 * */
void moveSteppers(int32_t x,int32_t y,int32_t z,uint64_t motionDelay){
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
// region main functions
void setup(){
  setupSteppers();
}
void loop(){
}
// endregion