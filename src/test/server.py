#! /usr/bin/python3

from bottle import route, post, request, run, static_file, get
from bottle.ext.websocket import GeventWebSocketServer
from bottle.ext.websocket import websocket
from geventwebsocket.exceptions import WebSocketError
from threading import Thread
from time import sleep
from random import random
import json


'''
A simple server to test web-interface

To install dependencies: pip install bottle bottle-websocket
'''

STATIC_PATH = '../static'


class Device:

    def __init__(self):
        
        self.socket = None
        self.temp = 24.3
        
        def send_stats_periodic():
            while True:
                if self.socket is not None:
                    stats = {
                                'temp': self.temp,
                            }
                    try:  
                        self.socket.send(json.dumps(stats), binary=False)
                    except WebSocketError as e:
                        print('Websocket error:', e)

                sleep(1.0)

        self.stat_thread = Thread(target=send_stats_periodic,
                                  args=[], daemon=True)
        self.stat_thread.start()

    
    def on_power_btn(self, down):
        self.power_btn_down = down


    def on_reset_btn(self, down):
        self.reset_btn_down = down


    def dispatch_message(self, msg):
        print('Client WS message:', msg)


dev = Device()

@route('/')
def hello():
    return static_file('index.html', root=STATIC_PATH)


@route('/<filename>')
def server_static(filename):
    return static_file(filename, root=STATIC_PATH)


@get('/ws', apply=[websocket])
def ws_dispatch(ws):
    if dev.socket is not None:
        try:
            dev.socket.close()
        except WebSocketError:
            pass

    dev.socket = ws

    while True:
        try:
            msg = ws.receive()
        except:
            break

        if msg is not None:
            try:
                msg = json.loads(msg)
                dev.dispatch_message(msg)
            except json.decoder.JSONDecodeError as e:
                print(repr(e))
        else:
            break


run(host='localhost', reloader=False, port=8080,
    debug=True, server=GeventWebSocketServer)
