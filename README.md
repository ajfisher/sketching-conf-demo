=====================================
Sketching in Hardware Light Demo
=====================================

This was a demo I did for my presentation for Sketching in Hardware 2012: http://sketching-in-hardware.com/ 

The aim of this demo was to show that the real time web could be used to generate a mechanism for real world play or interaction. 

The application is broken into two components:

* Web sockets server and application 
* Arduino hardware and firmware

Web sockets server and application
-----------------------------------

The websockets server uses Django Socket IO. A Web Sockets, Socket IO server designed for use with django. You can get the source and see more about the package here: https://github.com/stephenmcd/django-socketio

Once installed and running, the server takes care of the messages that are sent by the web browser and forwards them onto the arduino. Most of the action pretty much happens in sketching/sketching/views.py

The web side creates all of the interface which sets up the interface and constructs all of that.


Arduino hardware and firmware
------------------------------

The arduino makes use of an adapted (for Socket IO) version of Kevin Rohling's Arduino WebSocketClient library whicha handles a lot of the connections and the general web sockets stuff. I've added some additional elements to handle the SocketIO components.

Load one of the arduino sketches up and you'll be able to connect to the server.


