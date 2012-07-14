#include "Arduino.h"
#include <Ethernet.h>
#include <SPI.h>
#include <string.h>
#include <stdlib.h>

#include <WebSocketClient.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "10.0.1.15";
char path[] = "/socket.io/websocket/";
int port = 8000;
WebSocketClient client;

#define BUFLENGTH 32
char buf[BUFLENGTH]; // character buffer for json processing

int CKI = 2;
int SDI = 3;

#define STRIP_LENGTH 19 // Number of RGBLED modules connected

long ind_colours[STRIP_LENGTH]; // actual individual modules.

struct colour_module {
  int r;
  int g;
  int b;
  //long rgb;
};

struct colour_module modules[STRIP_LENGTH];

#define DECAY_TIME 8 // number of cycles to run before decaying the data.

int decay_counter = 0; // use to keep track of cycles before doing a decay loop.

//#define DEBUG

void setup() {
  // set up the devices
  pinMode(SDI, OUTPUT);
  pinMode(CKI, OUTPUT);

  Serial.begin(9600);
  #ifdef DEBUG
    Serial.println("Starting Up");
    freeMem("At Start: ");
    Serial.println("initialise modules");
  #endif

  for (int i=0; i<STRIP_LENGTH; i++) {
    modules[i].r = 0;
    modules[i].g = 0;
    modules[i].b = 0;
    //modules[i].rgb = 0;
  }
  
  modules[1].g = 63;
  #ifdef DEBUG 
  freeMem("After struct setup");
  #endif
  
  Ethernet.begin(mac);
  delay(1000);
  #ifdef DEBUG
    Serial.println("Connecting");
  #endif
  if (client.connect(server, path, port) ) {
    #ifdef DEBUG
    Serial.println("Attempt a subscribe");
    #endif
    client.subscribe("pong");
    #ifdef DEBUG
    Serial.println("Sending data");
    #endif
    client.setDataArrivedDelegate(dataArrived);
    
    // set the connected light to be on.
    digitalWrite(13, HIGH);
  } else {
    #ifdef DEBUG
    Serial.println("Not Connected - check connection and start again");
    #endif
    while (1) {
      //TODO Drop some debug code in here
    }
  }
}

void loop() {
  client.monitor();
  
  // we check here if it's time to run a decay loop or not.
  if (decay_counter-- <= 0) {
    decay_counter = DECAY_TIME;
    // decay the values on the light strip.
    for (int i=0; i < STRIP_LENGTH; i++) {
      if (--modules[i].r < 0) modules[i].r = 0;
      if (--modules[i].g < 0) modules[i].g = 0;
      if (--modules[i].b < 0) modules[i].b = 0;
      
      if (modules[i].r == 0 && modules[i].g == 0 && modules[i].b == 0) {
          ind_colours[i] = 0;
      } else {
          ind_colours[i] = get_colour(modules[i].r, modules[i].g, modules[i].b);
      }
    }
    
    post_set(ind_colours);
  }
  delay(1);
}

void dataArrived(WebSocketClient client, String data) {

  // check here for a json message as it's most timely and also most frequent
  int jsonindex = data.lastIndexOf("~j~");
  if (jsonindex > 0) {
    //now get the json message
    String msg_string = data.substring(jsonindex+3);
    msg_string = msg_string.substring(1, msg_string.lastIndexOf("}"));
    msg_string.replace(" ", "");
    msg_string.replace("\"", "");
    
    #ifdef DEBUG
    Serial.println("Message:");
    Serial.println(msg_string);
    freeMem("After JSON arrives: ");
    #endif
    
    msg_string.toCharArray(buf, BUFLENGTH);

    #ifdef DEBUG
    freeMem("After char conversion: ");
    #endif
    
    // iterate over the tokens of the message - assumed flat.
    char *p = buf;
    char *str;
    
    int red = 0;
    int green = 0;
    int blue = 0;
    int pos = -1;
    bool colour_change = false;
    
    while ((str = strtok_r(p, ",", &p)) != NULL) { 
      #ifdef DEBUG
      Serial.println(str);
      #endif
      
      char *tp = str;
      char *key; char *val;
      
      // get the key
      key = strtok_r(tp, ":", &tp);
      val = strtok_r(NULL, ":", &tp);
      
      #ifdef DEBUG
      Serial.print("Key: ");
      Serial.println(key);
      Serial.print("val: ");
      Serial.println(val);
      #endif
      
      if (*key == 'r') red = atoi(val);
      if (*key == 'g') green = atoi(val);
      if (*key == 'b') blue = atoi(val);
      if (*key == 'p') pos = atoi(val);
      if (*key == 'a' && *val == 'c') colour_change = true;
      
    }

    // if it's a colour change then lets change the colour.
    // also just check for the position too.
    if (colour_change && pos < STRIP_LENGTH) {
      
      #ifdef DEBUG
      Serial.print("Change colour: ");
      Serial.print(colour_change);
      Serial.print(" RGB: (");
      Serial.print(red);
      Serial.print(",");
      Serial.print(green);
      Serial.print(",");
      Serial.print(blue);
      Serial.print(") P: ");
      Serial.println(pos);    
      #endif
      
      // this code adds the new value on top of the existing one
      // and then applies a cap if required
      if ((modules[pos].r += red ) > 255) modules[pos].r = 255;
      if ((modules[pos].g += green ) > 255) modules[pos].g = 255;
      if ((modules[pos].b += blue ) > 255) modules[pos].b = 255;

      #ifdef DEBUG
      Serial.print(" RGB: (");
      Serial.print(modules[pos].r);
      Serial.print(",");
      Serial.print(modules[pos].g);
      Serial.print(",");
      Serial.print(modules[pos].b);
      Serial.print(") P: ");
      Serial.println(pos);    
      #endif
    }
    
  } else {
    // check for heartbeat message
    int heartindex = data.lastIndexOf("~h~");
    #ifdef DEBUG
    Serial.print("Heartbeat sequence: ");
    Serial.println(data.substring(heartindex+3));// datalength-3));

    freeMem("After heartbeat: ");
    #endif
    
  }  
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


#ifdef DEBUG
//Code to print out the free memory

struct __freelist {
  size_t sz;
  struct __freelist *nx;
};

extern char * const __brkval;
extern struct __freelist *__flp;

uint16_t freeMem(uint16_t *biggest)
{
  char *brkval;
  char *cp;
  unsigned freeSpace;
  struct __freelist *fp1, *fp2;

  brkval = __brkval;
  if (brkval == 0) {
    brkval = __malloc_heap_start;
  }
  cp = __malloc_heap_end;
  if (cp == 0) {
    cp = ((char *)AVR_STACK_POINTER_REG) - __malloc_margin;
  }
  if (cp <= brkval) return 0;

  freeSpace = cp - brkval;

  for (*biggest = 0, fp1 = __flp, fp2 = 0;
     fp1;
     fp2 = fp1, fp1 = fp1->nx) {
      if (fp1->sz > *biggest) *biggest = fp1->sz;
    freeSpace += fp1->sz;
  }

  return freeSpace;
}

uint16_t biggest;

void freeMem(char* message) {
  Serial.print(message);
  Serial.print(":\t");
  Serial.println(freeMem(&biggest));
}

#endif


