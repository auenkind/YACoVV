/* This is a very simple program for the monitoring the manometer of YACoVV
 *  https://github.com/auenkind/YACoVV
 *  
 *  The current state of code is only for prototyping, so don't use it with an patient is connected to your system.
 *  
 *  The state of the inputs and the calculated breathing frequency is written to the serial Port ans can be plotted with
 *  the "Serial Plotter" of the Arduino IDE
 *  
 *  The state of the system is reported with a stripe of WS2812 LEDs. See defines below for pin configuration.
 *  A momentary switch needs to be connected to reset the system from error state.
 *  
 *  On normal operation the LEDs are set to red. If the system starts working correctly and the frequencys for inhalation are exhalation
 *  are nearly the same, the LED changes to blue and blinks an every breathing cycle. If an error occurs, the LED color changes to red.
 *  If you press the button the LEDs changes to green an remains in this state until the next error occurs.
 */

#include <CapacitiveSensor.h>
#include <Adafruit_NeoPixel.h>

#define PIN_WS2812_LEDS   7
#define NUM_WS2812_LEDS   3
#define PIN_UPPER_SENSOR  2
#define PIN_LOWER_SENSOR  3
#define PIN_SWITCH        6
#define F_TIMEOUT         5000


#define OFF   0
#define GREEN 1
#define BLUE  2
#define RED   3

Adafruit_NeoPixel pixels(NUM_WS2812_LEDS, PIN_WS2812_LEDS, NEO_GRB + NEO_KHZ800);


int errorcommited = 0;
int errorcount = 0;
boolean ledstate = 0;
int state_a;
int state_b;

int state_a_prev;
int state_b_prev;

float freq_exsp = 0;
float freq_insp = 0;
long f_start_exsp;
long f_start_insp;

void setLed(int color) {
  pixels.clear();
  for (int i = 0; i < NUM_WS2812_LEDS; i++) {
    switch (color) {
      case OFF:
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        break;
      case GREEN:
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
        break;
      case BLUE:
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
        break;
      case RED:
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        break;
    }
    pixels.show();
  }
}

void setup()
{
  pinMode(PIN_UPPER_SENSOR,INPUT_PULLUP);
  pinMode(PIN_LOWER_SENSOR,INPUT_PULLUP);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  Serial.begin(115200);
  pixels.begin();
  setLed(RED);
}

void loop()
{
  //Read values from Sensors
  state_a = digitalRead(PIN_UPPER_SENSOR);
  state_b = digitalRead(PIN_LOWER_SENSOR);

  //Print frequencys in breathes per minute and states of sensors
  Serial.print(freq_exsp * 60);
  Serial.print("\t");
  Serial.print(freq_insp * 60);
  Serial.print("\t");
  Serial.print(state_a);
  Serial.print("\t");
  Serial.println(state_b);

  // Calculate frequency of upper sensor when input changes from low to high
  if ( state_a > state_a_prev ) {
    freq_exsp = (float)1000 / ((float)millis() - (float)f_start_exsp);
    f_start_exsp = millis();
  }
  // Calculate frequency of lower sensor when input changes from low to high
  if ( state_b > state_b_prev ) {
    freq_insp = (float)1000 / ((float)millis() - (float)f_start_insp);
    f_start_insp = millis();
  }

  // Set frequency on upper sensor to zero if sensor doesn't changed for F_TIMEOUT milliseconds
  if ( millis() - f_start_exsp > F_TIMEOUT) {
    freq_exsp = 0;
  }

  //Same fpr lower sensor
  if ( millis() - f_start_insp > F_TIMEOUT) {
    freq_insp = 0;
  }

  // If difference of frequency between inspiration and exspritaion is to big increase errorcounter
  if ( abs((freq_insp - freq_exsp) * 60) > 5) {
    errorcount++;
  } else {
    errorcount = 0;
  }

  // If errorcounter is > 4 raise an alarm
  if (errorcount > 3 || (freq_insp == 0 && freq_exsp == 0)) {
    errorcommited = 0;
    setLed(RED);
  }
  // Reset error when system comes back again
  else {
    // On non error state let the led blink with every breathcycle.
    if (state_a_prev != state_a || state_b_prev != state_b){
    if (ledstate) {
      // Set the LED to green if error was commited by a user with the momentary switch
      if (errorcommited) {
        setLed(GREEN);
      }
      //If not commited switch led to blue
      else {
        setLed(BLUE);
      }
    } else {
      setLed(OFF);
    }
    ledstate = !ledstate;
    }
  }
  //Commit error when switch is pressed
  if (!digitalRead(PIN_SWITCH)) {
    errorcommited = 1;
  }

  state_a_prev = state_a;
  state_b_prev = state_b;
  delay(10);                             // arbitrary delay to limit data to serial port
}
