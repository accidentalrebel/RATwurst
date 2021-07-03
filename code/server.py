#!/usr/bin/env python3

import time
import socket
import threading

HOST = "127.0.0.1"
PORT = 65432

class Client:
    address = None
    connection = None

clients = []

def ThreadClient(clientConnection, clientAddress):
    client = Client()
    client.connection = clientConnection
    client.address = clientAddress
    clients.append(client)
    print("[INFO] Connected to" + str(client.address))
    print("[INFO] Client: " + str(client.connection))

    try:
        while True:
            data = client.connection.recv(256)
            if not data:
                break

            print("Got data: " + str(data));
            command = data.decode()
            if command == "login":
                print(">> Got login. Sending info...")
                client.connection.send(b"info")
            else:
                client.connection.send(data)
            
    except socket.timeout:
        print("[EXCEPTION] Connection timed out!")
    except Exception as e:
        print("[EXCEPTION] Unknown exception: " + str(e))

    print("[INFO] Closing the connection for " + str(client.address))
    client.connection.close()

    clients.remove(client)

def ThreadSocketServer():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        print("[INFO] Binding to " + HOST + " and " + str(PORT) + "...")
        s.bind((HOST, PORT))
        print("[INFO] Listening...")
        s.listen()

        while True:
            conn, addr = s.accept()
            with conn:
                conn_dup = conn.dup() # We duplicate the connection because it somehow gets destroyed after starting a new thread
                threading.Thread(target=ThreadClient,args=(conn_dup, addr)).start()

        s.close()

threading.Thread(target=ThreadSocketServer).start()

time.sleep(1)

print("Available commands:\n  list: Lists down available clients.\n")

while True:
    command = input(">> ")
    if command == "list":
        if len(clients) > 0:
            print("Client 0: " + str(clients[0].connection) + "\n")
        else:
            print("No available clients.\n")
