/*********************************************************************
  ** Program Filename: chatclient.c
  ** Author: rlyth
  ** Date: 02/12/17
  ** Description: A simple command line chat client
  *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int const MSG_LENGTH = 500;

void getUsername(char name[]);
int initiateContact(char* hostname, char* portnum);
void handshake(int, char[], char[]);
int sendMsg(int, char[]);
int receiveMsg(int);

void error(const char *msg) { perror(msg); exit(0); }


int main(int argc, char *argv[]) {
	// Check arguments
    if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); }

	// Set username to a user-specified name of 10 chars or less
	char username[11];
	getUsername(username);
	printf("Hello %s.\n\n", username);
	
	printf("Connecting to %s...", argv[1]);
	
	// Get the connection socket
	int socketFD;
	socketFD = initiateContact(argv[1], argv[2]);
	
	// Send a cursory handshake, exchange handles
	char host[11];
	handshake(socketFD, username, host);
	
	printf(" Connection established.\n\n");
	
	printf("You are now talking to %s. Type '\\quit' to end the conversation.\n", host);
	int disconnect = 0;
	while(!disconnect) {
		// Returns 1 when user enters quit command
		if(sendMsg(socketFD, username)) {
			// send handwave
			send(socketFD, "bye", 3, 0);
			
			// Exit the loop early
			break;
		}
		
		// Returns 1 when server sends handwave
		disconnect = receiveMsg(socketFD);
	}
	
	close(socketFD);
	
	printf("Connection closed.\n");
	
	return 0;
}


/*********************************************************************
  ** Function: getUsername
  ** Description: Fills a string with a valid user-supplied name
  ** Parameters: A char that will hold the username
  ** Pre-Conditions: name is a valid string
  ** Post-Conditions: name contains a one-word username of max 10 chars
  *********************************************************************/
void getUsername(char name[]) {
	char temp[MSG_LENGTH]; // Temporary oversized string to mitigate overflow
	memset(temp, '\0', sizeof(temp));
		
	printf("Please enter a one-word username (up to 10 chars): ");
	fgets(temp, MSG_LENGTH-1, stdin);
	
	// Repeat prompt until valid input
	while(strlen(temp) > 11) {
		printf("That name is too long. Try again: ");
		fgets(temp, MSG_LENGTH-1, stdin);
	}
	
	// Copy valid string to name
	strncpy(name, temp, 10);
	
	// Chomp undesired characters
	name[strcspn(name, "\n")] = 0;
	name[strcspn(name, " ")] = 0;
	
	fflush(stdout);
}


/*********************************************************************
  ** Function: initiateContact
  ** Description: Establishes a connection to a host and returns an
		integer containing the connection socket. Based partially off
		of the examples from Beej's Guide to Network Programming
  ** Parameters: An array of chars containing the program's
		command-line arguments
  ** Pre-Conditions: argv contains the hostname at argv[1] and port #
		at argv[2]
  ** Post-Conditions: A valid socket is returned or the program exits
		with an error
  *********************************************************************/
int initiateContact(char *host, char *port) {
	int socketFD;
    struct addrinfo serverAddress;
	struct addrinfo *serverHostInfo, *ptr;
	
	// Set up the server address struct
	memset(&serverAddress, 0, sizeof(serverAddress)); // Clear out the address struct
	serverAddress.ai_family = AF_UNSPEC; // Create a network-capable socket
	serverAddress.ai_socktype = SOCK_STREAM; // TCP stream

	// Populate struct with host info
	if(getaddrinfo(host, port, &serverAddress, &serverHostInfo) != 0) {
		error("CLIENT: (getaddrinfo)");
	}
	
	ptr = serverHostInfo;
	int connected = 0;
	// Traverse list of addrinfo structs until connection is made or end is reached
	while(!connected && ptr != NULL) {
		// Attempt to set up the socket
		if((socketFD = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
			perror("CLIENT: (socket)");
		}
		
		// Attempt to connect the socket to the host
		if(connect(socketFD, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			close(socketFD);
		}
		else {
			// Successfully connected; exit loop
			connected = 1;
		}
		
		// Get the next addrinfo struct
		ptr = ptr->ai_next;
	}
	
	// No connection was made
	if (!connected) {
		error("CLIENT: (failed to connect)");
	}
	
	freeaddrinfo(serverHostInfo);
	
	return socketFD;
}


/*********************************************************************
  ** Function: handshake
  ** Description: A brief handshake with the client to exchange names
  ** Parameters: An int containing a connected socket, a string
		containing the client's handle, a string that will contain
		the host's handle
  ** Pre-Conditions: socketFD is an open socket connected to an
		appropriate server
  ** Post-Conditions: host contains the server's chat handle
  *********************************************************************/
void handshake(int socketFD, char name[], char host[]) {
	memset(host, '\0', sizeof(host));
	
	send(socketFD, name, sizeof(name), 0);
	recv(socketFD, host, sizeof(host), 0);
}


/*********************************************************************
  ** Function: sendMsg
  ** Description: Gets user input and sends it to the server
  ** Parameters: an integer containing a connected socket, a string
		containing the user's handle
  ** Pre-Conditions: The socket is valid and connected to an
		appropriate server
  ** Post-Conditions: The user's message has been sent OR the user
		has entered the quit command
  *********************************************************************/
int sendMsg(int socketFD, char username[]) {
	char buffer[MSG_LENGTH];
	// Deduct space for username from total message length
	char user_input[MSG_LENGTH - strlen(username) - 2];
	
	// Clear out the strings
	memset(buffer, '\0', sizeof(buffer));
	memset(user_input, '\0', sizeof(user_input));
	
	// Append prompt to buffer string
	strncat(buffer, username, strlen(username));
	strncat(buffer, "> ", 2);
	
	// Print prompt and get user input
	printf("%s", buffer);
	fgets(user_input, MSG_LENGTH, stdin);
	
	// Chomp newline
	user_input[strcspn(user_input, "\n")] = 0;
	
	// Return immediately if user types quit command
	if(strncmp(user_input, "\\quit", 5) == 0) {
		return 1;
	}

	// Append user input to buffer string and send completed message
	strncat(buffer, user_input, MSG_LENGTH);
	send(socketFD, buffer, sizeof(buffer), 0);

	fflush(stdout);
	
	return 0;
}

/*********************************************************************
  ** Function: receiveMsg
  ** Description: Waits for the server to send a message and prints
		it once received
  ** Parameters: an integer containing a connected socket
  ** Pre-Conditions: socketFD is an open socket connected to an
		appropriate server
  ** Post-Conditions: The server's message was printed OR the server
		sent the designated handwave to quit the chat
  *********************************************************************/
int receiveMsg(int socketFD) {	
	char buffer[MSG_LENGTH];
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer
	
	recv(socketFD, buffer, sizeof(buffer), 0);
	
	// If the handwave was sent, return true
	if(strncmp(buffer, "bye", 3) == 0) {
		return 1;
	}
	
	printf("%s\n", buffer);
	
	fflush(stdout);
	
	return 0;
}