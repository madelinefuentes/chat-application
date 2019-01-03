//class of functions dealing with chatrooms

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
#include <algorithm>

using namespace std;

vector<Room> roomlist;


//return list of rooms
vector<Room> rooms() {
  return roomlist;
}


//return room user is in; if user does not exist, return nullptr
Room* findRoom(char *username) {
  vector<Room>::iterator iter;
  vector<User>::iterator findUser;

  for(iter = roomlist.begin(); iter != roomlist.end(); ++iter) {
    findUser = find(iter->users.begin(), iter->users.end(), username);
    if(findUser != iter->users.end())
      break;
  }

  if(iter != roomlist.end())
    return &*iter;

  else
    return nullptr;
}


//return user stuct from user name; if user does not exist, return nullptr
User* findUser(char *username) {
  Room* findR = findRoom(username);
  vector<User>::iterator iter;

  for(iter = findR->users.begin(); iter != findR->users.end(); ++iter) {
    if(strcmp(iter->name, username) == 0)
      break;
  }

  if(iter != findR->users.end())
    return &*iter;

  else
    return nullptr;
}


//check if username exists
bool exists(char *username) {
  Room* find = findRoom(username);
  return (find != nullptr);
}


//add user to room
void join(User user, char *roomname) {
  vector<Room>::iterator iter;

  //find room the user is currently in
  Room* findR = findRoom(user.name);

	//if user is in a room, remove them
	if(findR != nullptr)
		leave(user.name);

	//place user in room, if room does not exist, create it
	iter = find(roomlist.begin(), roomlist.end(), roomname);

	if(iter != roomlist.end())
    iter->users.push_back(user);

	else{
		Room create;
		strcpy(create.name, roomname);
	  create.users.push_back(user);
	  roomlist.push_back(create);
	}
}


//return users from current room
vector<User> who(char *username) {
  Room* currRoom = findRoom(username);

  if (currRoom != nullptr)
    return currRoom->users;

  else
    return vector<User>();
}


//remove user from room it's currently in
void leave(char *username) {
  vector<User>::iterator iter;
  Room* removeUser = findRoom(username);

  if(removeUser == nullptr)
	  printf("Error: user should not be outside room at this point\n");

  else{
	  iter = find(removeUser->users.begin(), removeUser->users.end(), username);
	  removeUser->users.erase(iter);
  }
}
