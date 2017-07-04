/*
 * chats.c
 *
 *  Created on: Jun 6, 2017
 *      Author: Eran Peled
 *
 *      This File is the implement of the "Server"
 *      During this exercise the server is a kind of "DB" he is responsible keeping records of users in the system
 *      and provide a list of current connected users to the system to any given client.
 */
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<string.h>    //strlen, puts
#include<stdio.h>
#include<stdlib.h> //exit
#include<netinet/in.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include "chat.h"

#define NUM_OF_USERS 10 //total number of users that can be register
#define MAX_USERS 50
static unsigned int users_count = 0;

static msg_peer_t *listOfUsers[MAX_USERS] = {0}; //global array of connected users to the system. notice the {0}initializer it is very important!
static int port_cnt = 0;
/*Define by me a function that peeks into packet and by her type provide the service(excute the right code)*/
void *handlePacket(void *args);
/* Add user to userList */
void user_add(msg_peer_t *msg);
/* Delete user from userList */
void user_delete(msg_down_t* msg);

int main(int argc, char*argv[]) //***check all if for exit the program or not.
{

	/*Define vars for communication*/
	int socket_fd,client_fd;
	struct sockaddr_in serv_sck,client_sock;
	pthread_t t; //thread for the accepted request

	/*******************************/
	//Create socket for IPv4
	socket_fd = socket(AF_INET , SOCK_STREAM , 0); //SOCK stream used for TCP Connections
	if (socket_fd == -1)
	{
		printf("Could not create socket, there is something wrong with your system");
	}
	puts("Socket created");

	//clear! and Prepare the sockaddr_in structure
	// bzero((char *) &serv_sck, sizeof(serv_sck));
	memset(&serv_sck, 0, sizeof(serv_sck));

	serv_sck.sin_family = AF_INET; //IPv4 Structure
	serv_sck.sin_addr.s_addr = INADDR_ANY; //opened to any IP
	serv_sck.sin_port = htons( C_SRV_PORT ); //port num defined in chat.h

	//Bind socket
	if( bind(socket_fd,(struct sockaddr *)&serv_sck , sizeof(serv_sck)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen (opens the socket for income connection up to NUM_OF_USERS connection)
	if(listen(socket_fd, NUM_OF_USERS) == -1) {
		perror("listen fail");
		exit(1);
	}

	puts("Waiting For Clients...");
	while(1)
	{
		//accept  single connection from an incoming client
		//bzero((char *) &client_sock, sizeof(client_sock)); 	//clear! and Prepare the sockaddr_in structure
		memset(&client_sock, 0, sizeof(client_sock));
		client_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);

		if (client_fd < 0)
		{
			perror("try to accept incoming connection failed");
		}
		else{
			puts("Connection accepted sending MSG_ACK to client");
		}

		if(pthread_create(&t, NULL, handlePacket, (void*)&client_fd)!= 0) {

			perror("could not create thread");
		}

		pthread_join(t,NULL);
		if(close(client_fd) == -1) {
			perror("close fail");
		}
		sleep(1);
	}
	if(close(socket_fd) == -1){
		perror("close fail");
		exit(1);
	}

	return 0;
}

/* Add user to userList */
void user_add(msg_peer_t *msg){
	if(users_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
		return;
	}
	int i;
	for(i=0;i<MAX_USERS;i++){
		if(!listOfUsers[i]){
			listOfUsers[i] = msg;
			users_count++;
			return;
		}
	}
}

/* Delete user from userList */
void user_delete(msg_down_t *msg){
	int i;
	for(i=0;i<MAX_USERS;i++){
		if(listOfUsers[i]){
			if(listOfUsers[i]->m_addr == msg->m_addr && listOfUsers[i]->m_port == msg->m_port ){
				//free the content of the cell
				free(listOfUsers[i]);
				listOfUsers[i] = 0;
				users_count--;
				return;
			}
		}
	}
}

void *handlePacket(void *tArgs)
{
	int *client_fd = (int*) tArgs;

	msg_type_t msg_type; // get type of request from client
	int fd_request = *client_fd; //get file_desc from args

	if(recv(fd_request, (void*)&msg_type, sizeof(msg_type_t), MSG_PEEK) == -1){
		perror("Error! you are trying to connect with forign client - check API!");
		exit(1);
	}
	switch(msg_type) {
	case MSG_UP: { //check the client has send MSG_UP and return to the client MSG_ACK message
		printf("Got MSG_UP message\n");

		msg_up_t msg;
		if(recv(fd_request, (void*)&msg, sizeof(msg_up_t),0) == -1) {
			perror("read Messege \"UP\" fail");
			exit(1);
		}

		sleep(1);


		//after we fetched the data we need to generate MSG_ACK and send it to the client
		//get client information
		in_addr_t localClientIP = msg.m_addr; //client IP

		/*Get Local Port*/
		struct sockaddr_in local_address;
		socklen_t laddrlen = sizeof(local_address); //getsockname function will put the address and its length in this var
		getsockname( fd_request, (struct sockaddr *)&local_address, &laddrlen );
		in_port_t localClientPort =  local_address.sin_port;
		//printf("The ip:port of inComing connection is : %s:%d\n",inet_ntoa(*(struct in_addr *)&localClientIP),localClientPort);
		printf("The name of the user of inComing connection is : %s\n",msg.m_name);

		//since we are using a local system(localHost)I've added a increasing number to client port
		localClientPort+=++port_cnt; //add +1 to client port to register in our system in MSG_PEER list

		/*produce MSG_ACK message*/
		msg_ack_t msgToSend;
		msgToSend.m_port = localClientPort;
		msgToSend.m_type = MSG_ACK;

		/*register the client in our system!*/
		/*produce MSG_PEER messege*/

		/*Send MSG_ACK message*/
		if (send(fd_request, (void*) &msgToSend, sizeof(msgToSend), 0) < 0) {
			perror("Failed to send MSG_ACK..exiting..");
		}

		sleep(1);
		msg_peer_t *msgToSaveNServer = (msg_peer_t *)malloc(sizeof(msg_peer_t));
		msgToSaveNServer->m_addr = localClientIP;
		strcpy(msgToSaveNServer->m_name,msg.m_name); //copy from msg to Server (the name of the client)
		msgToSaveNServer->m_port = localClientPort;
		msgToSaveNServer->m_type = MSG_PEER;



		//save with our array  external function
		user_add(msgToSaveNServer);

		//printf("The ip of first user in array is: %s:\n",inet_ntoa(*(struct in_addr *)&actualArgs->programlist[0]->m_addr));
		//printf("The name of first user in array is: %s:\n",listOfUsers[0]->m_name);

	}break;

	case MSG_DOWN: {
		printf("Got MSG_DOWN message Removing User from SYSTEM\n");
		msg_down_t *msg = (msg_down_t *)malloc(sizeof(msg_down_t));
		if(recv(fd_request, msg, sizeof(msg_down_t),0) == -1) {
			perror("read Messege \"DOWN\" fail");
			free(msg);
			exit(1);
		}
		/*This messege will make the server remove the client from the system*/
		//external remove function from our array.
		user_delete(msg);
		free(msg);


	}break;

	case MSG_WHO: {
		printf("Got MSG_WHO message\n");
		printf("Sending MSG_HDR message\n");
		sleep(1);

		/*produce MSG_HDR message*/
		msg_hdr_t msgToSend;
		msgToSend.m_count = users_count;
		msgToSend.m_type = MSG_HDR;
		/*Send MSG_HDR message*/
		if (send(fd_request, (void*) &msgToSend, sizeof(msgToSend), 0) < 0) {
			perror("Failed to send MSG_HDR..exiting..");
		}
		/*meaning we are send MSG_HDR and than sleep(1) and than send MSG_PEERS*/
		sleep(1);
		printf("Sending [%d] Users Data please Hold On...",users_count);

		for(int i=0;i<MAX_USERS;i++){
			/*Send MSG_PEER messeges*/
			if(listOfUsers[i]){
				msg_peer_t sendMe;
				sendMe.m_addr = listOfUsers[i]->m_addr;
				sendMe.m_port = listOfUsers[i]->m_port;
				sendMe.m_type = listOfUsers[i]->m_type;
				strcpy(sendMe.m_name,listOfUsers[i]->m_name);

				if (send(fd_request,  &sendMe, sizeof(msg_peer_t), 0) < 0) {
					perror("Failed to send MSG_PEER..exiting..");
				}
			}
		}
		printf("\nall peers list sent successfully\n");

	}break;

	default: {
		printf("\nError! the client has not sent(or wrong)messege type at the first field of packet!\n");
	}
	} // end of switchcase

	pthread_detach(pthread_self());
	return 0;
}




