#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#define NUMBER_OF_STRING 6
#define MAX_STRING_SIZE 40
#define ERR(source) (perror(source),\
		fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		exit(EXIT_FAILURE))

#define BACKLOG 3

volatile sig_atomic_t do_work = 1;
volatile sig_atomic_t usr = 1;

void usage(char * name){
	fprintf(stderr,"USAGE: %s socket port\n",name);
}

void sigint_handler(int sig) {
	do_work = 0;
}

void sigusr_handler(int sig)
{
    usr = 0;
}

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}
int Ready_Port(char* add, int port, struct sockaddr_in* address)
{
    int fdT;
    int t = 1; 

    if( (fdT = socket(AF_INET , SOCK_STREAM , 0)) == 0)  ERR("socket");
     
    if( setsockopt(fdT, SOL_SOCKET, SO_REUSEADDR, (char *)&t, 
          sizeof(t)) < 0 )  ERR("setsockopt");
     
    address->sin_family = AF_INET;  
    address->sin_addr.s_addr = inet_addr(add);  
    address->sin_port = htons(port);  
         
    if (bind(fdT, address, sizeof(*address))<0)  ERR("bind"); 
         
    if (listen(fdT, BACKLOG) < 0)  ERR("listen"); 
 
    puts("Czekam na polaczenia...");  
    return fdT;
}

void send_messages(int* client_socket, int max, char questions[][MAX_STRING_SIZE], int which[], int byte_num[], int byte_count[])
{
    for(int i = 0; i < max; i++)
    {
        if(client_socket[i] != 0 && which[i] != -1)
        {
            if(send(client_socket[i], questions[which[i]] + byte_num[i], 
            byte_count[i], 0) != byte_count[i])  ERR("send");

            byte_num[i] += byte_count[i];

            if(byte_num[i] + byte_count[i] - 1 == strlen(questions[which[i]]))
                which[i] = -1;

            byte_count[i] = rand()%MAX_STRING_SIZE;

            if(byte_num[i] + byte_count[i] - 1 > strlen(questions[which[i]]))
                byte_count[i] = strlen(questions[which[i]]) - byte_num[i];
        }      
    }
}

void read_from_file(char questions[][MAX_STRING_SIZE], char * path)
{
    FILE* fp;
    size_t buf = 0;
    char *linebuf = NULL;
    fp = fopen(path, "r");
    if(fp == NULL) ERR("fopen");
    for(int i = 0; i < NUMBER_OF_STRING; i++)
    {
        getline(&linebuf, &buf, fp);
        stpcpy(questions[i], linebuf);
    } 
}

void doServer(char* add, int port, int max, char* path)
{ 
    int main_socket, addrlen, new_socket, host_socket[max], event, valread , temp;  
    int max_socket;
    int which[max], byte_num[max], byte_count[max];
    struct sockaddr_in* address = malloc(sizeof(struct sockaddr_in));
    char buffer[1025];
    int isfree = 0;
    struct timeval tv = {0, 330000};
    fd_set readfds;  
    int flags;
    
    srand(time(NULL));
    char *message = "Witamy w quizie! \r\n";  
    char *denial = "NIE\n";
    char* end = "KONIEC\n";

    char questions[NUMBER_OF_STRING][MAX_STRING_SIZE];
    read_from_file(questions, path);
    for (int i = 0; i < max; i++)  
    {
        host_socket[i] = 0; 
        which[i] = -1;
        byte_num[i] = 0;
    }
         
    main_socket = Ready_Port(add, port, address);
    flags = fcntl(main_socket, F_GETFL, 0);
    fcntl(main_socket, F_SETFL, flags | O_NONBLOCK);

    addrlen = sizeof(*address);  
         
    while(do_work != 0)  
    {   
        isfree = 0;
        FD_ZERO(&readfds);  
     
        FD_SET(main_socket, &readfds);  
        max_socket = main_socket;  
             
        for (int i = 0 ; i < max ; i++)  
        {  
            temp = host_socket[i];   
            if(temp > 0)  
                FD_SET( temp , &readfds);     
            if(temp > max_socket)  
                max_socket = temp;  
        }  
     
        event = select( max_socket + 1 , &readfds , NULL , NULL , &tv);  
        if ((event < 0) && (errno!=EINTR))  ERR("select");
        
        tv.tv_sec = 0;
        tv.tv_usec = 330000;

        //nowe polaczenie
        if (FD_ISSET(main_socket, &readfds) && do_work != 0)  
        {  
            if ((new_socket = accept(main_socket, address, (socklen_t*)&addrlen))<0)  ERR("accept");
             
            printf("New connection, socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address->sin_addr) , ntohs(address->sin_port));  
                
            for (int i = 0; i < max; i++)  
            {  
                if( host_socket[i] == 0 )  
                {  
                    if(usr == 1)
                    {
                        host_socket[i] = new_socket;  
                        which[i] = rand()%NUMBER_OF_STRING;
                        byte_count[i] = rand()%strlen(questions[which[i]]);
                        flags = fcntl(host_socket[i], F_GETFL, 0);
                        fcntl(host_socket[i], F_SETFL, flags | O_NONBLOCK);
                        if(send(new_socket, message, strlen(message), 0) != strlen(message))  ERR("send");
                    }      
                    else
                    {
                        if(send(new_socket, denial, strlen(denial), 0) != strlen(denial))  ERR("send");
                        close(new_socket);
                    }
                    isfree = 1;
                    break;  
                }  
            }  
            if(isfree == 0)
            {
                if(send(new_socket, denial, strlen(denial), 0) != strlen(denial))  ERR("send");
                close(new_socket);
            }     
        }  

        //nowa wiadomosc od hosta
        for (int i = 0; i < max; i++)  
        {  
            temp = host_socket[i];  
                 
            if (FD_ISSET( temp , &readfds))  
            {  
                if ((valread = read( temp , buffer, 1024)) == 0)  
                {  
                    getpeername(temp , address , \
                        (socklen_t*)&addrlen);  
                    printf("Host sie rozlaczyl, ip %s , port %d \n", inet_ntoa(address->sin_addr) , ntohs(address->sin_port));  
                    which[i] = -1;
                    byte_num[i] = 0;
                    close( temp );  
                    host_socket[i] = 0;  
                }  
                     
                else 
                {  
                    which[i] = rand()%NUMBER_OF_STRING;
                    byte_count[i] = rand()%strlen(questions[which[i]]);
                    byte_num[i] = 0;
                }  
            }  
        }  
        if(do_work != 0)
            send_messages(host_socket, max, questions, which, byte_num, byte_count);
    }  
    //zamykanie polaczen
    for(int i = 0; i < max; i++)
    {
        if(host_socket[i] != 0 && FD_ISSET(host_socket[i], &readfds))
        {
            if(send(host_socket[i], end, strlen(end), 0) != strlen(end))  ERR("send");
            close(host_socket[i]);
        }
    }
    close(main_socket);
}

int main(int argc , char *argv[]) 
{
    if(argc!=5) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
    sethandler(sigint_handler, SIGINT);
    sethandler(sigusr_handler, SIGUSR1);
    doServer(argv[1], atoi(argv[2]), atoi(argv[3]), argv[4]);
    return EXIT_SUCCESS;
}