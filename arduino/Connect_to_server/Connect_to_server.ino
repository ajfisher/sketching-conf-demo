#include "Arduino.h"
#include <Ethernet.h>
#include <SPI.h>
#include <string.h>
#include <stdlib.h>

#include <WebSocketIOClient.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "10.0.1.15";
char path[] = "/socket.io/websocket/";
int port = 8000;
WebSocketIOClient client;

#define BUFLENGTH 64
char buf[BUFLENGTH]; // character buffer for json processing

#define STATE_NONE 0
#define STATE_START 1
#define START_COLOURCHANGE 2
int state = STATE_NONE;

int pos = -1;
int red = 0;
int blue = 0;
int green = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting Up");
  freeMem("At Start: ");
  Ethernet.begin(mac);
  delay(1000);
  Serial.println("Connecting");
  if (client.connect(server, path, port) ) {
    Serial.println("Attempt a subscribe");
    client.subscribe("pong");
    Serial.println("Sending data");
    client.setDataArrivedDelegate(dataArrived);
  } else {
    Serial.println("Not Connected - check connection and start again");
    while (1) {}
  }
}

void loop() {
  client.monitor();
}

void dataArrived(WebSocketIOClient client, String data) {

  // check here for a json message as it's most timely and also most frequent
  int jsonindex = data.lastIndexOf("~j~");
  if (jsonindex > 0) {
    //now get the json message
    String msg_string = data.substring(jsonindex+3);
    msg_string = msg_string.substring(1, msg_string.lastIndexOf("}"));
    msg_string.replace(" ", "");
    msg_string.replace("\"", "");
    
    Serial.println("Message:");
    Serial.println(msg_string);
    freeMem("After JSON arrives: ");
    msg_string.toCharArray(buf, BUFLENGTH);
    freeMem("After char conversion: ");
    
    // iterate over the tokens of the message - assumed flat.
    char *p = buf;
    char *str;
    while ((str = strtok_r(p, ",", &p)) != NULL) { 
      Serial.println(str);
      char *tp = str;
      char *key; char *val;
      
      // get the key
      key = strtok_r(tp, ":", &tp);
      val = strtok_r(NULL, ":", &tp);
      Serial.print("Key: ");
      Serial.println(key);
      Serial.print("val: ");
      Serial.println(val);
      
      if (*key == 'r') red = atoi(val);
      if (*key == 'g') green = atoi(val);
      if (*key == 'b') blue = atoi(val);
      if (*key == 'p') pos = atoi(val);
      
    }
    
    Serial.print("RGB: (");
    Serial.print(red);
    Serial.print(",");
    Serial.print(green);
    Serial.print(",");
    Serial.print(blue);
    Serial.print(") P: ");
    Serial.println(pos);    
    
  } else {
    // check for heartbeat message
    int heartindex = data.lastIndexOf("~h~");
    Serial.print("Heartbeat sequence: ");
    Serial.println(data.substring(heartindex+3));// datalength-3));

    freeMem("After heartbeat: ");
  }  
}





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


