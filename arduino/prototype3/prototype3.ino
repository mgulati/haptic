//variables used inside pixel
const int _posTop = 1000;
const int _posBottom = 0;
unsigned int _posStop = 300;
unsigned int _posFlush = 400;

unsigned int BUZZ_THRESHOLD = 100;
unsigned int MOTOR_MIN = 150;

const int PWM_HIGH = 255;
const int PWM_LOW = 0;

const unsigned int NUM_PRESETS = 3;
double presets[NUM_PRESETS][3] = {{10, 0.2, 0.02}, {10, 0.3, 0.02}, {0, 0, 0}};

unsigned int touchSwap = 900;

struct pixel {
  const int _ledR;
  const int _ledG;
  const int _ledB;
  const int _ledGround;
  const int _dirDown;
  const int _dirUp;
  const int _motor;
  const int _touchIn;
  const int _analogPos;

  int actualPos;
  int desiredPos;
  int lastPos;
  double kI;
  double kP;
  double kD;
  int integral;
  int allowSlide;
  int action;

  int touchState;
  int touchCount;
  int touchTemp;
  int touchAwait;

  int red;
  int green;
  int blue;

  pixel(int analogPos, int touchIn, int motor, int dirUp, int dirDown, int ledR, int ledGround, int ledG, int ledB)
    : _ledR(ledR), _ledG(ledG), _ledB(ledB), _ledGround(ledGround)
    , _dirDown(dirDown), _dirUp(dirUp), _motor(motor)
    , _touchIn(touchIn), _analogPos(analogPos)
    , actualPos(0), lastPos(0), action(0)
    , integral(0), allowSlide(0)
    , touchState(0), touchCount(0)
  {
    setPIDPreset(0);
    setColor(0, 0, 0);
    setTarget(800);
  
    pinMode(_analogPos, INPUT);
    pinMode(_touchIn, INPUT);
    pinMode(_motor, OUTPUT);
    pinMode(_dirDown, OUTPUT);
    pinMode(_dirUp, OUTPUT);
    pinMode(_ledGround, OUTPUT);
    pinMode(_ledR, OUTPUT);
    pinMode(_ledG, OUTPUT);
    pinMode(_ledB, OUTPUT);
  }

  void setPIDPreset(int preset) {
    setPIDValues(presets[preset][0], presets[preset][2], presets[preset][1]);
  }

  void setPIDValues(double p, double i, double d) {
    kP = p;
    kI = i;
    kD = d;
  }

  void setColor(int R, int G, int B) {
    red = R;
    green = G;
    blue = B;
  }

  void setTarget(int target) {
    desiredPos = target;
  }

  void readPosition() {
    lastPos = actualPos;
    actualPos = map(analogRead(_analogPos), 0, 1023, _posBottom, _posTop);
    actualPos = constrain(actualPos, _posBottom, _posTop);
  }

  void flushTouchPin() {
    touchState = touchCount;
    touchCount = 0;
    pinMode(_touchIn, OUTPUT);
    digitalWrite(_touchIn, LOW);
    pinMode(_touchIn, INPUT);
  }

  void readTouchState() {
    if (digitalRead(_touchIn) == LOW) touchCount++;
  }

  void calculatePIDAction() {
    //if (touchState == 1 && allowSlide == 1) desiredPos = actualPos;
    int error = desiredPos - actualPos;
    int derivative = lastPos - actualPos;

    integral = constrain((integral + error), -20000, 20000);
    if (derivative == 0 && error == 0) integral = 0;

    action = (error * kP) + (integral * kI) + (derivative * kD);
    action = constrain(map(action, -1000, 1000, -PWM_HIGH, PWM_HIGH), -PWM_HIGH,  PWM_HIGH);
    if (abs(action) < BUZZ_THRESHOLD) action = PWM_LOW;
  }

  void setDirection() {
    if (action > 0) {
      digitalWrite(_dirUp, HIGH);
      digitalWrite(_dirDown, LOW);
    } else {
      digitalWrite(_dirUp, LOW);
      digitalWrite(_dirDown, HIGH);
    }
  }

  void moveMotor() {
    setDirection();
    int pwmWrite = map(abs(action), PWM_LOW, PWM_HIGH, MOTOR_MIN, PWM_HIGH);
    if (action == PWM_LOW) pwmWrite = PWM_LOW;
    analogWrite(_motor, pwmWrite);
  }

  void serialPrintPixel(int prependId) {
    Serial.print(prependId);
    Serial.print(",");
    Serial.print(touchState);
    Serial.print(",");
    Serial.print(actualPos);
    Serial.print(",");
    Serial.print(desiredPos);
    Serial.print(",");
    Serial.print(action);
    Serial.println("");
  }

  double speedAtPWM(int testAction) {
    double t_kP = kP;
    double t_kI = kI;
    double t_kD = kD;
    setPIDPreset(2);

    //move to opposite end
    if (testAction > 0) action = -255;
    if (testAction < 0) action = 255;
    moveMotor();
    delay(1000);

    //let it settle
    action = 0;
    moveMotor();
    delay(100);

    //starting values
    action = testAction;
    readPosition();
    unsigned int startPoint = actualPos;
    unsigned long startTime = millis();
    action = testAction;
    moveMotor();

    if (testAction > 0) {
      while (actualPos < 850 && ((millis() - startTime) < 2000)) {
        readPosition();
      }
    }
    if (testAction < 0) {
      while (actualPos > 350 && ((millis() - startTime) < 2000)) {
        readPosition();
      }
    }

    unsigned long endTime = millis();
    unsigned int endPoint = actualPos;
    
    action = 0;
    moveMotor();

    unsigned long duration = endTime - startTime;
    int distance = endPoint - startPoint;
    double speed = distance / duration;

    setPIDValues(t_kP, t_kI, t_kD);
    return speed;
  }

};

const int numPixels = 9;
// {analogPos, touchIn, motor, dirUp, dirDown, ledR, ledGround, ledG, ledB}
pixel pixels[numPixels] = {
  {A14, 42, 7, 27, 26, 45, A4, 44, 46}, //new 0
  {A13, 53, 2, 29, 28, 45, A0, 44, 46}, //new 1
  {A10, 48, 5, 32, 35, 4, A1, 9, 13}, //new 2
  {A15, 41, 8, 27, 24, 4, A3, 9, 13}, //new 3
  {A9, 49, 3, 30, 31, 45, A2, 44, 46}, //new 4
  {A7, 50, 12, 37, 33, 45, A3, 44,46}, //new 5
  {A11, 47, 10, 38, 39, 45, A1, 44, 46}, //new 6
  {A12, 40, 6, 23, 22, 4, A2, 9, 13}, //new 7
  {A8, 51, 11, 34, 36, 4, A0, 9, 13} //new 8 *broken*
};

const unsigned int BUFFER_SIZE = 20;
char inData[BUFFER_SIZE];

unsigned int index = 0;
unsigned int serialTimer = 0;
unsigned int STIMER_THRESHOLD = 11;

const unsigned int ledPairs[5][2] = {{1, 8}, {2, 6}, {4, 7}, {3, 5}, {0, 0}};
unsigned int ledCounter = 0;
unsigned int ledDelay = 8;
unsigned int currentPair = 0;

unsigned int pixelCounter = 0;
unsigned int pixelPrintCounter = 0;
String debugPixels = "111111111";

const unsigned int touchOut = 52;
unsigned int touchCounter = 0;

void setup() {
  Serial.begin(115200);

  pinMode(touchOut, OUTPUT);
  digitalWrite(touchOut, HIGH);
  //  assignmentTest();
  //  startupAnimation();
  //timers for pwm
  /*TCCR1B = (TCCR1B & 0xF8) | 0x05;
  TCCR2B = (TCCR2B & 0xF8) | 0x07;
  TCCR3B = (TCCR3B & 0xF8) | 0x05;
  TCCR4B = (TCCR4B & 0xF8) | 0x05;*/
}

void loop() {

  pixelCounter++;
  if (pixelCounter == numPixels) pixelCounter = 0;
  if (debugPixels[pixelCounter] == '0') return;

  touchCounter++;
  if (touchCounter == touchSwap) {
    for (int i = 0; i < numPixels; i++) {
      if (debugPixels[i] == '1') pixels[i].flushTouchPin();
    }
    digitalWrite(touchOut, HIGH);
    touchCounter = 0;
  };

  ledCounter++;
  if (ledCounter > (5 * ledDelay - 1)) ledCounter = 0;
  if (ledCounter % ledDelay == 0) writeLEDPair();

  for (int i = 0; i < 5; i++) {
    serialRead();
  }

  pixels[pixelCounter].readPosition();
  pixels[pixelCounter].readTouchState();

  pixels[pixelCounter].calculatePIDAction();
  pixels[pixelCounter].moveMotor();

  serialTimer++;
  if (serialTimer > STIMER_THRESHOLD) {
    while (debugPixels[pixelPrintCounter] == '0') {
      pixelPrintCounter++;
      if (pixelPrintCounter == numPixels) pixelPrintCounter = 0;
    }
    pixels[pixelPrintCounter].serialPrintPixel(pixelPrintCounter);
    serialTimer = 0;
    pixelPrintCounter++;
    if (pixelPrintCounter == numPixels) pixelPrintCounter = 0;
  }
}

void writeLEDPair() {
  digitalWrite(pixels[ledPairs[currentPair][0]]._ledGround, HIGH);
  currentPair = ledCounter / ledDelay;
  for (int i = 0; i < 2; i++) {
    int id = ledPairs[currentPair][i];
    analogWrite(pixels[id]._ledR, pixels[id].red);
    analogWrite(pixels[id]._ledG, pixels[id].green);
    analogWrite(pixels[id]._ledB, pixels[id].blue);
  }
  digitalWrite(pixels[ledPairs[currentPair][0]]._ledGround, LOW);
}

//read one command from serial interface and react accordingly
void serialRead() {
  while (Serial.available() > 0 && Serial.peek() != 10)
  {
    if (index > BUFFER_SIZE - 1) index = 0; // One less than the size of the array
    inData[index] = Serial.read(); // Read a character, store it
    index++; // Increment where to write next
  }

  if (strlen(inData) != 0 && Serial.peek() == 10) {
    Serial.read();
    int id = String(inData).substring(0, 1).toInt();
    if (id <= numPixels - 1) {

      //Set color, #C255000255
      if (inData[1] == 'C') {
        pixels[id].red = constrain(String(inData).substring(2, 5).toInt(), 0, 255);
        pixels[id].green = constrain(String(inData).substring(5, 8).toInt(), 0, 255);
        pixels[id].blue = constrain(String(inData).substring(8, 11).toInt(), 0, 255);
      }

      //set desired position #P1000
      if (inData[1] == 'P') pixels[id].setTarget(map(constrain(String(inData).substring(2, 6).toInt(), 0, 1000), 0, 1000, 300, 1000));

      //set allowsliding on capacitive touch #A1
      if (inData[1] == 'A') pixels[id].allowSlide = constrain(String(inData).substring(2, 3).toInt(), 0, 1);

      //pid presets #S2
      if (inData[1] == 'S') pixels[id].setPIDPreset(constrain(String(inData).substring(2, 3).toInt(), 0, NUM_PRESETS));

      //test pixel at speed and print result, #T255,
      if (inData[1] == 'T') {
        int testSpeed = constrain(String(inData).substring(2, 6).toInt(), -255, 255);
        Serial.print("T,");
        Serial.print(id);
        Serial.print(',');
        Serial.print(testSpeed);
        Serial.print(',');
        Serial.println(pixels[id].speedAtPWM(testSpeed));
      }

    } else {
      //set debug pixel, not currently used, #D
      if (inData[1] == 'D') {
        debugPixels = String(inData).substring(2, 11);
        for (int i = 0; i < numPixels; i++) {
          if (debugPixels[i] == '0') {
            pixels[i].setColor(0,0,0);
            pixels[i].setTarget(_posStop);
            pixels[i].readPosition();
            pixels[i].calculatePIDAction();
            pixels[i].moveMotor();
            delay(200);
            pixels[i].action = 0;
            pixels[i].moveMotor();
          }
        }
      }

      //change led Delay
      if (inData[1] == 'L') ledDelay = (constrain(String(inData).substring(2, 6).toInt(), 1, 9999));

      //change serial treshold
      if (inData[1] == 'S') STIMER_THRESHOLD = (constrain(String(inData).substring(2, 6).toInt(), 1, 9999));
    }

    for (int i = 0; i < BUFFER_SIZE; i++) {
      inData[i] = 0;
    }

    index = 0;
  }
}

void animateCorners(int new_desiredPos, int new_red, int new_green, int new_blue) {
  pixels[0].desiredPos = new_desiredPos;
  pixels[2].desiredPos = new_desiredPos;
  pixels[6].desiredPos = new_desiredPos;
  pixels[8].desiredPos = new_desiredPos;
  pixels[0].setColor = (new_red,new_green,new_blue);
  pixels[2].setColor = (new_red,new_green,new_blue);
  pixels[6].setColor = (new_red,new_green,new_blue);
  pixels[8].setColor = (new_red,new_green,new_blue);

}

void animateLateral(int new_desiredPos, int new_red, int new_green, int new_blue) {
  pixels[1].desiredPos = new_desiredPos;
  pixels[3].desiredPos = new_desiredPos;
  pixels[5].desiredPos = new_desiredPos;
  pixels[7].desiredPos = new_desiredPos;
  pixels[1].setColor = (new_red,new_green,new_blue);
  pixels[3].setColor = (new_red,new_green,new_blue);
  pixels[5].setColor = (new_red,new_green,new_blue);
  pixels[7].setColor = (new_red,new_green,new_blue);
}

void animateCenter(int new_desiredPos, int new_red, int new_green, int new_blue) {
  pixels[4].desiredPos = new_desiredPos;
  pixels[4].setColor(new_red, new_green, new_blue);
}

void updatePixels(unsigned long waitTime) {
  loopStart = millis();
  unsigned int loopPixelCount = 0;
  while (millis()-loopStart < waitTime) {
    if (debugPixel[loopPixelCount] == "1") {
      pixel[loopPixelCount].readPosition();
      pixel[loopPixelCount].calculatePIDAction();
      pixel[loopPixelCount].moveMotor();
    }
    loopPixelCount++;
    if (loopPixelCount == numPixels) loopPixelCount = 0;
  }
}

void startupAnimation() {

  animateCorners(300,0,0,255);
  animateLateral(450,0,255,0);
  animateCenter(600,255,0,0);
  updatePixels(300);

  for (int a = 0; a < 4; a++) {
    animateCorners(200 + a*100,0,0,255);
    animateLateral(400 + a*100,0,255,0);
    animateCenter(600 + a*100,255,0,0);
    updatePixels(100);
  }

  animateCorners(600,0,0,255);
  animateLateral(450,255, 0, 0);
  animateCenter(300,0, 255, 0);
  updatePixels(300);

  animateCorners(300,0,0,0);
  animateLateral(300,0, 0, 0);
  animateCenter(300,0, 0, 0);
  updatePixels(300);
}

//
//void assignmentTest() {
//  for (int i = 0; i < 9; i++ ) {
//    pixels[i].desiredPos = i*100;
//    pixels[i].red = 0;//(i == 0 || i == 3 || i == 6) ? 255 : 0;
//    pixels[i].green = 255;//(i == 1 || i == 4 || i == 7) ? 255 : 0;
//    pixels[i].blue = 0;//(i == 2 || i == 5 || i == 8) ? 255 : 0;
//  }
//}
//
//void singleTest(int i) {
//  for (int a = 0; a < 9; a++) {
//    pixels[a].desiredPos = 0;
//    pixels[a].setColor(0,0,0);
//  }
//  pixels[i].desiredPos = 1000;
//  pixels[i].setColor(255,255,255);
//}
