#! /usr/bin/python3

from bottle import route, post, request, run, static_file

'''
A simple server to test web-interface
'''

STATIC_PATH = '../static'


@route('/')
def hello():
    return static_file('index.html', root=STATIC_PATH)


@route('/<filename>')
def server_static(filename):
    return static_file(filename, root=STATIC_PATH)


@post('/cmd')
def do_cmd():
    cmd = request.json['cmd']
    return f'executing "{cmd}" (not)'


run(host='localhost', reloader=True, port=8080, debug=True)
