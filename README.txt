P2p chat System written in C language:

-ChatBasedOnFD
	
	when connection is established between peers, this program will "take the 	wheels" and communicate with user on each side of the connection(via 	terminal)

-Final_Client
	
	This folder contains the client connecting to the server
	The client will connect to server, register and than ask a list of 	connected users, the client can connect directly to another PEER without 	the server!
	

chatc.c - a client, several copies of which may be active concurrently.
-Final_Server

	This folder contains the Server: the server responsible to register the 	clients and provide list of connected clients to who's asking

chats.c - the server program, only one of which should be active.


Notice!!!
When a connection is established in clientP2P he’s does “fork()“ and open my third-party program called “ChatBasedOnFD”
So that file needs to be compiled(ChatBasedOnFD) in the client folder when running the app.

 * The server keeps track of which clients are currently online. Clients notify
 * the server of their presence once they come up, and also when they plan to 
 * terminate. To account for error termination of a client, clients confirm 
 * their continued presence by periodically sending an upkeep message. 
 * The server will remove a client from its "currently online" list if it 
 * doesn't hear from that client for longer than the upkeep message period. 
 * 
 * Clients may request the list of all clients that are currently online. 
 * This list specifies, for each online client, known as "peer", its IP address
 * and port number, to which the peer is supposed to be listening, and its 
 * "display name", which could be the name of the client's user or any other 
 * string the user may choose to use as identification. 
 *
 * A peer, then, may initiate a chat with any other peer by sending a chat 
 * message to its IP address and port. This establishes a link over which the 
 * two may chat until either side decides to terminate the chat. 
 *
 *
 * The protocol is as follows:
 *
 * 1. A client sends the server a MSG_UP message when it first comes up, 
 *    notifying the server that it is now up and ready. The message contains 
 *    the IP address of the client, and the name with which he or she would 
 *    like to be identified in any published listing ("display name").  
 *
 *    The server generates a new port number for that client for receiving 
 *    chat requests from other peers. The server responds with a MSG_ACK 
 *    message containing that generated port number. 
 *
 * 2. The client sends the server a MSG_DOWN message to indicate its wish 
 *    to disconnect from the chat room. 
 *
 *    The server need not respond to this message, but it should remove 
 *    the sending client from its "currently online" list.
 *
 * 3. A client may ask the server to see who is currently online, using
 *    the message MSG_WHO. 
 *
 *    The server responds to this message with MSG_HDR containing the number 
 *    of online clients, followed by that number of MSG_PEER messages. 
 *    Each MSG_PEER message contains the the IP address and port number  
 *    some online client is expecting messages on. 
 *
 *    Care should be taken here that the "currently online" list is not 
 *    modified while this is done. Otherwise, the number of MSG_PEER messages 
 *    sent may disagree with the number stated in MSG_HDR, causing either the 
 *    server or client to hang (waiting for additional sends or receives). 
 *    
 * 4. Any client, say S, may start a chat session with any other client
 *    currently registered with the server, say R, by sending it a MSG_CONN 
 *    message. The message contains S's particulars, among them the IP address
 *    and port number S is listening to. 
 *    
 *    The receiving client, R, responds with a MSG_RESP message indicating
 *    its willingness to engage in a chat: it contains a field with TRUE if 
 *    it does, and FALSE if not. R may decline an incoming chat if it is 
 *    engaged in one already. 
 *
 *    As part of its preparation for a chat, R opens a new socket and connects 
 *    to the port S is listening to. This arrangement allows for asynchronous 
 *    full duplex communications between the two clients.
 *
 * 5. Once a chat is established, S and R may send text messages to each other
 *    asynchronously by using the MSG_TEXT message. The message contains size 
 *    of the text one wishes to convey to the other, followed by the text 
 *    itself. 
 *
 *    Each client displays the text it receives on the standard output, 
 *    preceded by the display-name of the client who sent it. 
 * 
 *    Either client, S or R, may terminate the chat by sending an MSG_END message.
 *    The other party to the chat, the one receiving the message, closes the 
 *    relevant sockets at its side. Now both peers are ready to participate in
 *    other chat sessions with other peers. 
 *
 * The following extensions to the protocol are optional.
 *
 * 6. A special message type, MSG_SHUTDOWN, is in place to assist in complete 
 *    shutdown of the chat system. Any client may send such a message to the 
 *    server and all registered clients. A recipient of such a message should 
 *    clean up, close all connections and terminate. 
 *
 * 7. A special message type, MSG_DUMP, is in place to assist in obtaining
 *    the internal state of the server or client, for the purpose of debugging.
 *
 * 8. The client sends the server a MSG_ALIVE messages every C_ALIVE seconds. 
 *    This message is aimed at positively verifying that the client is still 
 *    alive even though it may have produced no traffic. 
 *
 *    The server responds to this message with a MSG_ACK message as 
 *    well, containing the port number initially assigned to this client. 
 *    However, if two consecutive C_ALIVE seconds elapse with no MSG_ALIVE 
 *    received, the server may removes the respective client from its 
 *    "currently online" list.
 *
 *    If a MSG_ALIVE message is received from a client that is not in the 
 *    "currently online" list, the server responds with a MSG_NACK instead. 
 *    The client should understand from this response that it should 
 *    re-establish communications with the server (using MSG_UP again).
 */