#include <timer.h>
#include <TM1637.h>
#include <Adafruit_NeoPixel.h>
#include <EasyButton.h>

#define BUTTON_RED_PIN      10
#define BUTTON_WHITE_PIN    11
#define BUTTON_BLUE_PIN     12
#define BUTTON_YEALLOW_PIN  13

#define BUZZER_PIN 5

#define PIXEL_PIN    8    // Digital IO pin connected to the NeoPixels.
#define PIXEL_COUNT 16

#define DISPLAY_CLK_PIN   7
#define DISPLAY_DIO_PIN   6

#define ROTARY_ANGLE_SENSOR_PIN  A0
#define ADC_REF 5//reference voltage of ADC is 5v.If the Vcc switch on the seeeduino
         //board switches to 3V3, the ADC_REF should be 3.3
#define GROVE_VCC 5//VCC of the grove interface is normally 5v
#define FULL_ANGLE 20//full value of the rotary angle is 300 degrees

EasyButton redButton(BUTTON_RED_PIN);
EasyButton whiteButton(BUTTON_WHITE_PIN);
EasyButton blueButton(BUTTON_BLUE_PIN);
EasyButton yeallowButton(BUTTON_YEALLOW_PIN);

const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average
int angle = 0;

// Parameter 1 = number of pixels in strip,  neopixel stick has 8
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

TM1637 tm1637(DISPLAY_CLK_PIN, DISPLAY_DIO_PIN);

auto timer = timer_create_default(); // create a timer with default settings

byte patternIndex = 0;
byte oldPatternIndex = 0;
int counter = 0;

unsigned long patternInterval = 50 ; // default time between steps in the pattern
unsigned long lastPatternUpdate = 0 ; // for millis() when last update occoured

bool isStarted = false;

void setup() {
    Serial.begin(115200);

    // initialize all the readings to 0:
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }
  
    redButton.begin();
    redButton.onPressed(onRedButtonPressed);
    redButton.onSequence(3, 2000, onButtonLongPressed);
    
    whiteButton.begin();
    whiteButton.onPressed(onWhiteButtonPressed);
    whiteButton.onSequence(3, 2000, onButtonLongPressed);
  
    blueButton.begin();
    blueButton.onPressed(onBlueButtonPressed);
    blueButton.onSequence(3, 2000, onButtonLongPressed);
  
    yeallowButton.begin();
    yeallowButton.onPressed(onYeallowButtonPressed);
    yeallowButton.onSequence(3, 2000, onButtonLongPressed);
  
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
  
    tm1637.init();
    tm1637.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    pinMode(ROTARY_ANGLE_SENSOR_PIN, INPUT);

    timer.every(1000, displayTime);
}

void loop() {

    redButton.read();
    whiteButton.read();
    blueButton.read();
    yeallowButton.read();  
      
    if (isStartable()) {
        int newAngle = getAngle();
        if (newAngle != angle) {
            tm1637.displayNum(newAngle);
        }
        angle = newAngle;

        if (oldPatternIndex != patternIndex && patternIndex != 0) {
            counter = angle;
            isStarted = true;
        }
    }
  
    oldPatternIndex = patternIndex;

    if (millis() - lastPatternUpdate > patternInterval) {
        updatePattern(patternIndex);
    }

    timer.tick();
}
//////// BUZZER //////////

void microwaveDoneBuzzer() {
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 2000, 500);
        delay(700);
        noTone(BUZZER_PIN);
        delay(350);
    }
}

//////// DISPLAY /////////

bool displayTime(void *) {
    if (!isStarted) {
        return true;
    }

    Serial.println(counter);

    tm1637.displayNum(counter);
    
    if (counter <= 0) {
        patternIndex = 0;
        isStarted = false;
        microwaveDoneBuzzer();
    }

    counter--;

    return true; // repeat? true
}

//////// ROTATORY ANGLE SENSOR ///////

int getAngle() {

    // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    readings[readIndex] = analogRead(ROTARY_ANGLE_SENSOR_PIN);
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;
  
    // if we're at the end of the array...
    if (readIndex >= numReadings) {
        // ...wrap around to the beginning:
        readIndex = 0;
    }
  
    // calculate the average:
    average = total / numReadings;
      
    float voltage = (float)average*ADC_REF/1023;
    float degrees = (voltage*FULL_ANGLE)/GROVE_VCC;

    if (degrees < 0) {
        degrees = 0;
    }
    return degrees;
}

////////// BUTTONS ////////////

void onRedButtonPressed() {
    Serial.println("RED Button has been pressed!");

    if (!isStartable) {
      return;
    }
  
    int newPatternIndex = patternIndex;
    newPatternIndex++;
    if (newPatternIndex > 6)
        newPatternIndex = 0;
  
    patternIndex = newPatternIndex;
}

void onButtonLongPressed() {
  Serial.println("Long pressed detected");
  counter = 0;
}

void onWhiteButtonPressed() {
    Serial.println("WHITE Button has been pressed!");

    if (!isStartable) {
      return;
    }
  
    patternIndex = 1;
}

void onBlueButtonPressed() {
    Serial.println("BLUE Button has been pressed!");

    if (!isStartable) {
      return;
    }
  
    patternIndex = 2;
}

void onYeallowButtonPressed() {
    Serial.println("YEALLOW Button has been pressed!");

    if (!isStartable) {
      return;
    }
  
    patternIndex = 3;
}


////////// NEOPIXELS /////////

void updatePattern(byte i) {
  switch (i) {
    case 0: theaterChase(strip.Color(0, 0, 0)); // Black//off
      break;
    case 1: theaterChase(strip.Color(127, 127, 127)); // White
      break;
    case 2: theaterChase(strip.Color(255,   0,   0)); // Red
      break;
    case 3: theaterChase(strip.Color(  0,   255, 0)); // Blue
      break;
    case 4: theaterChase(strip.Color(  0,   0, 255)); // Blue
      break;
    case 5: rainbow();
      break;
    case 6: rainbowCycle();
      break;
    case 7: theaterChaseRainbow();
      break;
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c) {
    static int i = 0;

    strip.setPixelColor(i, c);
    strip.show();      

    i++;
    if (i >= strip.numPixels()) {
        i = 0;
    }

    lastPatternUpdate = millis(); // time for next change to the display
}

void rainbow() {
    static uint16_t j = 0;
    uint16_t i;

    for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();

    j++;
    if (j > 255) {
        j = 0;
    }

    lastPatternUpdate = millis();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle() {
    static uint16_t j = 0;
    uint16_t i;

    for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();

    j++;
    if (j > 255) {
        j = 0;
    }

    lastPatternUpdate = millis();    
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c) {
    static int q = 0;
    static bool on = true;

    if (on) {
        for (int i = 0; i < strip.numPixels(); i = i + 3) {
            strip.setPixelColor(i + q, c);  //turn every third pixel on
        }
    } else {
        for (int i = 0; i < strip.numPixels(); i = i + 3) {
            strip.setPixelColor(i + q, 0);      //turn every third pixel off
        }    
     
    }
    strip.show();    

    q++;
    if (q >= 3) {
        q = 0;
    }   
    on = !on;

    lastPatternUpdate = millis();
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow() {
    static int j = 0;
    static bool on = true;
  
    for (int q = 0; q < 3; q++) {
      if (on) {
        for (int i = 0; i < strip.numPixels(); i = i + 3) {
          strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
        }      
      } else {
        for (int i = 0; i < strip.numPixels(); i = i + 3) {
          strip.setPixelColor(i + q, 0);      //turn every third pixel off
        }      
      }
    }
    strip.show();

    j++;
    if (j > 255) {
        j = 0;
    }   
    on = !on;

    lastPatternUpdate = millis();  
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

bool isStartable() {
    return !isStarted;
}
