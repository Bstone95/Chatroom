# Chatroom
Basic Chatroom using multithreading to create a server and client placement
Server:
To create the chatroom we first compile and run the ChatroomServer file. We can open server using 

"./ChatroomServer <port>"
Example: ./ChatroomServer 12345

We will receive a message that the server has opened
While the server is running it will display users joining and any messages from clients in similar or seperate rooms so they can 
be monitored. The server will also notify when a user leaves a particular chatroom. 

Client:
We must first compile the ChatroomClient file. 
To connect to a chatroom we must first ensure a server port is open. Once this is complete, we can run the client using:

"./ChatroomClient <exampleaddress><portnumber>"
Example: ./ChatroomClient beta.bmt.lamar.edu 12345

When connected to the server a request will be made as to what chatroom user would like to join, they can simply
enter the name of the room in the prompt. Once joining the room the user can enter any message and it will be displayed
to the server as well as all other users, any message received from other users will be displayed,
users remain anonymous. When a user wants to exit they can simply type the phrase "EXIT" and they will be taken out
of the room. 
