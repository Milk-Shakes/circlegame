#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"
#include "LevelsMap.h"
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

const uint8_t PixelCount = 120; // Total number of pixels for both strips to be split up later
const uint8_t PixelPin = 3; //WS2812 data pin

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);

//Encoder homing sequence variables

int HomingOffset = 286;
volatile bool ringOffset = false;
bool Homed = false;
volatile bool ClockwiseSpinDirection = false; //debounce for interrupt change so the code isn't altering the interrupt every cycle
volatile byte LastPlayerPosition = 0;

volatile int encodervalue;
volatile int16_t encoderDelta;

//Kickplate debounce

volatile unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
volatile unsigned long debounceDelay = 5;


//GameSetup
byte CurrentLevel = 1;
byte NumberOfLevels = 3;
bool Setup = false;
bool firstrun = false;

byte EnemyArrayPosition = 0;
byte EnemyArrayScale = 0;
byte MovingEnemyArrayScale = 0;
volatile byte FinishPosition = 0;
bool Updated = true;
byte UpdatePosition = 0;

#define BackgroundColour RgbColor(127, 127, 127)
#define EnemyColour RgbColor(255, 0, 0)
#define MovingEnemyColour RgbColor(127, 255, 0)
#define Finishcolour RgbColor(0, 0, 0)

volatile unsigned long Gametick = 0;
volatile unsigned long lastGameTickTime = 0;  // the last time the output pin was toggled
volatile unsigned long GameTickDelay = 500;

volatile bool GameLost = false;
volatile bool LevelWon = false;

byte CurrentMovingEnemys[16] {
  /*clockwise*/200, 200, 200, 200, 200, 200, 200, 200 /*Counterclockwise*/, 200, 200, 200, 200, 200, 200, 200, 200
};


void EncoderReadCode( void * pvParameters ) {

  for (;;) {

    encodervalue = rotaryEncoder.readEncoder();
    encoderDelta = rotaryEncoder.encoderChanged();

  }
}

void drawPlayer(byte pixelAddress) {
  if (strip.GetPixelColor(pixelAddress) == EnemyColour or strip.GetPixelColor(pixelAddress) == MovingEnemyColour) {
    GameLost = true;
    return;
  } else if (strip.GetPixelColor(pixelAddress) == Finishcolour) {
    LevelWon = true;
    return;
  }
  strip.SetPixelColor(LastPlayerPosition, BackgroundColour);
  strip.SetPixelColor(pixelAddress, RgbColor(0, CharacterSaturation, 0));
  strip.Show();
  LastPlayerPosition = pixelAddress;
}

void setZero() {
  if (Homed == false) {
    rotaryEncoder.setEncoderValue(HomingOffset);
    Homed = true;
  }
}

int SmoothEncoderInput() {
  int smoothedencodervalue = 0;
  smoothedencodervalue = map(encodervalue, 0, 358, 0, 59);
  if (ringOffset == true) {
    smoothedencodervalue = (smoothedencodervalue + 60);
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

void Homing(byte DirectionOffset) {
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

void LevelSetup() {
  byte CurrentEnemyposition = 0;
  byte CurrentMovingEnemyposition = 0;
  if (firstrun == false) {
    EnemyArrayScale = EnemyLevelSize(CurrentLevel - 1);
    MovingEnemyArrayScale = MovingEnemyLevelSize(CurrentLevel - 1);
    memset(CurrentMovingEnemys, 200, MovingEnemyArrayScale);
    EnemyArrayPosition = 0;
    firstrun = true;
  }
  if (Setup == false) {
    if (EnemyArrayPosition < EnemyArrayScale or EnemyArrayPosition < MovingEnemyArrayScale and Setup == false == false) {

      CurrentEnemyposition = LevelRequest(CurrentLevel - 1, EnemyArrayPosition, 1);
      CurrentMovingEnemyposition = LevelRequest(CurrentLevel - 1, EnemyArrayPosition, 2);

      if (EnemyArrayPosition < MovingEnemyArrayScale and CurrentMovingEnemyposition < 119) {
        CurrentMovingEnemys[EnemyArrayPosition]  = CurrentMovingEnemyposition;
        strip.SetPixelColor(CurrentMovingEnemyposition, MovingEnemyColour);
      }
      if (EnemyArrayPosition < EnemyArrayScale and CurrentEnemyposition < 119) {
        strip.SetPixelColor(CurrentEnemyposition, EnemyColour);
      }

      EnemyArrayPosition++;

    } else if (Setup == false) {
      FinishPosition = LevelRequest(CurrentLevel - 1, EnemyArrayPosition, 3);
      strip.SetPixelColor(FinishPosition, Finishcolour);
      firstrun = false;
      Setup = true;
    }
  }
}

void UpdateEnemies() {
  if (UpdatePosition < MovingEnemyArrayScale) {
    if (CurrentMovingEnemys[UpdatePosition] < 120) {
      if (UpdatePosition <= (MovingEnemyArrayScale / 2)) {
        strip.SetPixelColor(UpdatePosition, BackgroundColour);
        CurrentMovingEnemys[UpdatePosition]++;
        strip.SetPixelColor(UpdatePosition, MovingEnemyColour);
      } else {
        strip.SetPixelColor(UpdatePosition, BackgroundColour);
        CurrentMovingEnemys[UpdatePosition]--;
        strip.SetPixelColor(UpdatePosition, MovingEnemyColour);
      }
    }
    UpdatePosition++;
  } else {
    Updated = true;
  }
}

void setup() {

  delay(150);

  strip.ClearTo(RgbColor(ColourSaturation, ColourSaturation, ColourSaturation));

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

  if (Setup == false) {
    LevelSetup();
    return;
  }

  if (((millis() - lastGameTickTime) > GameTickDelay)) {
    Gametick++;
    MovingEnemyArrayScale = MovingEnemyLevelSize(CurrentLevel - 1);
    UpdatePosition = 0;
    Updated = false;
    lastGameTickTime = millis();
  }

  if (Updated == false) {
    UpdateEnemies();
  }

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


  if (Homed == true and encodervalue > 30 and encodervalue < 300) {
    Homed = false;
  }


}
