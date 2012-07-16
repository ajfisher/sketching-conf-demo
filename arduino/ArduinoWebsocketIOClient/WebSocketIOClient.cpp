/*
 WebsocketClient, a websocket client for Arduino
 Copyright 2011 Kevin Rohling
 http://kevinrohling.com
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */
 
 /*
 
 SocketIO Client Adaptations, a socket IO client for Arduino
 Extension by Andrew Fisher, 2012 licensed under the above permissions.
 http://ajfisher.me
 
 */

#include <WebSocketIOClient.h>
#include <WString.h>
#include <string.h>
#include <stdlib.h>

//#define DEBUG

prog_char stringVar[] PROGMEM = "{0}";
prog_char clientHandshakeLine1[] PROGMEM = "GET {0} HTTP/1.1";
prog_char clientHandshakeLine2[] PROGMEM = "Upgrade: WebSocket";
prog_char clientHandshakeLine3[] PROGMEM = "Connection: Upgrade";
prog_char clientHandshakeLine4[] PROGMEM = "Host: {0}";
prog_char clientHandshakeLine5[] PROGMEM = "Origin: ArduinoWebSocketClient";
prog_char serverHandshake[] PROGMEM = "HTTP/1.1 101";

PROGMEM const char *WebSocketClientStringTable[] =
{   
    stringVar,
    clientHandshakeLine1,
    clientHandshakeLine2,
    clientHandshakeLine3,
    clientHandshakeLine4,
    clientHandshakeLine5,
    serverHandshake
};

String WebSocketIOClient::getStringTableItem(int index) {
    char buffer[35];
    strcpy_P(buffer, (char*)pgm_read_word(&(WebSocketClientStringTable[index])));
    return String(buffer);
}

bool WebSocketIOClient::connect(char hostname[], char path[], int port) {
    bool result = false;
    #ifdef DEBUG
        Serial.print("Attempting a connection to");
        Serial.print(hostname);
        Serial.print(":");
        Serial.print(port);
    #endif
    if (_client.connect(hostname, port)) {
        #ifdef DEBUG
            Serial.println("Connecting");
        #endif
        sendHandshake(hostname, path);
        result = readHandshake();
    }
    #ifdef DEBUG
        if (result) {
            Serial.println("Connected");
        } else {
            Serial.println("Not connected");
        }
    #endif
        
	return result;
}


bool WebSocketIOClient::connected() {
    return _client.connected();
}

void WebSocketIOClient::disconnect() {
    _client.stop();
}

void WebSocketIOClient::monitor () {
    char character;
    
	if (_client.available() > 0 && (character = _client.read()) == 0) {
        String data = "";
        bool endReached = false;
        while (!endReached) {
            character = _client.read();
            endReached = character == -1;

            if (!endReached) {
                data += character;
            }
        }
        
        if (_dataArrivedDelegate != NULL) {
            _dataArrivedDelegate(*this, data);
        }
    }
}

void WebSocketIOClient::setDataArrivedDelegate(DataArrivedDelegate dataArrivedDelegate) {
	  _dataArrivedDelegate = dataArrivedDelegate;
}


void WebSocketIOClient::sendHandshake(char hostname[], char path[]) {
    String stringVar = getStringTableItem(0);
    String line1 = getStringTableItem(1);
    String line2 = getStringTableItem(2);
    String line3 = getStringTableItem(3);
    String line4 = getStringTableItem(4);
    String line5 = getStringTableItem(5);
    
    line1.replace(stringVar, path);
    line4.replace(stringVar, hostname);
    #ifdef DEBUG
        Serial.println("Handshake:");
        Serial.println(line1);
        Serial.println(line2);
        Serial.println(line3);
        Serial.println(line4);
        Serial.println(line5);
    #endif
    _client.println(line1);
    _client.println(line2);
    _client.println(line3);
    _client.println(line4);
    _client.println(line5);
    _client.println();
}

bool WebSocketIOClient::readHandshake() {
    bool result = false;
    char character;
    String handshake = "", line;
    int maxAttempts = 300, attempts = 0;
    
    #ifdef DEBUG
        Serial.println("Waiting to read response");
    #endif
    
    while(_client.available() == 0 && attempts < maxAttempts) 
    { 
        delay(100); 
        attempts++;
    }
    
    while((line = readLine()) != "") {
        handshake += line + '\n';
    }
    
    String response = getStringTableItem(6);
    result = handshake.indexOf(response) != -1;
    
    if(!result) {
        _client.stop();
    }
    
    return result;
}

String WebSocketIOClient::readLine() {
    String line = "";
    char character;
    
    while(_client.available() > 0 && (character = _client.read()) != '\n') {
        if (character != '\r' && character != -1) {
            line += character;
        }
    }
    
    return line;
}

void WebSocketIOClient::send (String data) {
/**    _client.print((char)0);
	_client.print(data);
    _client.print((char)255);
**/    
    _client.print((char)0);
    _client.print("~m~");
    _client.print(data.length()+3);
    _client.print("~m~~j~");
    _client.print(data);
    _client.print((char)255);
    #ifdef DEBUG
        Serial.println("Sent:");
        Serial.print("~m~");
        Serial.print(data.length()+3);
        Serial.print("~m~~j~");
        Serial.println(data);
    #endif
}

void WebSocketIOClient::subscribe(String room) {
    _client.print((char)0);
    _client.print("~m~13~m~__subscribe__~m~");
    _client.print(room.length());
    _client.print("~m~");
    _client.print(room);
    _client.print((char)255);
    #ifdef DEBUG
    Serial.print((char)0);
    Serial.print("~m~13~m~__subscribe__~m~");
    Serial.print(room.length());
    Serial.print("~m~");
    Serial.print(room);
    Serial.print((char)255);
    Serial.println();
    #endif
}
