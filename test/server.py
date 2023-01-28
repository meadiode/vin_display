#! /usr/bin/python3

from bottle import route, post, request, run, static_file, get
from bottle.ext.websocket import GeventWebSocketServer
from bottle.ext.websocket import websocket

'''
A simple server to test web-interface

To install dependencies: pip install bottle bottle-websocket
'''

STATIC_PATH = '../static'


@route('/')
def hello():
    return static_file('index.html', root=STATIC_PATH)


@route('/<filename>')
def server_static(filename):
    return static_file(filename, root=STATIC_PATH)


@get('/ws', apply=[websocket])
def echo(ws):
    while True:
        msg = ws.receive()
        if msg is not None:
            print(msg)
        else:
            break


run(host='localhost', reloader=True, port=8080,
    debug=True, server=GeventWebSocketServer)
