from django.shortcuts import render_to_response
from django.http import HttpResponseRedirect, HttpResponse
from django.core.urlresolvers import reverse
from django.template import RequestContext

from django_socketio import events


def home(request, template='index.html'):
    context = {"room" : "ping"}
    return render_to_response(template, context, RequestContext(request))
    
def msg_test(request, template='msg_test.html'):
    context = {"room" : "pong"}
    return render_to_response(template, context, RequestContext(request))
    
    
@events.on_message(channel="ping")
def message(request, socket, context, message):
    #socket.channels = ["ping"]
    #message = message[0]
    if message["a"] == "clr":
        
        pos = message["p"]
        red = message["r"]
        green = message["g"]
        blue = message["b"]
    
        print message
        msg = {
            "pos": pos,
            "r": red,
            "g": green,
            "b": blue,
            "a": "clr",
        }
        data = {"pid": socket.session.session_id, 'a': 'ack'}
        socket.send(data)
        socket.broadcast_channel(msg, channel="pong")
    
@events.on_connect()
def subscribe(request, socket, context):
    data = {"pid": socket.session.session_id, 'msg': "Connect"}
    socket.send(data)
