/*
 * main.c
 *
 *  Created on: Jun 16, 2017
 *      Author: parallels
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>

#include "chat.h"
/*This Program gets an fd from who is trigger her and opens two new threads
 * One for writing into socket
 * Two for reading from socket*/
/*the socket will be closed when MSG_END will be sent*/
pthread_t tid[2];
int isAlive=1;

void *WriteToScreen(void* connfd)
{
	//set up variables for select()
	fd_set all_set, r_set;
	int maxfd = *(int*)connfd + 1;
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set);
	FD_SET(*(int*)connfd, &all_set);
	r_set = all_set;
	struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;

	char peer_replay[256];
	while(isAlive){
		r_set = all_set;
		//check to see if we can read from  sock
		 select(maxfd, &r_set, NULL, NULL, &tv);
		   if(FD_ISSET(*(int*)connfd, &r_set)){
		        //Receive a reply from the server
		        if( recv(*(int*)connfd , peer_replay , 256 , 0) < 0)
		        {
		            puts("recv failed");
		            break;
		        }
		        /*if we got "exit" terminated both threads*/
		        if(strcasestr(peer_replay,"exit") != 0)
		        	{
		        	isAlive=0;
		        	printf("the peer on the otherside send you MSG_END Goodbye(press anykey to exit)\n");
		        	fputs("exit",stdin);
		        	}
		        	printf("Peer Reply: %s\n", peer_replay);
		        	peer_replay[0]='\0';
		   }
	}
	return 0;
}

void *ReadFromKeyboard(void* connfd)
{
	char text[256] = {0};

	while(isAlive){
		memset(&text[0], 0, sizeof(text));
		fgets(text,256,stdin);
		text[256]='\0';
		 if(strcasestr(text,"exit") != 0)
			{
			isAlive=0;
			fputs("exit",stdin);
			}
		send(*(int*)connfd,text,strlen(text)+1,0);
		printf("YOU:%s\n",text);
	}
	return 0;
}

int main(int argc,char*argv[])
{
	if(argc <= 1) {
	        printf("You did not feed me arguments, I will die now :( ...");
	        exit(1);
	     }  //otherwise continue on socket communicate....
	int connfd = atoi(argv[1]);  //argv[0] is the program name
	                           //atoi = ascii to int

	//Check if socket is valid!
	int val;
	socklen_t len = sizeof(val);
	if (getsockopt(connfd, SOL_SOCKET, SO_ACCEPTCONN, &val, &len) == -1){
	    printf("fd %d is not a connected socket between 2 peers exiting...\n", connfd);
	    exit(0);
	}

	printf("Chat Session Has Been Started..\n");
	printf("To End Chat Session simply enter [exit]..\n");
	/*open thread for Reading*/
	if(pthread_create(&tid[0] , NULL , WriteToScreen ,&connfd) < 0){
		perror("thread");
	}
	/*open thread for Writing*/
	if(pthread_create(&tid[1] , NULL , ReadFromKeyboard ,&connfd) < 0){
		perror("thread");
	}
	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	close(connfd);

	return 0;
}
