#include <NeoPixelBus.h>

int playerPosition = 0;
int playerLevel = 0;

const uint8_t playfieldLength = 200;
const uint8_t directionPin = A0;
const uint8_t levelSwitchPin = 1;
const uint8_t stripPin = 2;
int tickSpeed = 10;

bool updated = 0;
bool move = 1;
int direction = 0;
int tickDivider = 0;

long tickCount = 0;

RgbColor player(0, 255, 128);
NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(playfieldLength, stripPin);

void setup() {
  pinMode(directionPin, INPUT);
  pinMode(levelSwitchPin, INPUT);
  Serial.begin(9600);

  strip.Begin();
  strip.Show();

  // Set up the timers for the game tickspeed
  cli();
  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4  = 0;
  // 1 tick = 16uS (0.016mS)
  int multiplier = tickSpeed * (float)62.5;

  OCR4A = multiplier*2;

  // turn on CTC mode
  TCCR4B |= (1 << WGM12);
  // Set CS11 for an 256x prescaler
  TCCR4B |= (1 << CS12) ;  
  // enable timer compare interrupt
  TIMSK4 |= (1 << OCIE4A);

  sei();//allow interrupts
}

void loop() {
  if (updated) {
    strip.ClearTo(0);
    strip.SetPixelColor(playerPosition, player);
    strip.Show();
    updated = 0;
  }
  refreshInputs();
}

ISR(TIMER4_COMPA_vect){ //Runs as the tickspeed
  movePlayer();

  // Increment the tick counter
  tickCount++;
}

void refreshInputs() {
  int rawInput = analogRead(directionPin);
  int speedValue = map(rawInput,0,1024,-16,16);
  if (speedValue < 0) { speedValue = speedValue * -1; direction = -1; }
  else if (speedValue == 0) { direction = 0; }
  else { direction = 1; }
  tickDivider = map(speedValue, 1, 16, 16, 1);
}

void movePlayer() {
  if ( (tickCount % tickDivider) == 0) {
    updated = 1;
    if (direction == 1) { 
      if (playerPosition >= (playfieldLength -1)) { playerPosition = 0; }
      else { playerPosition++; }
    }
    else if (direction == -1) { 
       if (playerPosition <= (0)) { playerPosition = (playfieldLength - 1); }
      else { playerPosition--; }
    }
  }
}

void moveObstacles() {
  
}