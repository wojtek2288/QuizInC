# QuizInC
Console application written in C implementing simple server and clients communication. Server is sending random questions  from a file to clients via TCP and waits for their response. Server uses one thread and makes connection with many clients.  

Input arguments for server program: address of server, port number, max number of clients, path to file containing questions. 
Input arguments for cilent program: server address, port number.
