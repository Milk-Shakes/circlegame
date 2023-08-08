#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"
#include <NeoPixelBus.h>


#define ColourSaturation 130 //Background Brightness, Caution if power supply is below 6 amps DO NOT set all leds to 100% brightness
#define CharacterSaturation 255 //Player and 'enemy's brightness

/*
S2 Mini Pinout for Circle Game
Rotary encoder pins CLK = 9   DT = 8
Optical homing button = 35
Kick Plate Button = 18` 
Led Data Pin = 3
*/

#define HomingPoint 35
#define KickPlateButton 18

#define ROTARY_ENCODER_A_PIN 8
#define ROTARY_ENCODER_B_PIN 9
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
#define ROTARY_ENCODER_BUTTON_PIN 15

#define ROTARY_ENCODER_STEPS 4

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

TaskHandle_t EncoderRead;

//WS2812 led strip setup

const uint16_t PixelCount = 120; // Total number of pixels for both strips to be split up later
const uint8_t PixelPin = 3; //WS2812 data pin

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);

//Encoder homing sequence variables

int HomingOffset = 286;
volatile bool ringOffset = false;
bool Homed = false;
volatile bool ClockwiseSpinDirection = false; //debounce for interrupt change so the code isn't altering the interrupt every cycle

volatile int encodervalue;
volatile int16_t encoderDelta;

//Kickplate debounce

volatile unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
volatile unsigned long debounceDelay = 5;

void EncoderReadCode( void * pvParameters ) {
  
  for(;;) {

    encodervalue = rotaryEncoder.readEncoder();
    encoderDelta = rotaryEncoder.encoderChanged();

  }
}

void drawPlayer(int pixelAddress) {
  strip.ClearTo(RgbColor(ColourSaturation,ColourSaturation,ColourSaturation));
  strip.SetPixelColor(pixelAddress, RgbColor(0,CharacterSaturation,0));
  strip.Show();
}

void setZero() {
  if (Homed == false){
    rotaryEncoder.setEncoderValue(HomingOffset);
    Homed = true;
  }
}

int SmoothEncoderInput() {
  int smoothedencodervalue = 0;
  smoothedencodervalue = map(encodervalue, 0, 358, 0, 59);
  if (ringOffset == true) {
    smoothedencodervalue = (smoothedencodervalue+60);
  }
  return smoothedencodervalue;
}


void Switch(bool Pressed) {
  if (Pressed == false) {
    lastDebounceTime = millis();
    return;
  } else if (((millis() - lastDebounceTime) < debounceDelay) and Pressed == true) {
    ringOffset = !ringOffset;
  }
}

void Homing(int DirectionOffset) {
  rotaryEncoder.setEncoderValue(HomingOffset + DirectionOffset);
    Homed = true;
}

void IRAM_ATTR readEncoderISR()
{
	rotaryEncoder.readEncoder_ISR();
}

void HomingDirection() {
  if (encoderDelta > 0 and ClockwiseSpinDirection == true and encodervalue > 30 and encodervalue < 300) { 
        ClockwiseSpinDirection = false; 
    } else if (encoderDelta < 0 and ClockwiseSpinDirection == false and encodervalue > 30 and encodervalue < 300) {
        ClockwiseSpinDirection = true; 
      }
}

void setup() {

  rotaryEncoder.begin();
	rotaryEncoder.setup(readEncoderISR);

  rotaryEncoder.setBoundaries(0, 358, true); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  rotaryEncoder.setAcceleration(0); 

  strip.Begin();
  strip.Show();

   pinMode(HomingPoint, INPUT_PULLUP);

    pinMode(KickPlateButton, INPUT_PULLDOWN);

  xTaskCreatePinnedToCore(
                    EncoderReadCode,   /* Task function. */
                    "EncoderRead",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &EncoderRead,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */
  
}

void loop() {
  // put your main code here, to run repeatedly:

  HomingDirection();
  drawPlayer(SmoothEncoderInput());

  if (Homed == false) {
  if (digitalRead(HomingPoint) == 1 and ClockwiseSpinDirection == true) {
    Homing(0);
  } else if (digitalRead(HomingPoint) == 1 and ClockwiseSpinDirection == false) {
    Homing(1);
  }
  }

    Switch(digitalRead(KickPlateButton));
  

  if (Homed == true and encodervalue > 30 and encodervalue < 300) { Homed = false;  }


}
