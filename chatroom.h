//header file for chatroom.cpp

#ifndef __CHATROOM_H
#define __CHATROOM_H

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
#include <algorithm>

using namespace std;


struct User {
  
  char name[100];
  int connfd;

  bool operator==(const User obj) const {
    return strcmp(this->name, obj.name) == 0;
  }  

  bool operator==(const char *username) const {
    return strcmp(this->name, username) == 0;
  }
}; 


struct Room {

  char name[100];
  vector<User> users;

  bool operator==(const Room obj) const {
    return strcmp(this->name, obj.name) == 0;
  } 

  bool operator==(const char *roomname) const {
    return strcmp(this->name, roomname) == 0;
  }
};


//function headers
vector<Room> rooms();

Room* findRoom(char *username);

User* findUser(char *username);

bool exists(char *username);

void join(User user, char *roomname);

vector<User> who(char *username);

void leave(char *username);


#endif