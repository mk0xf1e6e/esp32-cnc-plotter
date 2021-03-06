#include "SD.h"
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
int32_t xUpperLimit=3360,yUpperLimit=3360,zUpperLimit=896;
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
  uint64_t start_micros=esp_timer_get_time(),x_time=esp_timer_get_time(),y_time=esp_timer_get_time(),z_time=esp_timer_get_time(),t;
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
// region SD card functions
/*
 * Current directory
 * */
String current_directory="";
/*
 * Mounting the SD card.
 * */
void SD_setup(){
  uint8_t index=0;
  while(!SD.begin()||index<10){
    index+=1;
    delay(100);
  }
  if(SD.cardType()==CARD_NONE){return;}
}
/*
 * get total byte size of card
 * */
void gtb(){Serial.println(String((unsigned long)SD.totalBytes()));}
/*
 * get total used byte
 * */
void gub(){Serial.println(String((unsigned long)SD.usedBytes()));}
/*
 * Getting list of directories
 * */
void ls(String dir){
  if(dir[dir.length()-1]=='/'){dir=current_directory+dir.substring(0,dir.lastIndexOf('/'));}
  else{dir=current_directory+dir;}
  dir="/"+dir;
  if(!SD.exists(dir)){Serial.println(":::Error: <"+dir+"> directory does not exist.");}
  else{
    File root=SD.open(dir);
    if(!root.isDirectory()){Serial.println(":::Error: not a directory.");}
    else{
      File file=root.openNextFile();
      while(file){
        if(file.isDirectory()){
          String sub_dir=String(file.name());
          Serial.print(sub_dir.substring(sub_dir.lastIndexOf('/')+1));
          Serial.print(" ");
        }
        else{
          String respond=String(file.name());
          respond=respond.substring(respond.lastIndexOf('/')+1)+':';
          respond+=String((long)file.size());
          Serial.print(respond);
          Serial.print(" ");
        }
        file=root.openNextFile();
      }
      Serial.println();
    }
  }
}
/*
 * cd to a directory
 * */
void cd(String dir){
  if(dir==".."){
    if(current_directory[current_directory.length()-1]=='/')
      current_directory=current_directory.substring(0,current_directory.length()-1);
    current_directory=current_directory.substring(0,current_directory.indexOf('/'));
    Serial.println("ok");
    return;
  }
  if(current_directory[current_directory.length()-1]=='/'){dir=current_directory+dir;}
  else{dir=current_directory+"/"+dir;}
  if(!SD.exists(dir)){Serial.println(":::Error: <"+dir+"> directory does not exist.");}
  else{
    File root=SD.open(dir);
    if(!root.isDirectory()){Serial.println(":::Error: not a directory.");}
    else{
      current_directory=dir;
      Serial.println("ok");
    }
  }
}
/*
 * pwd -> report then current directory.
 * */
void pwd(){
  if(current_directory[0]=='/'){Serial.println(current_directory);}
  else{Serial.println("/"+current_directory);}
}
/*
 * create new directory
 * */
void mkdir(String dir){
  if(dir==".."){
    Serial.println(":::Error: can't create that directory.");
    return;
  }
  if(dir[dir.length()-1]=='/'){dir=dir.substring(0,dir.length()-1);}
  String _cd=current_directory[current_directory.length()-1]=='/'?"":"/";
  _cd=current_directory+_cd;
  dir="/"+_cd+dir;
  if(SD.exists(dir)){Serial.println(":::Error: directory already exist.");}
  else{
    if(SD.mkdir(dir.c_str())){Serial.println("Done");}
    else{Serial.println(":::Error: can't create directory.");}
  }
}
/*
 * rm removes the directory or files. (directory should be empty)
 * */
void rm(String dir){
  if(dir.isEmpty()){
    Serial.println(":::Error: you must specify a directory.");
    return;
  }
  if(dir[dir.length()-1]=='/'){dir=dir.substring(0,dir.length()-1);}
  String _cd=current_directory[current_directory.length()-1]=='/'?"":"/";
  _cd=current_directory+_cd;
  dir="/"+_cd+dir;
  if(!SD.exists(dir)){Serial.println(":::Error: directory does not exist.");}
  else{
    if(SD.open(dir.c_str()).isDirectory()){
      if(SD.rmdir(dir.c_str())){Serial.println("Done");}
      else{Serial.println(":::Error: can't remove directory.");}
    }
    else{
      if(SD.remove(dir.c_str())){Serial.println("Done");}
      else{Serial.println(":::Error: can't remove directory.");}
    }
  }
}
/*
 * read the existing file
 * */
void readFile(String dir){
  if(dir.isEmpty()){
    Serial.println(":::Error: you must specify a directory.");
    return;
  }
  if(dir[dir.length()-1]=='/'){dir=dir.substring(0,dir.length()-1);}
  String _cd=current_directory[current_directory.length()-1]=='/'?"":"/";
  _cd=current_directory+_cd;
  dir="/"+_cd+dir;
  if(!SD.exists(dir)){Serial.println(":::Error: the file dose not exist.");}
  else{
    File file=SD.open(dir.c_str(),FILE_READ);
    if(!file){Serial.println(":::Error: can't open the file.");}
    else{
      while(file.available()){
        byte buff[1024];
        int i;
        for(i=0;i<1024;++i){
          if(file.available()){buff[i]=file.read();}
          else break;
        }
        for(int j=0;j<i;++j) Serial.write(buff[j]);
        if(i<1023) break;
      }
    }
    file.close();
  }
}
/*
 * write to a file
 * */
void writeFile(String dir,const char*mode,String data,bool printResult=true){
  if(dir.isEmpty()){
    Serial.println(":::Error: you must specify a directory.");
    return;
  }
  if(dir[dir.length()-1]=='/'){dir=dir.substring(0,dir.length()-1);}
  String _cd=current_directory[current_directory.length()-1]=='/'?"":"/";
  _cd=current_directory+_cd;
  dir="/"+_cd+dir;
  if(!SD.exists(dir)&&strcmp(mode,"a")==0){Serial.println(":::Error: directory does not exist.");}
  else{
    File file=SD.open(dir.c_str(),mode);
    if(!file){Serial.println(":::Error: can't open the file.");}
    else{
      for(uint16_t i=0;i<data.length();++i){file.write((uint8_t)data.charAt(i));}
      if(printResult)Serial.println("Done");
    }
    file.close();
  }
}
/*
 * touch: creates a empty file.
 * */
void touch(String dir){writeFile(dir,FILE_WRITE,"");}
/*
 * read a file draw from the SD card.
 * */
void readAndDraw(String dir);
// endregion
// region G-code processing
float g00FeedRate=1000,lastFeedRate=1000;// mm/min
/*
 * step/mm if you multiply this factor by distance it will give you the number of steps.
 * (this factor is for full step)
 * */
uint8_t xStpMm=7,yStpMm=7,zStpMm=7;
/*
 * is it absolute positioning?
 * */
bool isAbsolutePosition=true;
/*
 * The offset for G92
 * */
float xOffset=0,yOffset=0,zOffset=0;
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
        if(params[i].indexOf('x')!=-1){
          x=params[i].substring(1).toFloat()-(isAbsolutePosition?(cpx+xOffset):0);
        }
        else if(params[i].indexOf('y')!=-1){
          y=params[i].substring(1).toFloat()-(isAbsolutePosition?(cpy+yOffset):0);
        }
        else if(params[i].indexOf('z')!=-1){
          z=params[i].substring(1).toFloat()-(isAbsolutePosition?(cpz+zOffset):0);
        }
        else if(params[i].indexOf('f')!=-1){
          f=params[i].substring(1).toFloat();
        }
      }
    }
    float amplitude=getAmplitude(x,y,z);
    f=(upCode=="g01")?f==0?lastFeedRate:f:g00FeedRate;
    uint64_t motionDelay=(amplitude*60)/(f/1000000);
    int32_t xStp=roundf(x*xStpMm*currentStepMode),yStp=roundf(y*yStpMm*currentStepMode),zStp=roundf(z*zStpMm);
    moveSteppers(xStp,yStp,zStp,motionDelay);
    if(upCode=="g28"){setEnableSteppers(false);}
    lastFeedRate=(upCode=="g01")?f:lastFeedRate;
  }
  else if(upCode=="g90"){isAbsolutePosition=true;}
  else if(upCode=="g91"){isAbsolutePosition=false;}
  else if(upCode=="g92"){
    float cpx=0,cpy=0,cpz=0;
    cpx=(float)xcpStp/(float)(xStpMm*16);
    cpy=(float)ycpStp/(float)(yStpMm*16);
    cpz=(float)zcpStp/(float)zStpMm;
    for(int i=0;i<3;++i){
      if(params[i].indexOf('x')!=-1){xOffset=params[i].substring(1).toFloat()-cpx;}
      else if(params[i].indexOf('y')!=-1){yOffset=params[i].substring(1).toFloat()-cpy;}
      else if(params[i].indexOf('z')!=-1){zOffset=params[i].substring(1).toFloat()-cpz;}
    }
  }
    // M codes
  else if(upCode=="m17"){setEnableSteppers(true);}
  else if(upCode=="m18"||upCode=="m84"){setEnableSteppers(false);}
  else if(upCode=="m114"){
    float cpx=0,cpy=0,cpz=0;
    cpx=(float)xcpStp/(float)(xStpMm*16);
    cpy=(float)ycpStp/(float)(yStpMm*16);
    cpz=(float)zcpStp/(float)zStpMm;
    bool isRealPosition=false;
    for(uint8_t i=0;i<4;++i){
      if(params[i].indexOf('r')!=-1){
        isRealPosition=true;
        break;
      }
    }
    if(isRealPosition){
      Serial.print("X:");
      Serial.print(cpx);
      Serial.print(" Y:");
      Serial.print(cpy);
      Serial.print(" Z:");
      Serial.println(cpz);
    }
    else{
      Serial.print("X:");
      Serial.print(cpx+xOffset);
      Serial.print(" Y:");
      Serial.print(cpy+yOffset);
      Serial.print(" Z:");
      Serial.println(cpz+zOffset);
    }
  }
    // MS code this is for microstepping
  else if(upCode=="ms1"){setSteppersMode(1);}
  else if(upCode=="ms2"){setSteppersMode(2);}
  else if(upCode=="ms4"){setSteppersMode(4);}
  else if(upCode=="ms8"){setSteppersMode(8);}
  else if(upCode=="ms16"){setSteppersMode(16);}
    // SD card commands
  else if(upCode=="gtb"){
    gtb();
    return;
  }
  else if(upCode=="gub"){
    gub();
    return;
  }
  else if(upCode=="ls"){
    ls(params[0]);
    return;
  }
  else if(upCode=="cd"){
    cd(params[0]);
    return;
  }
  else if(upCode=="pwd"){
    pwd();
    return;
  }
  else if(upCode=="mkdir"){
    mkdir(params[0]);
    return;
  }
  else if(upCode=="rm"){
    rm(params[0]);
    return;
  }
  else if(upCode=="rf"){
    readFile(params[0]);
    return;
  }
  else if(upCode=="touch"){
    touch(params[0]);
    return;
  }
  else if(upCode=="write"){
    String data="";
    unsigned long lastRead=millis();
    while(millis()-lastRead<=1000){
      int rc=Serial.read();
      if(rc!=-1){
        data+=(char)rc;
        lastRead=millis();
      }
    }
    writeFile(params[0],FILE_WRITE,data);
    return;
  }
  else if(upCode=="draw"){
    readAndDraw(params[0]);
    return;
  }
    // if command doses not exist.
  else{
    Serial.println(":::Error: Unknown command.");
    return;
  }
  Serial.println("ok");
}
// endregion
// region read & draw
void readAndDraw(String dir){
  if(dir.isEmpty()){
    Serial.println(":::Error: you must specify a directory.");
    return;
  }
  if(dir[dir.length()-1]=='/'){dir=dir.substring(0,dir.length()-1);}
  String _cd=current_directory[current_directory.length()-1]=='/'?"":"/";
  _cd=current_directory+_cd;
  dir="/"+_cd+dir;
  if(!SD.exists(dir)){Serial.println(":::Error: the file dose not exist.");}
  else{
    File file=SD.open(dir.c_str(),FILE_READ);
    if(!file){Serial.println(":::Error: can't open the file.");}
    else{
      while(file.available()){
        String upCode="";
        String params[4]={"","","",""};
        bool isEndOfLine;
        while(1){
          int16_t ci=file.read();
          uint64_t start=micros();
          while(ci==40||ci==59){if(file.read()==10||micros()-start>=100000){break;}}
          if(ci==-1)continue;
          isEndOfLine=ci==10;
          if(ci==10||ci==32)break;
          else upCode+=(char)ci;
        }
        upCode.toLowerCase();
        while(!isEndOfLine){
          int16_t ci=file.read();
          uint64_t start=micros();
          while(ci==40||ci==59){if(file.read()==10||micros()-start>=100000){break;}}
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
          int16_t ci=file.read();
          uint64_t start=micros();
          while(ci==40||ci==59){if(file.read()==10||micros()-start>=100000){break;}}
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
          int16_t ci=file.read();
          uint64_t start=micros();
          while(ci==40||ci==59){if(file.read()==10||micros()-start>=100000){break;}}
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
          int16_t ci=file.read();
          uint64_t start=micros();
          while(ci==40||ci==59){if(file.read()==10||micros()-start>=100000){break;}}
          if(ci==40||ci==59){
            isEndOfLine=true;
            break;
          }
          if(ci==-1)continue;
          isEndOfLine=ci==10;
          if(ci==10||ci==32)break;
          else params[3]+=(char)ci;
        }
        for(uint8_t i=0;i<4;++i){params[i].toLowerCase();}
        if(upCode.charAt(upCode.length()-1)==(char)13)
          upCode=upCode.substring(0,upCode.length()-1);
        Serial.print(upCode);
        for(uint8_t i=0;i<4;i++)Serial.print(" "+params[i]);
        doGCode(upCode,params);
      }
      Serial.println("Done");
    }
    file.close();
  }
}
// endregion
// region joystick manager!
TaskHandle_t jmTask;
void joystickManagent(void*parameter){
  pinMode(27,INPUT);
  pinMode(36,INPUT);
  pinMode(39,INPUT);
  for(;;){
    int8_t x=-round(((analogRead(36)-2075)/2000));
    int8_t y=round(((analogRead(39)-2075)/2000));
    if(x!=0||y!=0){
      moveSteppers(x*currentStepMode,y*currentStepMode,0,8000);
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
  setupSteppers();
  xTaskCreatePinnedToCore(joystickManagent,"jm",1000,NULL,1,&jmTask,0);
  SD_setup();
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
  for(uint8_t i=0;i<4;++i){params[i].toLowerCase();}
  doGCode(upCode,params);
}
// endregion
