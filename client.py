# implementation of a chat client that can interact with
# other connected clients over a server

from socket import *
import threading
import time

SERVER_NAME = 'localhost'
SERVER_PORT = 3000

lock = threading.RLock()
sentence = ""

def connect():
    # Create a socket of Address family AF_INET.
    sock = socket(AF_INET, SOCK_STREAM)

    # Client socket connection to the server
    sock.connect((SERVER_NAME, SERVER_PORT))

    # to prevent recv from blocking in client()
    sock.settimeout(.05)

    return sock


# send and recieve data from server
def send(sock, message):
    sock.send(bytearray(message, 'utf-8'))


def recv(sock):
    return sock.recv(1024).decode('utf-8')


# let client set unique username
def set_username(sock):
    response = 'null'

    # loop until unique username is entered
    while response == 'null':
        user = raw_input('Create a username: ')
        send(sock, user)
        response = recv(sock)
        if response.strip() == 'null':
            print('\nUsername is taken, try entering another one: ')

    print(response.strip())
    print('Enter a command or message: \n')
    print("Type '\\HELP' for a list of available commands\n\n");


# thread process to ask for new input after command is processed
def getInput():
    global lock
    global sentence
    while True:
        print("Testing1\n")
        time.sleep(2)
        if len(sentence) == 0:
            sentence = raw_input()


def client():
    connection = connect()
    set_username(connection)

    # create seperate thread for reading input, so client is not blocked from
    # recieving messages in real time from input()
    inputThread = threading.Thread(target = getInput, args = ())
    inputThread.daemon = True
    inputThread.start()

    global lock
    global sentence
    # loop until '\LEAVE' command
    while True:
        if(len(sentence) != 0):
            send(connection, sentence)
            response = recv(connection)
            print(response.strip())
            if(response == 'Disconnecting...goodbye'):
                break
            sentence = ""
            print('\n')
        
        # if input was not entered, check for incoming messages
        else:
            try:
                response = recv(connection)
                print(response.strip())
                print('\n')  
            except timeout:
                continue;

    exit()

client()
