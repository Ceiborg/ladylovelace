#include <Arduino.h>

// constants
const int PANIC_BTN_PIN = 8;
const int RIGHT_BTN_PIN = 12;
const int LEFT_BTN_PIN = 13;
const int RED_LED_PIN = 11;
const int GREEN_LED_PIN = 10;
const int BLUE_LED_PIN = 9;
const int RELAY0_PIN = A3;
const int RELAY1_PIN = A4;
const int DEBOUNCE_TIME = 10;
const int LONG_PRESS_TIME = 1000;
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

// configurable values at startup
long at_enter_delay = 40000;
long at_exit_delay = 15000;

state_t state = MANUAL;
long switchedAt = 0;
bool automatic = false;

bool panicBtnDown = false;
bool rightBtnDown = false;
bool leftBtnDown = false;
bool panicBtnPressed = false;
bool rightBtnPressed = false;
bool leftBtnPressed = false;
bool panicBtnReleased = false;
bool rightBtnReleased = false;
bool leftBtnReleased = false;
long rightBtnPressedAt = 0;
long leftBtnPressedAt = 0;
bool rightBtnLongPress = false;
bool leftBtnLongPress = false;  

// function declarations
void spin( int s);
void scan_inputs(long now);


void setup() {  
  
  pinMode(PANIC_BTN_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BTN_PIN, INPUT_PULLUP);
  pinMode(LEFT_BTN_PIN , INPUT_PULLUP);
  pinMode(RED_LED_PIN  , OUTPUT);
  pinMode(BLUE_LED_PIN  , OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RELAY0_PIN   , OUTPUT);
  pinMode(RELAY1_PIN   , OUTPUT);

  // initialize outputs
  digitalWrite(RELAY0_PIN, LOW);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  
  // Serial.begin(9600);

  // if panic pressed at startup, use different delay values
  // for testing purposes
  if (digitalRead(PANIC_BTN_PIN) == LOW){
    delay(DEBOUNCE_TIME);
    if (digitalRead(PANIC_BTN_PIN) == LOW){
      at_enter_delay = 5000;
      at_exit_delay = 5000;
      // give red feedback to aknowledge button was read
      digitalWrite(BLUE_LED_PIN, HIGH);
      while(digitalRead(PANIC_BTN_PIN) == LOW)
        delay(10);
      digitalWrite(BLUE_LED_PIN, LOW);
    }
  }else{
    // give blue feedback on normal mode
    for (int i =0; i<=10; i++){
      digitalWrite(BLUE_LED_PIN, i%2);
      delay(100);
    }
  }

}

void transition_to(state_t s){
  state = s;
  switchedAt = millis();
}

void fade_leds(long e, long cycle, byte min_r, byte max_r, byte min_g, byte max_g, byte min_b, byte max_b){

  // first cycle goes up, second cycle goes down
  long val = ( (e / cycle) % 2 == 0) ? (e % cycle) : cycle - (e % cycle);

  byte r = map( val, 0, cycle, min_r, max_r );
  byte g = map( val, 0, cycle, min_g, max_g );
  byte b = map( val, 0, cycle, min_b, max_b );

  // update leds
  analogWrite(RED_LED_PIN, r);
  analogWrite(GREEN_LED_PIN, g);
  analogWrite(BLUE_LED_PIN, b);
}

void animate_leds(){
  long now = millis();
  long elapsed = now - switchedAt;

  switch(state){
    // idle
    case MANUAL:
      fade_leds( elapsed, 2000, 0, 0, 40, 255, 40, 255);
      break;
    // thinking
    case LEFT_ENTER:
    case RIGHT_ENTER: 
      {
        if ( elapsed < at_enter_delay - 2000){
          // update randomly every 0.1s
          if( elapsed % 100 == 0){
            byte r = random(255);
            byte g = random(255);
            byte b = random(255);
            // update leds
            analogWrite(RED_LED_PIN, r);
            analogWrite(GREEN_LED_PIN, g);
            analogWrite(BLUE_LED_PIN, b);
          }
        }
        else{
          // fadeout last second, so phase out to get falling fade only
          long e = elapsed - at_enter_delay + 2000 * 2;
          fade_leds( e, 2000, 0, 255, 0, 255, 0, 255);
        }
      }
      break;
    // moving
    case LEFT_UPDATE:
    case RIGHT_UPDATE:
      {
        // strobo at 0.1s
        byte strobo = (elapsed/50) % 2;
        digitalWrite(RED_LED_PIN, strobo);
        digitalWrite(GREEN_LED_PIN, strobo);
        digitalWrite(BLUE_LED_PIN, strobo);
      }
      break;
    // breath
    case LEFT_EXIT:
    case RIGHT_EXIT:
      if ( elapsed < at_exit_delay - 2000){
        fade_leds( elapsed, 3000, 10, 10, 10, 255,10,  255);
      }
      else{
        // fadeout last second, so phase out to get falling fade only
        long e = elapsed - at_exit_delay + 2000 * 2;
        fade_leds( e, 2000, 10, 10, 10, 255,10,  255);
      }
      break;
    // panic
    case PANIC:
      fade_leds( elapsed, 500,40,  255,0,  5,0,  5);
      break;
  }

}


void loop() {
  long now = millis();
  long elapsed = now - switchedAt;

  scan_inputs(now);
  
  // check panic buttons in first place
  if((state != PANIC) && panicBtnPressed ){
    state= PANIC;    
  } else{

  switch(state){
    case MANUAL:
      if(rightBtnReleased ){
        transition_to(RIGHT_ENTER);
        automatic = rightBtnLongPress;
      } else if(leftBtnReleased ){
        transition_to(LEFT_ENTER);
        automatic = leftBtnLongPress;
      }   
      break;
    case RIGHT_ENTER:
      if (automatic && elapsed < at_enter_delay)
        break;
      spin(1);
      transition_to(RIGHT_UPDATE);
      break;
    case RIGHT_UPDATE:
      if ( elapsed > TIME_TO_PANIC){
        transition_to(PANIC);
        break;
      }
      if ( rightBtnReleased ){
        transition_to(RIGHT_EXIT);
      }
      break;
    case RIGHT_EXIT:
      if (automatic && elapsed < at_exit_delay)
        break;
      spin(0);
      if(automatic){
        transition_to(LEFT_ENTER);
      }
      else{
        transition_to(MANUAL);
      }
      break;
    case LEFT_ENTER:
      if (automatic && elapsed < at_enter_delay)
        break;
      spin(-1);
      transition_to(LEFT_UPDATE);
      break;
    case LEFT_UPDATE:
      if ( elapsed > TIME_TO_PANIC){
        transition_to(PANIC);
        break;
      }
      if ( leftBtnReleased ){
        transition_to(LEFT_EXIT);
      }
      break;
    case LEFT_EXIT:
      if (automatic && elapsed < at_exit_delay)
        break;
      spin(0);
      if(automatic){
        transition_to(RIGHT_ENTER);
      }
      else{
        transition_to(MANUAL);
      }
      break;
    case PANIC:
      spin(0);
      if(panicBtnPressed ){
        transition_to(MANUAL);
      }
      break;
    }

  }

  animate_leds();

  // valid for a single frame
  panicBtnPressed = false;
  rightBtnPressed = false;
  leftBtnPressed = false;
  panicBtnReleased = false;
  rightBtnReleased = false;
  leftBtnReleased = false;
}


void spin( int s){
  digitalWrite(RELAY0_PIN, LOW);
  digitalWrite(RELAY1_PIN, LOW);
  if(s<0)
    digitalWrite(RELAY0_PIN, HIGH);
  else if(s>0)
    digitalWrite(RELAY1_PIN, HIGH); 
}

void scan_inputs(long now) {
  int panicBtn = digitalRead(PANIC_BTN_PIN);
  int rightBtn = digitalRead(RIGHT_BTN_PIN);
  int leftBtn = digitalRead(LEFT_BTN_PIN);

  // check Btn press (activated on low)
  if(!panicBtnDown && panicBtn == LOW){
    delay(DEBOUNCE_TIME);
    panicBtn = digitalRead(PANIC_BTN_PIN);
    if(panicBtn == LOW){
      panicBtnDown = true;
      panicBtnPressed = true;
    }
  }
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
  if(panicBtnDown && panicBtn == HIGH){
    panicBtnDown = false;
    panicBtnReleased = true;
  }
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
