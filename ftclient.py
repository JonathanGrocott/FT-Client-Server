#!/usr/bin/python

###########################################################################
# Program Filename: ftclient.py
# Author: Jonathan Grocott
# Date: 11/18/18
# CS_372_400_F2018 Project 2
# Description: Simple file transfer client implementation
# requires ftserver.c on another host.
# References -
# https://www.bogotobogo.com/python/python_network_programming_server_client_file_transfer.php
# https://docs.python.org/2/library/socket.html
# http://docs.python.org/release/2.6.5/library/internet.html
###########################################################################

#import required libraries
import socket
import sys
import fileinput
import threading
import os.path

#Checks number of command line args
if len(sys.argv) < 4:
	print("Invalid arguments: ")
	print("ftclient [host] [port] [command] [filename]")
	sys.exit(1)

#gets username and password to authenticate with the server
username = input("Username: ")
password = input("Password: ")
authentication = username + "&" + password

#sets command line arguments
server_host = sys.argv[1];
server_port = int(sys.argv[2])
command = sys.argv[3]
message = command
data_port = server_port - 1

#checks number of command line arguments
if len(sys.argv) == 5:
	filename = sys.argv[4]
	message = message + " " + filename
	if os.path.exists(filename):
		overwrite = input("File already exists. Would you like to overwrite it? yes/no\n")
		if overwrite[:1] == "n" or overwrite[:1] == "N":
			print("Exiting program...")
			sys.exit(0)

#recieves the directory
def rcvDir(dataSocket):
	dataConnection, address = dataSocket.accept()
	fileData = dataConnection.recv(10000)
	dirListing = fileData.split()
	
	for currFileName in dirListing:
		print(currFileName.decode("utf-8"))

#recieves the file
def rcvFile(dataSocket, filename):
	dataConnection, address = dataSocket.accept()
	outFile = open(filename,"w+")
	while True:
		fileData = dataConnection.recv(10000)
		if not fileData: break
		if len(fileData) == 0: break
		outFile.write(fileData.decode("utf-8"))
	outFile.close()

#handles the file transfer communication
def ftComm(serverSocket):
	while 1:
		server_response = serverSocket.recv(1000)
		if not server_response: break
		if len(server_response) == 0: break
		if server_response == b'/quit\x00': break
		print(server_response.decode("utf-8"))
	serverSocket.close()

#create socket to send commands to server
serverSocket = socket.socket(socket.AF_INET,socket.SOCK_STREAM);
serverSocket.connect((server_host,server_port))

#create socket to receive data
dataSocket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
dataSocket.bind(('',data_port))
dataSocket.listen(5)

#send authentication data
serverSocket.send(authentication.encode("utf-8"))
authReply = serverSocket.recv(100)
decodedReply = authReply.decode("utf-8")
print(decodedReply)
if decodedReply[:1] == "I":
	sys.exit(0)

#send command and listen for resoonse
serverSocket.send(message.encode("utf-8"))
if len(sys.argv) == 5:
	threading.Thread(target=rcvFile,args=(dataSocket, filename),).start()
else:
	threading.Thread(target=rcvDir,args=(dataSocket,),).start()
threading.Thread(target=ftComm,args=(serverSocket,),).start()
