#!/usr/bin/env python3

import socket

HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 65432        # Port to listen on (non-privileged ports are > 1023)

print("Starting...")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print("Binding...")
    s.bind((HOST, PORT))
    print("Listening...")
    s.listen()
    print("Accepting...")
    conn, addr = s.accept()
    with conn:
        print('Connected by' + str(addr))
        while True:
            data = conn.recv(15)
            print("Got data: " + str(data));
            if not data:
                break
            conn.sendall(data)
