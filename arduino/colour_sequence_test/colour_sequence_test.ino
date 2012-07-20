/*

    Test to ensure all of the programmable LEDs are working
    correctly before moving to a full web sockets version.
    
    Author: Andrew Fisher
    Version: 0.1
*/

#include "Arduino.h"
#include <SPI.h>
#include <stdlib.h>


int CKI = 2;
int SDI = 3;
int colour_id = 0;

#define STRIP_LENGTH 19 // Number of RGBLED modules connected

#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define YELLOW 0xFFFF00
#define CYAN 0x00FFFF
#define PURPLE 0xFF00FF
#define WHITE 0xFFFFFF
#define EMPTY 0x000000

long ind_colours[STRIP_LENGTH]; // actual individual modules.

long colour_array[] = {RED, GREEN, BLUE, YELLOW, CYAN, PURPLE, WHITE, EMPTY};

enum COLOUR_STATE {
    R,
    G,
    B,
    Y,
    C,
    P,
    W,
    E
};

int state = R;

//#define DEBUG

void setup() {
  // set up the devices
  pinMode(SDI, OUTPUT);
  pinMode(CKI, OUTPUT);

  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Starting Up");
  #endif
  
}

void loop() {

    if (++state > E) state = R;

  
    for (int i=0; i < STRIP_LENGTH; i++) {
        ind_colours[i] = colour_array[state];
        post_set(ind_colours);
        delay(100);
    }

  
  delay(1);
}


// stuff here for posting colours

long get_colour(int red, int green, int blue) {
  // given an 8 bit value for a colour channel, return the
  // long version of it.
  long r = (long) red<<16;
  long g = (long) green<<8; 
  long b = (long) blue;  
  return (r + g + b);
}

void post_set(long *modules) {
  // takes the full set of modules and then posts the data appropriately
  for (int led_index =0; led_index < STRIP_LENGTH; led_index++) {
    write_to_module(modules[led_index]);
  }
  
  //Pull clock low to put strip into reset/post mode
  digitalWrite(CKI, LOW);
  delayMicroseconds(500); //Wait for 500us to go into reset
}

void write_to_module(long led_colour) {
  // this method actually writes the colour that is provided to the module string
  
    for(byte color_bit = 23 ; color_bit != 255 ; color_bit--) {
      //Feed color bit 23 first (red data MSB)

      digitalWrite(CKI, LOW); //Only change data when clock is low

      long mask = 1L << color_bit;
      //The 1'L' forces the 1 to start as a 32 bit number, otherwise it defaults to 16-bit.
      if(led_colour & mask) 
        digitalWrite(SDI, HIGH);
      else
        digitalWrite(SDI, LOW);

      digitalWrite(CKI, HIGH); //Data is latched when clock goes high
    }
}

