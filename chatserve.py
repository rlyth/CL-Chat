########################################################################
  ## Program Filename: chatserve.py
  ## Author: rlyth
  ## Date: 02/11/17
  ## Description: A simple chat server, initialization code based on 
  ##	the example SocketServer provided by the Python documentation
  ######################################################################

import sys
import SocketServer
import signal

MSG_LENGTH = 500

## Defines a handler for the socket server
## Sends and receives messages with client
class ChatServer(SocketServer.BaseRequestHandler):
	def handle(self):
		## Gets input from client and displays it to user
		def receiveMsg():
			msg = self.request.recv(MSG_LENGTH).rstrip()
			
			# Check if the client has sent the handwave
			if(msg == 'bye'):
				return True
			# Otherwise print message from client
			else:
				print msg
				return False
				
		## Gets input from user and sends it to client
		def sendMsg():
			user_input = raw_input(serverName + "> ")
			
			# Check if user has entered quit command
			if user_input == '\\quit':
				return True
			
			# Append host name to the front of the user's message
			msg = serverName + "> " + user_input
			self.request.send(msg)
			
			return False
		
		serverName = "notHAL"
	
		# Respond to the client's handshake, exchange names
		handshake = self.request.recv(MSG_LENGTH)
		self.request.send(serverName)
	
		print("Connection accepted. Now talking to " + handshake + ".")
		print("Type '\quit' to end the conversation.")
		
		# Chat continues until either user quits
		while True:
			# Returns true when the client has quit
			if (receiveMsg()):
				print(handshake + " has left the chat.\n")
				return
			
			# Returns true when server decides to quit
			if(sendMsg()):
				# Send handwave to client
				self.request.send("bye")
				print("Connection closed.\n")
				return
	
	
## Accepts name of host (string) and port number (int)
## Creates a TCP server and listens indefinitely
def startUp(host, port):
	server = SocketServer.TCPServer((host, port), ChatServer)
	
	print("Waiting for connections...\n")
	
	try:
		server.serve_forever()
	except KeyboardInterrupt:
		print("\nInterrupt received, halting program.")
	finally:
		server.server_close()


##################################################
## Main program begins here
##################################################

# Check arguments
if len(sys.argv) < 2:
	print("ERROR: Proper usage is %s port" % sys.argv[0])
	sys.exit()

# Initialize server and start listening
startUp('', int(sys.argv[1]))