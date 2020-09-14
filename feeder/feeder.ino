#define MAX7219CLK 5
#define MAX7219CS 6
#define MAX7219DIN 7
#define PUSH_BUTTON 13
#define FEED_DETECTION_SWITCH 2
#define PWM_OUT_PIN 3
#define PWM_SPEED 160

const unsigned long ONE_DAY = 86400000; // in milliseconds
const short RESET_TIME_THRESHOLD = 4000; // hold button for > 4 seconds
const short FEEDBACK_SWITCH_THRESHOLD = 100; // hold button for > 0.1 seconds

byte portionsRequired = 1;
byte portionsFed = 0;

unsigned long lastFeedTime = 0;
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
boolean feeding = false;

/* Variables for Push Button */
int statePb;                            // the current reading from the input pin
int lastStatePb = LOW;                  // the previous reading from the input pin
int startPressedPb = 0;                 // the moment the button was pressed
int endPressedPb = 0;                   // the moment the button was released
unsigned long lastDebounceTimePb = 0;   // the last time the output pin was toggled
/* End */

/* Variables for Motor Switch */
int stateSw;
int startPressedSw = 0;
int endPressedSw = 0;

int lastStateSw = LOW;
unsigned long lastDebounceTimeSw = 0;
/* End */

void setup() {
  Serial.begin(9600);
  pinMode(2,INPUT);
  
  MAX7219init();
  MAX7219brightness(2);

  MAX7219senddata(5,15);
  MAX7219senddata(6,15);
  MAX7219senddata(7,15);
  MAX7219senddata(8,portionsRequired);
}

void loop() {
  displayNextFeed();
  checkButton();
  if (lastFeedTime != 0) {
    if ((millis() - lastFeedTime) >= ONE_DAY) {
      feedFish();
    }
    if (feeding) {
      checkFeedOffSwitch();
    }
  }
  checkFeedOffSwitch();
}

void displayNextFeed() {
  if (lastFeedTime == 0) {
    displayNumber(0);
  } else {
    unsigned int nextFeedCountdown = (ONE_DAY - (millis() - lastFeedTime))/60000; // display in minutes
    displayNumber(nextFeedCountdown);
  }
}

void displayNumber(unsigned long t) {
  unsigned long k=t;
  for(int i=1;i<5;i++){
    MAX7219senddata(i,k%10);
    k=k/10;
  }
}

void checkButton() {
  int LOOP_TIME = millis();
  int reading = digitalRead(PUSH_BUTTON);
  if (reading != lastStatePb) {
    lastDebounceTimePb = millis();
  }
  
  if ((LOOP_TIME - lastDebounceTimePb) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current statePb:
  
    // if the button statePb has changed:
    if (reading != statePb) {
      statePb = reading;
      if (statePb == HIGH) {
        startPressedPb = LOOP_TIME;
      } else {
        endPressedPb = LOOP_TIME;
        int holdTimePb = endPressedPb - startPressedPb;
        if (holdTimePb >= RESET_TIME_THRESHOLD) {
          feedFish();
        } else if (holdTimePb <= 1000) {
          increasePortionsRequired();
        }
      }
    }
  }
  lastStatePb = reading;
}

void checkFeedOffSwitch() {
  int LOOP_TIME = millis();
  int reading = digitalRead(FEED_DETECTION_SWITCH);
  if (reading != lastStateSw) {
    lastDebounceTimeSw = millis();
  }

  if (LOOP_TIME - lastDebounceTimeSw > debounceDelay) {
    if (reading != stateSw) {
      stateSw = reading;
      if (stateSw == HIGH) {
        startPressedSw = LOOP_TIME;
      } else {
        endPressedSw = LOOP_TIME;
        if (endPressedSw - startPressedSw >= FEEDBACK_SWITCH_THRESHOLD) {
          portionsFed++;
          if (portionsFed >= portionsRequired) {
            analogWrite(PWM_OUT_PIN, 0);
            feeding = false;
            portionsFed = 0;
          }
        }
      }
    }
  }
  lastStateSw = reading;
}

void feedFish() {
  lastFeedTime = millis();
  feeding = true;
  analogWrite(PWM_OUT_PIN, PWM_SPEED);
}

void increasePortionsRequired() {
  portionsRequired++;
  portionsRequired = portionsRequired % 10;
  if (portionsRequired == 0) {
    portionsRequired = 1;
  }
  MAX7219senddata(8,portionsRequired);
}

void MAX7219brightness(byte b){  //0-15 is range high nybble is ignored
  MAX7219senddata(10,b);        //intensity  
}

void MAX7219init(){
  pinMode(MAX7219DIN,OUTPUT);
  pinMode(MAX7219CS,OUTPUT);
  pinMode(MAX7219CLK,OUTPUT);
  digitalWrite(MAX7219CS,HIGH);   //CS off
  digitalWrite(MAX7219CLK,LOW);   //CLK low
  MAX7219senddata(15,0);        //test mode off
  MAX7219senddata(12,1);        //display on
  MAX7219senddata(9,255);       //decode all digits
  MAX7219senddata(11,7);        //scan all
  for(int i=1;i<9;i++){
    MAX7219senddata(i,0);       //blank all
  }
}

void MAX7219senddata(byte reg, byte data){
  digitalWrite(MAX7219CS,LOW);   //CS on
  for(int i=128;i>0;i=i>>1){
    if(i&reg){
      digitalWrite(MAX7219DIN,HIGH);
    }else{
      digitalWrite(MAX7219DIN,LOW);      
    }
  digitalWrite(MAX7219CLK,HIGH);   
  digitalWrite(MAX7219CLK,LOW);   //CLK toggle    
  }
  for(int i=128;i>0;i=i>>1){
    if(i&data){
      digitalWrite(MAX7219DIN,HIGH);
    }else{
      digitalWrite(MAX7219DIN,LOW);      
    }
  digitalWrite(MAX7219CLK,HIGH);   
  digitalWrite(MAX7219CLK,LOW);   //CLK toggle    
  }
  digitalWrite(MAX7219CS,HIGH);   //CS off
}
