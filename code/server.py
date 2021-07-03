#!/usr/bin/env python3

import time
import socket
import threading

HOST = "127.0.0.1"
PORT = 65432

def clientThread(clientConnection, clientAddress):
    print("[INFO] Connected to" + str(clientAddress))
    print("[INFO] Client: " + str(clientConnection))

    try:
        while True:
            data = clientConnection.recv(256)
            if not data:
                break

            print("Got data: " + str(data));
            command = data.decode()
            if command == "login":
                print(">> Got login. Sending info...")
                clientConnection.send(b"info")
            else:
                clientConnection.send(data)
            
    except socket.timeout:
        print("[EXCEPTION] Connection timed out!")
    except Exception as e:
        print("[EXCEPTION] Unknown exception: " + str(e))

    print("[INFO] Closing the connection for " + str(clientAddress))
    clientConnection.close()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print("[INFO] Binding to " + HOST + " and " + str(PORT) + "...")
    s.bind((HOST, PORT))
    print("[INFO] Listening...")
    s.listen()

    while True:
        conn, addr = s.accept()
        with conn:
            conn_dup = conn.dup() # We duplicate the connection because it somehow gets destroyed after starting a new thread
            threading.Thread(target=clientThread,args=(conn_dup, addr)).start()

    s.close()
