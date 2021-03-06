/*
1 - speed
2 - direction 
3 - direction

4 - direction 
5 - direction
6 - speed

7 - direction
8 - direction
9 - speed

10 - speed
11 - direction
12 - direction

analog channels
touch - potentiometer
01 - 1 (456)
23 - 2 (123)
45 - 4 (10 11 12)
67 - 7 (789)

selector pins:
S0 - 7
S1 - 6
S2 - 5
S3 - 4

*/


#ifndef prog_uint8_t
#define prog_uint8_t const uint8_t
#endif

#include <Tlc5940.h>
#include <tlc_animations.h>
#include <tlc_config.h>
#include <tlc_fades.h>
#include <tlc_progmem_utils.h>
#include <tlc_servos.h>
#include <tlc_shifts.h>

struct pixel {
  int dirDown;
  int dirUp;
  
  int ledR;
  int ledG;
  int ledB;
  int motor;
  int touchIn;
  
  int actualPos;
  int desiredPos;
  int lastPos;
  int analogPos;
  
  double kI;
  double kP;
  double kD;
  
  int integral;
  int derivative;
  
  int allowSlide;
  int touchState;
  int touchCount;
  
  int red;
  int blue;
  int green;
};

const int numPixels = 4;
struct pixel pixels[numPixels];

int S0 = 7;
int S1 = 6;
int S2 = 5;
int S3 = 4;
int analogMux = A0;

const int BUFFER_SIZE = 20;
char inData[BUFFER_SIZE];

const int _posTop = 1000;
const int _posBottom = 0;

const int BUZZ_THRESHOLD = 150;
const int MOTOR_MIN = 2700;

const int PWM_HIGH = 4095;
const int PWM_LOW = 0;

int index = 0;
unsigned int serialTimer = 0;
const int STIMER_THRESHOLD = 20;

const int NUM_PRESETS = 1;
int presets[NUM_PRESETS][3] = {{0.6, 0.2, 0.02}};

void setup() {
  Serial.begin(115200);
  Tlc.init();
  Tlc.clear();
  
  //Common initializations 
  for (int i = 0; i < numPixels; i++) {
    pixels[i].desiredPos = 500;
    pixels[i].integral = 0;
    pixels[i].derivative = 0;
    pixels[i].touchState = 0;
    pixels[i].touchCount = 0;
    pixels[i].kP = 0.6;
    pixels[i].kD = 0.2;
    pixels[i].kI = 0.02;
  }

  //MOTOR 1 (pixels[0])
  pixels[0].motor = 1;
  pixels[0].dirDown = 3;
  pixels[0].dirUp = 2;
  pixels[0].analogPos = 2;
  
  //MOTOR 2 (pixels[1])
  pixels[1].motor = 6;
  pixels[1].dirDown = 5;
  pixels[1].dirUp = 4;
  pixels[1].analogPos = 1;
  
  //MOTOR 3
  pixels[2].motor = 9;
  pixels[2].dirDown = 7;
  pixels[2].dirUp = 8;
  pixels[2].analogPos = 7;
 
  //MOTOR 4
  pixels[3].motor = 10;
  pixels[3].dirDown = 11;
  pixels[3].dirUp = 12;
  pixels[3].analogPos = 4;
  
  pinMode(analogMux, INPUT);
  pinMode(S0,OUTPUT);
  pinMode(S1,OUTPUT);
  pinMode(S2,OUTPUT);
  pinMode(S3,OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  serialRead();
  serialTimer++;
  for (int i = 0; i < numPixels; i++) {
    
    
    readPosition(i);
    //readTouchState(i);
    
    int action = calculatePIDAction(i);
    //padPrint(action, 3);
    //Serial.print("     ");
    if (serialTimer > STIMER_THRESHOLD) {
      serialPrintPixel(i);
    }
    moveMotor(i, action);
  }
  if (serialTimer > STIMER_THRESHOLD) serialTimer = 0;
  Tlc.update();
}

void serialPrintPixel(int i) {
    Serial.print(i);
    Serial.print(",");
    Serial.print(pixels[i].touchState);
    Serial.print(",");
    Serial.print(pixels[i].actualPos);
    Serial.print(",");
    Serial.print(pixels[i].desiredPos);
   Serial.println("");
}

//set channel on analog mux to read from
void setAnalogMux(int channel) {
  digitalWrite(S3, HIGH && (channel & B00001000));
  digitalWrite(S2, HIGH && (channel & B00000100));
  digitalWrite(S1, HIGH && (channel & B00000010));
  digitalWrite(S0, HIGH && (channel & B00000001));
}

//read value from certain channel of analog multiplexer
int analogMuxRead(int channel) {
  setAnalogMux(channel);
  return map(analogRead(analogMux),0,1023,0,1000);
}

//read current position of slider and update its respective stored value
void readPosition(int pixel) {
  pixels[pixel].actualPos = map(analogMuxRead(pixels[pixel].analogPos),0,1023,_posBottom,_posTop);
  pixels[pixel].actualPos = constrain(pixels[pixel].actualPos, _posBottom, _posTop);
}

void readTouchState(int pixel) {
  //NOT TESTED, NEEDS FURTHER IMPLEMENTATION
  if (digitalRead(pixels[pixel].touchIn) == HIGH) {
    pixels[pixel].touchCount += 1;
    if (pixels[pixel].touchCount > 50) {
      pixels[pixel].touchState = 1;
    }
  } else {
    pixels[pixel].touchCount = 0;
    pixels[pixel].touchState = 0;
  }
}

//returns a motor speed, all pid controls
int calculatePIDAction(int pixel) {
  //if (pixels[channel].touchState == 1 && pixels[channel].allowSlide == 1) pixels[channel].desiredPos = pixels[channel].actualPos;
 
  int error = pixels[pixel].desiredPos - pixels[pixel].actualPos;
  
  pixels[pixel].integral = constrain((pixels[pixel].integral + error), -20000, 20000);
  
  if (pixels[pixel].derivative == 0 && error == 0) {
    pixels[pixel].integral = 0;
  }
    
//  if (pixels[pixel].integral > PWM_HIGH) pixels[pixel].integral = PWM_HIGH;
//  if (pixels[pixel].integral < -PWM_HIGH) pixels[pixel].integral = -PWM_HIGH;

  pixels[pixel].derivative = pixels[pixel].lastPos - pixels[pixel].actualPos;
  pixels[pixel].lastPos = pixels[pixel].actualPos;
  
  //taken out integral acction for now )
  double drive = (error*pixels[pixel].kP) + (pixels[pixel].integral*pixels[pixel].kI) + (pixels[pixel].derivative*pixels[pixel].kD);
  
  int motorSpeed = constrain(map(drive,-500, 500, PWM_HIGH, -PWM_HIGH), -PWM_HIGH,  PWM_HIGH);
  if (abs(motorSpeed) < BUZZ_THRESHOLD) {
    motorSpeed = 0;
  }
 
  return motorSpeed;
}

//set Tlc (pwm multiplexer) channel value. Note: changes won't take effect until Tlc.update() is called
void setPWMValue(int channel, int value) {
  //this thing is actually reverse
  Tlc.set(channel, PWM_HIGH-value);
}

//set the direction of the motor according to its dirUp and dirDown pins. dir=1 indicates upwards
void setDirection(int pixel, int action) {
  if (action > 0) {
     setPWMValue(pixels[pixel].dirUp, PWM_HIGH);    
     setPWMValue(pixels[pixel].dirDown, PWM_LOW);
  } else {
    setPWMValue(pixels[pixel].dirUp, PWM_LOW);
    setPWMValue(pixels[pixel].dirDown, PWM_HIGH); 
  }
}

void setPWMPreset(int i, int preset) {
  pixels[i].kP = presets[preset][0];
  pixels[i].kD = presets[preset][1];
  pixels[i].kI = presets[preset][2];
}

void moveMotor(int pixel, int action) {
  setDirection(pixel, action);
  int pwmWrite = map(abs(action),PWM_LOW, PWM_HIGH, MOTOR_MIN, PWM_HIGH);
  if (action == PWM_LOW) pwmWrite = PWM_LOW;
  setPWMValue(pixels[pixel].motor, pwmWrite);
}

//read one command from serial interface and react accordingly
void serialRead() {
  while (Serial.available() > 0 && Serial.peek() != 10)
  {
    if(index > BUFFER_SIZE - 1) index = 0;// One less than the size of the array
    inData[index] = Serial.read(); // Read a character, store it
    index++; // Increment where to write next
  }
  
  if (strlen(inData) != 0 && Serial.peek() == 10) {
    Serial.read();
    int id = String(inData).substring(0,1).toInt();
    if (id > numPixels - 1) return;
    
    if (inData[1] == 'C') {
      pixels[id].red = constrain(String(inData).substring(2,5).toInt(),0,255);
      pixels[id].green = constrain(String(inData).substring(5,8).toInt(),0,255);
      pixels[id].blue = constrain(String(inData).substring(8,11).toInt(),0,255);
    }
    
    if (inData[1] == 'P') {
      pixels[id].desiredPos = constrain(String(inData).substring(2,6).toInt(),0,1000);
    }
    
    if (inData[1] == 'A') {
      pixels[id].allowSlide = constrain(String(inData).substring(2,3).toInt(),0,1);
    }
    
    if (inData[1] == 'S') {
      setPWMPreset(id, constrain(String(inData).substring(2,3).toInt(),0, NUM_PRESETS));
    }
        
    for (int i=0;i<BUFFER_SIZE;i++) {
      inData[i]=0;
    }
    index = 0;
  }
}

void padPrint(int value, int width)
{
  // pads values with leading zeros to make the given width
  char valueStr[6]; // large enough to hold an int
  itoa (value, valueStr, 10);
  int len = strlen(valueStr);
  if(len < width){
    len = width-len;
    while(len--)
     Serial.print(' ');
  }
 Serial.print(valueStr);   
}