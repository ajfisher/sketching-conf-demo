#!/usr/bin/python

#################################################
# Gunicorn config for sketching app
# 
#################################################

import os

bind = "127.0.0.1:8000"
daemon = False
debug = False
pidfile = "/tmp/gunicorn.pid"
workers = (os.sysconf("SC_NPROCESSORS_ONLN") * 4) + 1
loglevel = "info"
timeout = 120

