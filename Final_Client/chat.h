/**
 *  chat.h - Header file for network communications/threads exercise
 *
 *  part of a chat system
 *
 *
 * This file is part of a peer-to-peer system which allows participating
 * clients to chat with each other on line. It consists of several source files:
 *
 * 1. chat.h - a common header file.
 * 2. chats.c - the server program, only one of which should be active.
 * 3. chatc.c - a client, several copies of which may be active concurrently.
 *
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

#include <netinet/in.h>

/*
 *  Port number for chat server.
 *  Also the base for all port numbers allocated to peers.
 */

#define C_SRV_PORT 	12360

/*
 *  Period of time (in seconds) a client should wait between
 *  issues of MSG_ALIVE messages.
 */
#define C_ALIVE 	20

/*
 *  Size of buffer to read and write text messages
 */
#define C_BUFF_SIZE 	1024

/*
 *  Maximum size of display name
 */
#define C_NAME_LEN 	24



/*
 *  message types
 */
#define MSG_ERR     	 0

#define MSG_UP    	 	 1
#define MSG_DOWN      	 2

#define MSG_ALIVE     	11
#define MSG_ACK       	12
#define MSG_NACK       	13

#define MSG_WHO    		21
#define MSG_HDR    		22
#define MSG_PEER    	23

#define MSG_CONN   		31
#define MSG_RESP   		32
#define MSG_TEXT   		33
#define MSG_END   		34

#define MSG_DUMP   		91
#define MSG_SHUTDOWN	92



typedef int msg_type_t;

/*
 *  message formats: MSG_UP
 *  Used by clients to notify the server of their existence.
 */
typedef struct msg_up
{
    msg_type_t	m_type;	 			// = MSG_UP
    in_addr_t   m_addr;				// client's IP address
    char	m_name[C_NAME_LEN + 1];		// client display name
}
    msg_up_t;




/*
 *  message formats: MSG_DOWN
 *  Used by a client to notify the server that it is about to terminate.
 */
typedef struct msg_down
{
    msg_type_t	m_type;	 			// = MSG_UP
    in_addr_t   m_addr;				// client's IP address
    in_port_t   m_port;				// client's assigned port
}
    msg_down_t;



/*
 *  message formats: MSG_ALIVE
 *  Used by clients to maintain their existence.
 */
typedef struct msg_alive
{
    msg_type_t	m_type;	 			// = MSG_ALIVE
    in_addr_t   m_addr;				// client's IP address
    in_port_t   m_port;				// client's assigned port
}
    msg_alive_t;



/*
 *  message formats: MSG_ACK
 *  The way the server responds to MSG_UP and MSG_ALIVE client messages.
 *  It contains the client's assigned port number.
 */
typedef struct msg_ack
{
    msg_type_t	m_type;				// = MSG_ACK
    in_port_t   m_port;				// client's assigned port
}
    msg_ack_t;



/*
 *  message formats: MSG_NACK
 *  The server responds to MSG_ALIVE from a non-existing client with MSG_NACK.
 *  The client should infer from the response that the server is not aware of
 *  its existence, and should reinitialize, sending a MSG_UP message.
 */
typedef struct msg_nack
{
    msg_type_t	m_type;				// = MSG_NACK
}
    msg_nack_t;



/*
 *  message formats: MSG_WHO
 *  Used by clients to obtain a list of the clients currently online.
 */
typedef struct msg_who
{
    msg_type_t	m_type;				// = MSG_WHO
}
    msg_who_t;



/*
 *  message formats: MSG_HDR
 *  Used by the server to respond to MSG_WHO, detailing the current
 *  number of peers. This is also the number of MSG_PEER messages
 *  that should follow.
 */
typedef struct msg_hdr
{
    msg_type_t	m_type;				// = MSG_HDR
    int         m_count;			// number of peers in list
}
    msg_hdr_t;



/*
 *  message formats: MSG_PEER
 *  Used by the server to convey the details of a single peer.
 */
typedef struct msg_peer
{
    msg_type_t	m_type;				// = MSG_PEER
    in_addr_t   m_addr;				// Peer's IP address
    in_port_t   m_port;				// Peer's port number
    char        m_name[C_NAME_LEN + 1];		// Peer's display name
}
    msg_peer_t;



/*
 *  message formats: MSG_CONN
 *  Used by a peer to request a chat connection:
 */
typedef struct msg_conn
{
    msg_type_t	m_type;				// = MSG_CONN
    in_addr_t   m_addr;				// Sender's IP address
    in_port_t   m_port;				// Sender's port number
	char   	 	m_name[C_NAME_LEN + 1];		// Sender's display name
}
    msg_conn_t;



/*
 *  message formats: MSG_RESP
 *  Used by a peer to respond to a chat connection request:
 */
typedef struct msg_resp
{
    msg_type_t	m_type;				// = MSG_RESP
	int         m_agree;			// 1 if willing, 0 to decline
}
    msg_resp_t;



/*
 *  message formats: MSG_TEXT
 *  Used by peers to chat:
 */
typedef struct msg_text
{
    msg_type_t	m_type;				// = MSG_TEXT
    int			m_size;				// size of text
    char        m_text[0];			// text of given size
}
    msg_text_t;



/*
 *  message formats: MSG_END
 *  Used by peers to signal the end of a chat
 */
typedef struct msg_end
{
    msg_type_t	m_type;				// = MSG_END
}
    msg_end_t;



/*
 *  message formats: MSG_DUMP
 *  Used by anyone to cause the receiver to print
 *  its internal state (dump): its perceived port number,
 *  the peers with which it is currently chatting, etc.
 *  This can be useful while debugging the system.
 */
typedef struct msg_dump
{
    msg_type_t	m_type;				// = MSG_DUMP
}
    msg_dump_t;



/*
 *  message formats: MSG_SHUTDOWN
 *  Used by the server to bring down the system.
 *  Any client receiving this message should terminate.
 */
typedef struct msg_shutdown
{
    msg_type_t	m_type;				// = MSG_SHUTDOWN
}
    msg_shutdown_t;





