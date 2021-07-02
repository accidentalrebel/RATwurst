#!/usr/bin/env python3

import time
import socket
import _thread
import threading

HOST = '127.0.0.1'
PORT = 65432

print("Starting...")

def clientThread(clientConnection, clientAddress):
    print("client: " + str(clientConnection))
    print('Connected to' + str(clientAddress))

    try:
        while True:
            data = clientConnection.recv(256)
            if not data:
                break

            print("Got data: " + str(data));
            clientConnection.send(data)
            
    except socket.timeout:
        print("Connection timed out!")
    except Exception as e:
        print("Unknown exception: " + str(e))

    print("Closing the connection for " + str(clientAddress))
    clientConnection.close()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print("Binding...")
    s.bind((HOST, PORT))
    print("Listening...")
    s.listen()
    print("Accepting...")

    while True:
        conn, addr = s.accept()
        with conn:
            conn_dup = conn.dup() # We duplicate the connection because it somehow gets destroyed after starting a new thread
            threading.Thread(target=clientThread,args=(conn_dup, addr)).start()

    s.close()
