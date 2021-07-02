#!/usr/bin/env python3

import socket

HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

print("Starting...")

def clientThread(clientConnection, clientAddress):
    clientConnection.settimeout(30)
    
    print('Connected to' + str(clientAddress))

    try:
        while True:
            data = clientConnection.recv(256)
            if not data:
                break

            print("Got data: " + str(data));
            clientConnection.sendall(data)
    except socket.timeout:
        print("Connection timed out!")

    print("Closing the connection for ")
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
            clientThread(conn, addr)

    s.close()
