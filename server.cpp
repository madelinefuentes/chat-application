// multi-threaded chat server that may connect to multiple
// clients(or users)and allow them to interact with messaging and
// and chat rooms

#include "chatroom.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace std;

// Simplifies calls to bind(), connect(), and accept()
typedef struct sockaddr SA;

// Max text line length
#define MAXLINE 8192

// Second argument to listen()
#define LISTENQ 1024

// A lock for the message buffer.
pthread_mutex_t lock;

// A wrapper around recv to simplify calls.
int receive_message(int connfd, char *message) {
  return recv(connfd, message, MAXLINE, 0);
}

// A wrapper around send to simplify calls.
int send_message(int connfd, char *message) {
  return send(connfd, message, strlen(message), 0);
}

//send result of processing message through a client's connection file descriptors
int send_response(int connfd, char *message) {
  return send_message(connfd, message);
}


//     implementions of commands the server is able act on     //

//send a list of available rooms to user
int ROOMS(char* user, int connfd) {
  vector<Room> rlist = rooms();
  char response[MAXLINE] = "\nRooms: [";

  for (unsigned int i = 0; i < rlist.size(); i++) {
    // add Room's name to the string
    strcat(response, rlist[i].name);

    if (i != rlist.size() - 1) {
      // add separating commas
      strcat(response, ", ");
    }
  }
  strcat(response, "]");
  return send_response(connfd, response);
}

//send a list of users that are currently in the same room as the client
int WHO(char* user, int connfd) {
  char response[MAXLINE] = "\nUsers: [";
  vector<User> ulist = who(user);

  for (unsigned int i = 0; i < ulist.size(); i++) {
    // add User's name to the string
    strcat(response, ulist[i].name);
    if (i != ulist.size() - 1) {
      // add separating commas
      strcat(response, ", ");
    }
  }

  strcat(response, "]");
  return send_response(connfd, response);
}

//returns a list of available commands and descriptions to the client
int HELP(char* user, int connfd) {

  char response[MAXLINE] = "\nAvailable Commands:\n";

  strcat(response, "\\JOIN <room> adds the user to the specified room;");
  strcat(response, "if the room does not exist, it is created\n");
  strcat(response, "\\ROOMS returns a list of available rooms\n");
  strcat(response, "\\LEAVE removes you from your current room and disconnects the client\n");
  strcat(response, "\\WHO sends a list of users in your current room\n");
  strcat(response, "\\HELP sends a list of available commands\n");
  strcat(response, "\\<nickname> <message> sends a message to the user specified by nickname\n");
  strcat(response, "All other messages will be sent to users in your current room");

  return send_response(connfd, response);
}

//removes the user from the room it is currently in and disconnects them from the server
int LEAVE(char* user, int connfd) {
  printf("Disconnecting '%s' from the chat...\n", user);
  leave(user);
  char response[MAXLINE] = "Disconnecting...goodbye";
  return send_response(connfd, response);
}

//move the user to the specified room
int JOIN(char* user, int connfd, char* token) {
  if(token == NULL){
    char response[MAXLINE] = "Include a room to join";
    return send_response(connfd, response);
  }
  else{
    char response[MAXLINE] = "You joined room: ";
    strcat(response, token);
    strcat(response, "!\n");
    User check;
    strcpy(check.name, user);
    check.connfd = connfd;
    join(check, token);
    return send_response(connfd, response);
  }
}

//send messages to users in room, including current user
int message_room(char* user, char* message) {
  char response[MAXLINE] = "";
  strcat(response, user);
  strcat(response, ":");
  strcat(response, message);
  Room* currRoom = findRoom(user);

  vector<User>::iterator iter;
  
  for(iter = currRoom->users.begin(); iter != currRoom->users.end(); ++iter)
    send_response(iter->connfd, response);

  return 0;
}

//send direct message to specified user
int nickname_message(char* userTo, char* userFrom, char* message){
  char response[MAXLINE] = "Message sent to '";
  strcat(response, userTo);
  strcat(response, "'!\n");
        
  char mes[MAXLINE] = "Direct message from '";
  strcat(mes, userFrom);
  strcat(mes, "': \n");
  strcat(mes, message);
  
  User* from = findUser(userFrom);
  send_response(from->connfd, response);
  User* to= findUser(userTo);
  return send_response(to->connfd, mes);
}

//process message the client gave to the server
//username and connfd are also passed in to
//send reponse back to client
int process_message(int connfd, char *message, char *user) {

  //if message has a backwards slash (\), it is interpreted as a command
  if (message[0] == '\\') {
    if (strcmp(message, "\\ROOMS") == 0)
      return ROOMS(user, connfd);

    else if (strcmp(message, "\\WHO") == 0)
      return  WHO(user, connfd);

    else if (strcmp(message, "\\HELP") == 0)
      return HELP(user, connfd);

	  else if (strcmp(message, "\\LEAVE") == 0)
      return LEAVE(user, connfd);

    else {
      char *token = strtok(message, " ");
      char *nickname = token;

      //\JOIN command
      if (strcmp(token, "\\JOIN") == 0)
        return JOIN(user, connfd, token);

	    //'\nickname message' command
	    else if(exists(++nickname)){
        //parse message
        char mes[MAXLINE] = "";
        token = strtok(NULL, " ");
        while(token != NULL){
          strcat(mes, token);
          token = strtok(NULL, " ");
        }

        return nickname_message(nickname, user, mes);
	    }

      //if server processed the client's message as a command
	    //and it did not match possible commands, server responds
	    //with an error and the message
	    else {
        printf("Server could not recognize command: %s\n", message);
        char response[MAXLINE] = "\n\"";
        strcat(response, message);
        strcat(response, "\" command not recognized\n");
        return send_response(connfd, response);
      }
    }
  }

  //messages not interpreted by commands will be sent to all
  //users in the current room of the sending user
  return message_room(user, message);
}


// get and process commands from the client
void handle_requests(int connfd, char* user) {
  size_t n;

  // Holds the received message.
  char message[MAXLINE];

  while ((n = receive_message(connfd, message)) > 0) {
	  printf("Processing message from user: %s\n", user);
    message[n] = '\0';  // null terminate message (for string operations)
    n = process_message(connfd, message, user);
  }
}


// Helper function to establish an open listening socket on given port.
int open_listenfd(int port) {
  int listenfd;    // the listening file descriptor.
  int optval = 1;
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

  /* Eliminates "Address already in use" error from bind */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                 sizeof(int)) < 0)
    return -1;

  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) return -1;

  /* Make it a listening socket ready to accept connection requests */
  if (listen(listenfd, LISTENQ) < 0) return -1;
  return listenfd;
}


void *thread(void *vargp);

int main(int argc, char **argv) {
  // Check the program arguments and print usage if necessary.
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  pthread_mutex_init(&lock, NULL);

  // The port number for this server.
  int port = atoi(argv[1]);

  // The listening file descriptor.
  int listenfd = open_listenfd(port);

  while (1) {
    // The connection file descriptor.
    int *connfdp = (int *)malloc(sizeof(int));

    // The client's IP address information.
    struct sockaddr_in clientaddr;

    // Wait for incoming connections.
    socklen_t clientlen = sizeof(struct sockaddr_in);
    *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);

    struct hostent *hp =
        gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                      sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    // The server IP address information.
    char *haddrp = inet_ntoa(clientaddr.sin_addr);

    // The client's port number.
    unsigned short client_port = ntohs(clientaddr.sin_port);

    printf("Server connected to %s (%s), port %u\n", hp->h_name, haddrp,
            client_port);

    // Create a new thread to handle the connection.
    pthread_t tid;
    pthread_create(&tid, NULL, thread, connfdp);
  }
}


/* thread routine */
void *thread(void *vargp) {
  // Grab the connection file descriptor.
  int connfd = *((int *)vargp);

  pthread_detach(pthread_self());
  free(vargp);

  //get username of client, username must not be taken by
  //another client; loops until a unique name is entered
  User newUser;

  bool added = false;
  pthread_mutex_lock(&lock);
  while(!added){
    char name[50] = "";
    receive_message(connfd, name);
	  //username cannot be taken
    if(!exists(name)){
      char res[MAXLINE] = "Successfully connected to the chat...\n";
      send_response(connfd, res);
      added = true;
      strcpy(newUser.name, name);
      newUser.connfd = connfd;
    }
    else{
      char res[5] = "null";
      send_response(connfd, res);
    }
  }
  pthread_mutex_unlock(&lock);

  //add this new user to a default room
  char room[50] = "377";
  join(newUser, room);

  printf("User %s joined the server\n", newUser.name);

  // Handle client requests.
  handle_requests(connfd, newUser.name);

  //while loop in client stopped
  printf("User: %s was disconnected\n", newUser.name);
  close(connfd);
  return NULL;
}
