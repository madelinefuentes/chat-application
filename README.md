# Multi-Threaded Chat - TCP Server and Client
This is a **basic messaging application** that uses **socket programming** to connect multiple clients to a server. Each client represents a user that may join different chatrooms, send messages to the chatroom, and send direct messages to other connected users. The server may process specified commands from clients (joining a room, getting a list of users in current room...etc).

A new **thread** is created in the server for each client that runs. For simplicity, a unique username is associated with every client. To receive messages in real-time (i.e. recieve messages without waiting for input), a seperate thread is created in the client to grab input from the console while **main** may still listen for messages and print them.

## Instructions
Compile the server and chatroom class, run the server with port number 3000:
./server 3000

Run the python client which will connect to the server and ask for a username.

Typing '\HELP' into the client will print a list of possible commands.
