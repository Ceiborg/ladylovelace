#include <Arduino.h>
#include "LedControl_SW_SPI.h"

// constants
const int RIGHT_BTN_PIN = 8;
const int LEFT_BTN_PIN = 9;
const int GREEN_LED_PIN = 10;
const int RED_LED_PIN = 3;
const int RELAY0_PIN = A3;
const int RELAY1_PIN = A4;
const int DEBOUNCE_TIME = 10;
const int LONG_PRESS_TIME = 2000;
const int REST_TIME = 10000;
const int TIME_TO_PANIC = 1500;

// state
enum state_t {
  MANUAL,
  RIGHT_ENTER,
  RIGHT_UPDATE,
  RIGHT_EXIT,
  LEFT_ENTER,
  LEFT_UPDATE,
  LEFT_EXIT,
  PANIC
};
state_t state = MANUAL;
bool automatic = false;
bool rightBtnDown = false;
bool leftBtnDown = false;
bool rightBtnPressed = false;
bool leftBtnPressed = false;
bool rightBtnReleased = false;
bool leftBtnReleased = false;
long rightBtnPressedAt = 0;
long leftBtnPressedAt = 0;
bool rightBtnLongPress = false;
bool leftBtnLongPress = false;  
long startedMovingAt = 0;
long switchedAt = 0;

byte ceiborg[]={ 
  B01111110,
  B01001111,
  B01001111,
  B01111111,
  B01111111,
  B00111111,
  B00000011,
  B00000001,
};




LedControl_SW_SPI lc = LedControl_SW_SPI();

// function declarations
void spin( int s);
void scan_inputs(long now);

void setup() {  
  pinMode(RIGHT_BTN_PIN, INPUT_PULLUP);
  pinMode(LEFT_BTN_PIN , INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN  , OUTPUT);
  pinMode(RELAY0_PIN   , OUTPUT);
  pinMode(RELAY1_PIN   , OUTPUT);

  // initialize outputs
  digitalWrite(RELAY0_PIN, LOW);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  Serial.begin(9600);
  // initialize led matrix
  lc.begin(13,11,12,4);
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  for(uint8_t i = 0; i < 4; i++){
    lc.shutdown(i,false);
    /* Set the brightness to a medium values */
    lc.setIntensity(i,0);
    /* and clear the display */
    lc.clearDisplay(i);
  }
  for(uint8_t i = 0; i < sizeof(ceiborg); i++){
    lc.setRow(0,i,ceiborg[i]);
  }
  
}


void loop() {
  long now = millis();

  scan_inputs(now);

  // check panic buttons
  if( state!=MANUAL 
      && state!=PANIC 
      && ( (rightBtnLongPress && rightBtnReleased) || (leftBtnLongPress && leftBtnReleased))){
    state= PANIC;    
    Serial.println("to panic from long press");
  }

  
  switch(state){
    case MANUAL:
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);
      if(rightBtnReleased ){
        state = RIGHT_ENTER;
        Serial.println("idle to right_enter");
        switchedAt = now;
        automatic = rightBtnLongPress;
      } else if(leftBtnReleased ){
        state = LEFT_ENTER;
        switchedAt = now;
        Serial.println("idle to left_enter");
        automatic = leftBtnLongPress;
      }   
      break;
    case RIGHT_ENTER:
      digitalWrite(GREEN_LED_PIN, HIGH);
      if (automatic && (now - switchedAt) < REST_TIME)
        break;
      startedMovingAt = now;
      spin(1);
      state = RIGHT_UPDATE;
      Serial.println("right_enter to update");
      break;
    case RIGHT_UPDATE:
    {  
      long elapsed = now - startedMovingAt;
      if ( elapsed > TIME_TO_PANIC){
        state = PANIC;
        Serial.println(elapsed);
        Serial.println("right_=update to panic");
        break;
      }
      if ( rightBtnReleased ){
        state = RIGHT_EXIT;
        Serial.println(elapsed);
        Serial.println("right_=update to exit");
      }
      digitalWrite(GREEN_LED_PIN, (elapsed/100) % 2);
      break;
    }
    case RIGHT_EXIT:
      spin(0);
      if(automatic){
        state = LEFT_ENTER;
        Serial.println("right_exit to left enter");
        switchedAt = now;
      }
      else{
        state = MANUAL;
        Serial.println("right_exit to manual");
      }
      break;
    case LEFT_ENTER:
      digitalWrite(GREEN_LED_PIN, HIGH);
      if (automatic && (now - switchedAt) < REST_TIME)
        break;
      startedMovingAt = now;
      spin(-1);
      state = LEFT_UPDATE;
      Serial.println("left enter to update");
      break;
    case LEFT_UPDATE:
    {  
      long elapsed = now - startedMovingAt;
      if ( elapsed > TIME_TO_PANIC){
        state = PANIC;
        Serial.println(elapsed);
        Serial.println("left update to panic");
        break;
      }
      if ( leftBtnReleased ){
        state = LEFT_EXIT;
        Serial.println(elapsed);
        Serial.println("left update to exit");
      }
      digitalWrite(GREEN_LED_PIN, (elapsed/100) % 2);
      break;
    }
    case LEFT_EXIT:
      spin(0);
      if(automatic){
        state = RIGHT_ENTER;        
        Serial.println("left exit to right enter");
        switchedAt = now;
      }
      else{
        state = MANUAL;
        Serial.println("LEFT_EXIT to manual");
      }
      break;
    case PANIC:
      spin(0);
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);
      if( (rightBtnLongPress && rightBtnReleased) || (leftBtnLongPress && leftBtnReleased)){
        state= MANUAL;        
        Serial.println("panic to manual");
      }
      break;
  }

  
  // valid for a single frame
  rightBtnPressed = false;
  leftBtnPressed = false;
  rightBtnReleased = false;
  leftBtnReleased = false;
}


void spin( int s){
  digitalWrite(RELAY0_PIN, LOW);
  digitalWrite(RELAY1_PIN, LOW);
  if(s>0)
    digitalWrite(RELAY0_PIN, HIGH);
  else if(s<0)
    digitalWrite(RELAY1_PIN, HIGH);
}

void scan_inputs(long now) {
  int rightBtn = digitalRead(RIGHT_BTN_PIN);
  int leftBtn = digitalRead(LEFT_BTN_PIN);

  // check Btn press (activated on low)
  if(!rightBtnDown && rightBtn == LOW){
    delay(DEBOUNCE_TIME);
    rightBtn = digitalRead(RIGHT_BTN_PIN);
    if(rightBtn == LOW){
      rightBtnDown = true;
      rightBtnPressed = true;
      rightBtnPressedAt = now;
    }
  }
  if(!leftBtnDown && leftBtn == LOW){
    delay(DEBOUNCE_TIME);
    leftBtn = digitalRead(LEFT_BTN_PIN);
    if(leftBtn == LOW){
      leftBtnPressed = true;
      leftBtnDown = true;
      leftBtnPressedAt = now;
    }
  }
  
  // check Btn release
  if(rightBtnDown && rightBtn == HIGH){
    rightBtnDown = false;
    rightBtnReleased = true;
  }
  if(leftBtnDown && leftBtn == HIGH){
    leftBtnDown = false;
    leftBtnReleased = true;
  }

  // check Btn long press
  rightBtnLongPress = ( rightBtnReleased || rightBtnDown ) && (now - rightBtnPressedAt) > LONG_PRESS_TIME;
  leftBtnLongPress = (leftBtnReleased || leftBtnDown ) && (now - leftBtnPressedAt) > LONG_PRESS_TIME;  
  

}
