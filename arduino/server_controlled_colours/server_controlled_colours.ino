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

#define BUFLENGTH 64
char buf[BUFLENGTH]; // character buffer for json processing

#define STATE_NONE 0
#define STATE_START 1
#define START_COLOURCHANGE 2
int state = STATE_NONE;

int CKI = 2;
int SDI = 3;
int colour_id = 0;

#define STRIP_LENGTH 3 // Number of RGBLED modules connected
long strip_colors[STRIP_LENGTH];

long ind_colours[STRIP_LENGTH]; // actual individual modules.

int pos = -1;
int red = 0;
int blue = 0;
int green = 0;

//#define DEBUG

void setup() {
  // set up the devices
  pinMode(SDI, OUTPUT);
  pinMode(CKI, OUTPUT);

  Serial.begin(9600);
  #ifdef DEBUG
    Serial.println("Starting Up");
    freeMem("At Start: ");
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
  } else {
    #ifdef DEBUG
    Serial.println("Not Connected - check connection and start again");
    #endif
    while (1) {}
  }
  
  digitalWrite(13, HIGH);
}

void loop() {
  client.monitor();
  
  for (int i=0; i < STRIP_LENGTH; i++) {
    if (pos == i) {
      ind_colours[i] = get_colour(red, green, blue);
    } else {
      ind_colours[i] = get_colour(0,0,0);
    }
  }
  
  post_set(ind_colours);
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
      
    }
    
    #ifdef DEBUG
    Serial.print("RGB: (");
    Serial.print(red);
    Serial.print(",");
    Serial.print(green);
    Serial.print(",");
    Serial.print(blue);
    Serial.print(") P: ");
    Serial.println(pos);    
    #endif
    
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

void post_frame (int colour_id) {
  for(int LED_number = 0; LED_number < STRIP_LENGTH; LED_number++)
  {
    long this_led_color = strip_colors[LED_number]; //24 bits of color data
    
    for(byte color_bit = 23 ; color_bit != 255 ; color_bit--) {
      //Feed color bit 23 first (red data MSB)

      digitalWrite(CKI, LOW); //Only change data when clock is low

      long mask = 1L << color_bit;
      //The 1'L' forces the 1 to start as a 32 bit number, otherwise it defaults to 16-bit.

      if(this_led_color & mask) 
        digitalWrite(SDI, HIGH);
      else
        digitalWrite(SDI, LOW);

      digitalWrite(CKI, HIGH); //Data is latched when clock goes high
    }
  }

  //Pull clock low to put strip into reset/post mode
  digitalWrite(CKI, LOW);
  delayMicroseconds(500); //Wait for 500us to go into reset
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


