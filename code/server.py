#!/usr/bin/env python3

import time
import socket
import threading

HOST = "127.0.0.1"
PORT = 65432

class Client:
    address = None
    connection = None
    info = None

class Command:
    command = None
    clientNumber = None

    def __init__(self, command, clientNumber):
        self.command = command
        self.clientNumber = clientNumber

clients = []
commandQueue = []

def ThreadRegisterClient(client):
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
                print("[INFO] Got login response. Sending info command...")
                client.connection.send(b"info")

                data = client.connection.recv(256)
                if not data:
                    break

                print("[INFO] Got info response...")
                client.info = data.decode()
                break
            else:
                client.connection.send(data)
    except socket.timeout:
        print("[EXCEPTION] Connection timed out!")
    except Exception as e:
        print("[EXCEPTION] Unknown exception: " + str(e))

    print("[INFO] Client connected and registered.")

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

            client = Client()
            client.connection = conn_dup
            client.address = addr
            clients.append(client)

            threading.Thread(target=ThreadRegisterClient,args=(client,)).start()

        s.close()

threading.Thread(target=ThreadSocketServer).start()

time.sleep(1)

print("Available commands:\n  list: Lists down available clients.\n")

while True:
    command = input(">> ")
    if command == "list":
        if len(clients) > 0:
            i = 0
            for c in clients:
                print("Client " + str(i) + ": " + c.info + "\n")
        else:
            print("No available clients.\n")
    elif command.startswith("shutdown"):
        commandSplitted = command.split(" ")
        clientNumber = int(commandSplitted[1])

        command = Command(commandSplitted[0], commandSplitted[1])
        commandQueue.append(command)

        client = clients[clientNumber]
        client.connection.send(b"shutdown")

        print("[INFO] Closing the connection for " + str(client.address))
        client.connection.close()
        clients.remove(client)
