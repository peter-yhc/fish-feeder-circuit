#define MAX7219DIN 4
#define MAX7219CS 5
#define MAX7219CLK 6

boolean DEBUG = true;

short ONE_DAY = 1440;
short RESET_TIME_THRESHOLD = 4000;

byte foodSize = 1;
int lastFeedTime = -1;

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int startPressed = 0;    // the moment the button was pressed
int endPressed = 0;      // the moment the button was released
int holdTime = 0;        // how long the button was hold

void setup() {
  Serial.begin(9600);
  pinMode(2,INPUT);
  
  MAX7219init();
  MAX7219brightness(2);

  MAX7219senddata(5,15);
  MAX7219senddata(6,15);
  MAX7219senddata(7,15);
  MAX7219senddata(8,foodSize);
}

void loop() {
  displayNextFeed();
  checkButton();
}

void displayNextFeed() {
  if (lastFeedTime == -1) {
    displayNumber(0);
  } else {
    int nextFeedCountdown = ONE_DAY - (millis() - lastFeedTime)/1000;
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
  int reading = digitalRead(2);
  if (reading != lastButtonState) {
    lastDebounceTime = LOOP_TIME;
  }
  
  if ((LOOP_TIME - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
  
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        stepFoodSize();
        startPressed = LOOP_TIME;
      } else {
        endPressed = LOOP_TIME;
        holdTime = endPressed - startPressed;
        if (holdTime >= RESET_TIME_THRESHOLD) {
          lastFeedTime = LOOP_TIME; // set motor threshold high
        }
      }
    }
  }
  
  lastButtonState = reading;
}

void stepFoodSize() {
  foodSize++;
  foodSize = foodSize % 6;
  if (foodSize == 0) {
    foodSize = 1;
  }
  MAX7219senddata(8,foodSize);
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
