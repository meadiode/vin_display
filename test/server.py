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

        self.power_on = False
        self.hdd_on = False
        self.pc_uptime = 0.0
        self.temp = 24.3

        self.power_btn_down = False
        self.power_btn_down_t = 0.0

        self.reset_btn_down = False
        self.reset_seq = False
        self.reset_seq_t = 0.0

        self.hdd_t = 0.0
        self.hdd_t1 = random()
        self.hdd_t2 = self.hdd_t1 + random()

        def simulate_pc():
            T = 0.001

            while True:

                if self.power_btn_down:
                    self.power_btn_down_t += T

                    if self.power_btn_down_t >= 3.0:
                        if self.power_on:
                            self.power_on = False
                    else:
                        if not self.power_on:
                            self.power_on = True

                else:
                    self.power_btn_down_t = 0.0


                if self.power_on:
                    self.pc_uptime += T

                    if self.reset_btn_down:
                        if not self.reset_seq:
                            self.reset_seq = True
                            self.reset_seq_t = 0.0
                            self.power_on = False
                    else:
                        self.reset_seq = False

                    if 0.0 <= self.hdd_t < self.hdd_t1:
                        self.hdd_on = False
                    elif self.hdd_t1 <= self.hdd_t < self.hdd_t2:
                        self.hdd_on = True
                    else:
                        self.hdd_t = 0.0
                        self.hdd_t1 = random()
                        self.hdd_t2 = self.hdd_t1 + random()

                    self.hdd_t += T

                else:
                    self.pc_uptime = 0.0
                    self.hdd_on = False


                if self.reset_seq:
                    if 0.0 <= self.reset_seq_t < 0.25:
                        self.power_on = False
                    elif 0.25 <= self.reset_seq_t < 0.5:
                        self.power_on = True
                    self.reset_seq_t += T

                sleep(T)

        self.pc_sim_thread = Thread(target=simulate_pc, args=[], daemon=True)
        self.pc_sim_thread.start()
        
        def send_stats_periodic():
            while True:
                if self.socket is not None:
                    stats = {
                                'power_on' : self.power_on,
                                'hdd_on' : self.hdd_on,
                                'pc_uptime': self.pc_uptime,
                                'temp': self.temp,
                            }
                    try:  
                        self.socket.send(json.dumps(stats), binary=False)
                    except WebSocketError as e:
                        print('Websocket error:', e)

                sleep(0.1)

        self.stat_thread = Thread(target=send_stats_periodic,
                                  args=[], daemon=True)
        self.stat_thread.start()

    
    def on_power_btn(self, down):
        self.power_btn_down = down


    def on_reset_btn(self, down):
        self.reset_btn_down = down


    def dispatch_message(self, msg):
        if 'btn_power_press' in msg:
            self.on_power_btn(msg['btn_power_press'])

        if 'btn_reset_press' in msg:
            self.on_reset_btn(msg['btn_reset_press'])



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
