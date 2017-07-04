/*
 * chatc.c
 *
 *  Created on: Jun 6, 2017
 *      Author: Eran Peled
 *
 *      This File is the implement of P2P Client that can also act as a server and a client
 *      Meaning we will have Full-Duplex ability during chat session between two users
 */

#include "chat.h"
#include <stdio.h> //printf
#include <stdlib.h> //exit, perror
#include <string.h> //memset
#include <arpa/inet.h> //inet_addr
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define NUM_OF_USERS 10 //total number of users that can be register
#define MAX_USERS 50
static unsigned int peers_count = 0;
static msg_peer_t *listOfPeers[MAX_USERS] = {0}; //global array of connected users to the system. notice the {0}initializer it is very important!
static	pthread_t listen_tid;
static char userSelection;

static void handler(int signum)
{
	pthread_exit(NULL);
}
/*eraseList function*/
void erase_all_users();
/* Add user to userList */
void user_add(msg_peer_t *msg);
/* Delete user from userList */
void user_delete(msg_down_t* msg);
/*get localIP Address using socket_fd*/
in_addr_t sockfd_to_in_addr_t(int sockfd);
/*function that connects client to the server sends MSG_UP and gets MSG_ACK*/
void connect_server(struct sockaddr_in * server_conn,msg_ack_t * server_assigned_port,int *server_fd,in_addr_t *localIP,char usr_input[C_BUFF_SIZE]);
/*function that opens a the client for incoming connection(runs in a new thread)*/
void *listenMode(void *args);
/*generate menu for our p2p client*/
int openChat(int fd); //opens new windows with xterm to chat.
/*When user sends MSG_WHO this method will get all users from our server*/
void getListFromServer(struct sockaddr_in * server_conn);
/*Remove peer from server*/
void removePeerFromServer(struct sockaddr_in * server_conn,msg_ack_t * server_assigned_port);
/*Handle Peer connection*/
void *handlePeerConnection(void *tArgs);
/*the peer choose which peer he wants to connect with*/
void selectPeerToConnect(struct sockaddr_in * out_sock,msg_ack_t * server_assigned_port,in_addr_t *localIP,char usr_input[C_BUFF_SIZE]);


void generate_menu(){
	printf("Hello dear user pls select one of the following options:\n");
	printf("[0]\t-\t Send MSG_WHO to server - get list of all connected users and choose peer to chat with\n");
	printf("[9]\t-\t Send MSG_DOWN to server - unregister ourselves from server\n");
}

int main(int argc, char *argv[])
{
	/*program Vars*/
	int server_fd= 0;
	msg_ack_t server_assigned_port;
	struct sockaddr_in server_conn,incoming_sck;
	struct sockaddr_in out_sck;
	in_addr_t localIP; //our peer IP Address
	char usr_input[C_BUFF_SIZE]; //for user input during the program
	/***************************************/
	signal(SIGUSR1, handler);
	memset(&server_conn, 0, sizeof(struct sockaddr_in));
	memset(&incoming_sck, 0, sizeof(struct sockaddr_in));
	memset(&out_sck, 0, sizeof(struct sockaddr_in));
	/*First*/
	/*the client connects to the server sends MSG_UP and gets MSG_ACK*/
	connect_server(&server_conn,&server_assigned_port,&server_fd,&localIP,(char *)&usr_input);
	printf("Congartulation your assign port number by the server is:%d\n",server_assigned_port.m_port);
	/*now the "Client" needs to start listen to incoming connections in a new thread*/
	/***************************** The Algorithm ********************************************************
	 **The algorithm is : we are listing for incoming connections at all time                          **
	 **if someone sends us MSG_CONN and we are at prompt, meaning not trying to connect to anyone      **
	 **we accept the connection and going to chat mode(opening a new terminal)                         **
	 **if someone is trying to connect with us and we are busy we will send a "busy" tone = MSG_RESP = 0*
	 **meantime as long we don't have a incoming connection we will present the user with menu and ask **
	 **him what he want to do                                                                          **
	 ****************************************************************************************************/

	/*create "another" main for incoming connections in thread*/
	if(pthread_create(&listen_tid, NULL, listenMode, (void*)&server_assigned_port) != 0)perror("could not create thread");
	/*while true ->>> present menu and if connection initiate new terminal windows*/

	do{
		generate_menu();
		userSelection = (char)getchar();
		sleep(1);
		if(userSelection == '9'){
			removePeerFromServer(&server_conn,&server_assigned_port);

		}
		if(userSelection == '0'){
			getListFromServer(&server_conn);
			selectPeerToConnect(&out_sck,&server_assigned_port,&localIP,(char *)&usr_input);
		}
		sleep(1);
	}
	while( userSelection != '9');

	pthread_join(listen_tid,NULL);
	//*The connection is closed by server in each communication!//

	return 0;
}



/*eraseList function*/
void erase_all_users(){for(int i=0;i<MAX_USERS;i++){free(listOfPeers[i]);listOfPeers[i]=0;}}
/* Add user to userList */
void user_add(msg_peer_t *msg){
	if(peers_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
	}
	int i;
	for(i=0;i<MAX_USERS;i++){
		if(!listOfPeers[i]){
			listOfPeers[i] = msg;
			peers_count++;
			return;
		}
	}
}

/* Delete user from userList */
void user_delete(msg_down_t *msg){
	int i;
	for(i=0;i<MAX_USERS;i++){
		if(listOfPeers[i]){
			if(listOfPeers[i]->m_addr == msg->m_addr && listOfPeers[i]->m_port == msg->m_port ){
				//free the content of the cell
				free(listOfPeers[i]);
				listOfPeers[i] = 0;
				peers_count--;
				return;
			}
		}
	}
}
/*get localIP Address using socket_fd*/
in_addr_t sockfd_to_in_addr_t(int sockfd)
{
	int s = sockfd;
	struct sockaddr_in sa;
	socklen_t sa_len;
	/* We must put the length in a variable.              */
	sa_len = sizeof(sa);
	/* Ask getsockname to fill in this socket's local     */
	/* address.                                           */
	if (getsockname(s, (struct sockaddr *)&sa, &sa_len) == -1) {
		perror("getsockname() failed");
		return -1;
	}
	return sa.sin_addr.s_addr;

}

/*function that connects client to the server sends MSG_UP and gets MSG_ACK*/
void connect_server(struct sockaddr_in * server_conn,msg_ack_t * server_assigned_port,int *server_fd,in_addr_t *localIP,char usr_input[C_BUFF_SIZE])
{
	/*function VARS*/
	char ip_str[INET_ADDRSTRLEN]; //holds the server IPv4 address in str form
	/************************************/
	/*The Client is trying to register in our *running* server, if it fails close client!*/
	printf("Hello dear Client pls write the server IP Address:(by the following format xxx.xxx.xxx.xxx)\n");
	printf("Press 'Enter' For Default IP\n");

	fgets(ip_str,INET_ADDRSTRLEN,stdin); //gets input from usr
	if(ip_str[0]==10){
		strcpy(ip_str,"127.0.0.1");
	}

	// store this IP address,port and IPv4 settings in server_conn:
	inet_pton(AF_INET, ip_str, &server_conn->sin_addr);
	server_conn->sin_family = AF_INET;
	//server_conn.sin_addr.s_addr = inet_addr(ip_str);
	server_conn->sin_port = htons(C_SRV_PORT);
	//if the IP is inValid exit program!
	if((server_conn->sin_addr.s_addr = inet_addr(ip_str)) == -1)	{
		perror(strerror(EAFNOSUPPORT));
		exit(1);
	}
	//open socket
	if((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\n Error : Could not create socket \n");
		exit(1);
	}

	if( connect(*server_fd, (struct sockaddr *)server_conn, sizeof(*server_conn)) < 0){
		perror("connect");
		exit(1);
	}

	puts("Connected to server, Please register in order to fetch user List\n");
	puts("Please write your name to register(no longer than 24 chars)\n");
	fgets(usr_input,C_BUFF_SIZE,stdin); //gets input from usr
	while(strlen(usr_input) > 24)
	{
		perror("the name you've entered is too big!\n");
		perror("Please try again!\n");
		fgets(usr_input,C_BUFF_SIZE,stdin); //gets input from usr
	}
	//send MSG_UP to our server
	msg_up_t sendToServer;
	sendToServer.m_type = MSG_UP;
	strcpy(sendToServer.m_name,usr_input);
	*localIP = sockfd_to_in_addr_t(*server_fd);
	sendToServer.m_addr = *localIP;

	//Send MSG_UP
	if( send(*server_fd ,(void*)&sendToServer, sizeof(sendToServer) , 0) < 0){
		puts("Send failed");
		exit(1);
	}
	puts("MSG_UP Sent to server\n");
	//get MSG_ACK message from server and start listen at the port inside MSG_ACK
	if(recv(*server_fd, server_assigned_port, sizeof(msg_ack_t),0) == -1) {
		perror("read Messege \"MSG_ACK\" fail");
		exit(1);
	}
	puts("Success : got MSG_ACK message");

}

/*This function is used to fork and execute a chat program in a new terminal*/
int openChat(int fd)
{
	char ascii_fd[1] = {fd + '0'};
	pid_t child_pid;
	/* Duplicate this process. */
	child_pid = fork ();
	if (child_pid != 0)
		/* This is the parent process. */
		return child_pid;
	else {
		/* Now execute CHAT */
		/*Note! because we are using fork we have the fd of connected_client*/
		/*The Program we are running : is doing  = While loop chat until MSG_END*/
		//char *args[] = {"/usr/bin/xterm", "-e","./ChatBasedOnFD",ascii_fd, NULL };
		char *args[] = {"/usr/bin/xterm","-xrm","'XTerm.vt100.allowTitleOps: false'","-T","P2P Chat Window", "-e","./ChatBasedOnFD",ascii_fd, NULL };
		execv("/usr/bin/xterm", args);
		/* The execl function returns only if an error occurs. */
		fprintf (stderr, "an error occurred in execlp\n");
		abort ();
	}
	return 0;
}

/*function that opens a the client for incoming connection(runs in a new thread)*/
void *listenMode(void *args)
{
	static sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);

	/*we have the port num start listen in that port for incoming connections*/
	/*got it from "main"*/
	/*Define vars for communication*/
	msg_ack_t *actualArgs = (msg_ack_t*) args;
	int socket_fd,client_fd;
	pthread_t t; //thread for the accepted request
	struct sockaddr_in addr;
	int sockfd, ret;
	/*******************************/
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	if (sockfd < 0) {
		printf("Error creating socket!\n");
		exit(1);
	}
	printf("Socket created...\n");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = actualArgs->m_port;


	ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0) {
		printf("Error binding!\n");
		exit(1);
	}
	printf("Binding done...\n");

	printf("Waiting for peer connection, Listening on Port:%d\n",addr.sin_port);
	listen(sockfd, 1);

	while(1)
	{
		//accept connection from an incoming pers
		client_fd = accept(sockfd, (struct sockaddr *)NULL, NULL); //busy-waiting...

		if (client_fd < 0)
		{
			perror("try to accept incoming connection failed");
		}
		else{
			puts("Connection accepted sending MSG_ACK to client");
		}

		/*prompt the user if he want to accept call*/
		/*in a new thread*/
		/*there deal with input*/
		if(pthread_create(&t, NULL, handlePeerConnection, (void*)&client_fd)!= 0) {

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

/*When user sends MSG_WHO this method will get all users from our server*/
void getListFromServer(struct sockaddr_in * server_conn){
	/*create a new sockaddr for a new connection*/
	struct sockaddr_in serv_who_peer;
	memset(&serv_who_peer, 0, sizeof(serv_who_peer));
	serv_who_peer.sin_family = AF_INET; //IPv4 Structure
	/*reusing struct sockaddr_in in main*/
	serv_who_peer.sin_addr = server_conn->sin_addr;
	serv_who_peer.sin_port = htons( C_SRV_PORT ); //port num defined in chat.h

	int server_fd = 3;//that number will be erased by socket
	//open socket

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\n Error : Could not create socket \n");
		exit(1);
	}

	if( connect(server_fd, (struct sockaddr *)&serv_who_peer, sizeof(*server_conn)) ==-1){
		perror("connect to server to get MSG_WHO refused");
		exit(1);
	}

	//send MSG_WHO to our server
	msg_who_t sendToServer;
	sendToServer.m_type = MSG_WHO;

	//Send MSG_WHO
	if( send(server_fd ,(void*)&sendToServer, sizeof(sendToServer) , 0) < 0){
		puts("Sending MSG_WHO failed");
		exit(1);
	}
	puts("MSG_WHO Sent to server\n");

	sleep(1);

	//get MSG_HDR message from server and start getting the list of connected users
	msg_hdr_t HdrMsgFromServer;
	if(recv(server_fd, &HdrMsgFromServer, sizeof(msg_hdr_t),0) == -1) {
		perror("read Messege \"MSG_HDR\" fail");
		exit(1);
	}
	puts("Success : got MSG_HDR message");
	sleep(1);
	/*in loop of number of connected users fill our array(peer_msg array) with data*/

	printf("Got %d peers from Server:...\n",HdrMsgFromServer.m_count);


	//clear array from previous results
	erase_all_users();

	for(int i=0;i<HdrMsgFromServer.m_count;i++){
		msg_peer_t *PeerMsgFromServer = (msg_peer_t *)malloc(sizeof(msg_peer_t));
		if(!PeerMsgFromServer)perror("malloc"); //check if malloc failed
		if(recv(server_fd, PeerMsgFromServer, sizeof(msg_peer_t),0) == -1) {
			perror("recv read Messege \"MSG_PEER\" fail");
			exit(1);
		}

		//save with our array  external function
		user_add(PeerMsgFromServer);
	}

}

/*Remove peer from server*/
void removePeerFromServer(struct sockaddr_in * server_conn,msg_ack_t * server_assigned_port){
	/*create a new sockaddr for a new connection*/
	struct sockaddr_in serv_who_peer;
	memset(&serv_who_peer, 0, sizeof(serv_who_peer));
	serv_who_peer.sin_family = AF_INET; //IPv4 Structure
	/*reusing struct sockaddr_in in main*/
	serv_who_peer.sin_addr = server_conn->sin_addr; //opened to any IP
	serv_who_peer.sin_port = htons( C_SRV_PORT ); //port num defined in chat.h

	int server_fd = 3; //that number will be erased by socket
	//open socket

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("\n Error : Could not create socket \n");
		exit(1);
	}

	if( connect(server_fd, (struct sockaddr *)&serv_who_peer, sizeof(*server_conn)) ==-1){
		perror("connect to server to send MSG_DOWN refused");
		exit(1);
	}

	//send MSG_DOWN to our server
	msg_down_t sendToServer;
	sendToServer.m_type = MSG_DOWN;
	sendToServer.m_port = server_assigned_port->m_port;
	sendToServer.m_addr = sockfd_to_in_addr_t(server_fd);

	//Send MSG_DOWN
	if( send(server_fd ,(void*)&sendToServer, sizeof(sendToServer) , 0) < 0){
		puts("Sending MSG_DOWN failed");
		exit(1);
	}
	puts("MSG_DOWN Sent to server\n");
	puts("Closing All Important Things..., Goodbye\n");
	puts("It's OK to Close the Window Now OR enter ctrl+c\n");
	pthread_exit(&listen_tid);
	pthread_cancel(listen_tid);
	pthread_kill(listen_tid,SIGKILL);
	exit(1);

}

void *handlePeerConnection(void *tArgs)
{
	int *client_fd = (int*) tArgs;
	openChat(*client_fd);

	pthread_detach(pthread_self());
	return 0;
}

/*the peer choose which peer he wants to connect with*/
void selectPeerToConnect(struct sockaddr_in * out_sock,msg_ack_t * server_assigned_port,in_addr_t *localIP,char usr_input[C_BUFF_SIZE]){
	/*Function VARS*/
	int equlsPeerFD = -1; //used to open a new chat windows using FD Number
	msg_peer_t peerSelection;
	int userSelection=-1;
	/****************/
	/*print all connected Peers*/
	for(int i=0;i<MAX_USERS;i++){
		/*Send MSG_PEER messeges*/
		if(listOfPeers[i]){
			/*print connected users*/
			printf("\n[%d]\t-\t Username : %s\n",i,listOfPeers[i]->m_name);
			printf("[%d]\t-\t IP : %s\n",i,inet_ntoa(*(struct in_addr *)&listOfPeers[i]->m_addr));
			printf("[%d]\t-\t Port : %d\n\n",i,listOfPeers[i]->m_port);
		}
	}

	/*Let user choose to which peer he want's to connect*/
	printf("Please Choose the client you want to chat with:(By Number)\n");

	scanf( "%d", &userSelection);

	/*Testing Purps*/
	// printf("%d",userSelection);

	/*create msg_conn_t with all data required*/
	msg_conn_t sendToPeer;
	sendToPeer.m_type = MSG_CONN;
	strcpy(sendToPeer.m_name,usr_input);
	sendToPeer.m_addr = *localIP;

	/*Fetch Peer data by user choise*/

	/*******************************/
	if(listOfPeers[userSelection]!=0)
	{
		peerSelection.m_addr = listOfPeers[userSelection]->m_addr;
		peerSelection.m_port = listOfPeers[userSelection]->m_port;
		peerSelection.m_type = listOfPeers[userSelection]->m_type;
		strcpy(peerSelection.m_name,listOfPeers[userSelection]->m_name);
		/***************************************************/
		/*Testing Purps*/
		printf("\nYou choose: %s\n",peerSelection.m_name);
		printf("The IP:port of Requested peers is : %s:%d\n",inet_ntoa(*(struct in_addr *)&peerSelection.m_addr),peerSelection.m_port);

		/*open socket, connect to other peer, get FD, send msg_conn_t and wait for RESPONSE from other PEER */
		out_sock->sin_family = AF_INET;
		out_sock->sin_addr.s_addr = peerSelection.m_addr;
		out_sock->sin_port = server_assigned_port->m_port;
		//open socket
		if((equlsPeerFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			printf("\n Error : Could not create socket \n");
			exit(1);
		}

		if(connect(equlsPeerFD, (struct sockaddr *)out_sock, sizeof(*out_sock)) < 0){
			perror("connect");
			exit(1);
		}

		printf("Connected to EqualPeer -%s-  Sending MSG_CONN and Waiting for Response...\n\n",peerSelection.m_name);


		openChat(equlsPeerFD);
	}


}

