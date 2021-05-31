//Oświadczam, że niniejsza praca stanowiąca podstawę do uznania osiągnięcia efektówuczenia się z przedmiotu SOP2
//została wykonana przeze mnie samodzielnie. Wojciech Klusek 305943

#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#define MAX_STRING_SIZE 40
#define ERR(source) (perror(source),\
		fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		exit(EXIT_FAILURE))

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}
int make_socket(void){
	int sock;
	sock = socket(PF_INET,SOCK_STREAM,0);
	if(sock < 0) ERR("socket");
	return sock;
}
struct sockaddr_in make_address(char *address, char *port){
	int ret;
	struct sockaddr_in addr;
	struct addrinfo *result;
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	if((ret=getaddrinfo(address,port, &hints, &result))){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}
	addr = *(struct sockaddr_in *)(result->ai_addr);
	freeaddrinfo(result);
	return addr;
}
int connect_socket(char *name, char *port){
	struct sockaddr_in addr;
	int socketfd;
	socketfd = make_socket();
	addr=make_address(name,port);
	if(connect(socketfd,(struct sockaddr*) &addr,sizeof(struct sockaddr_in)) < 0){
		if(errno!=EINTR) ERR("connect");
		else { 
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if(TEMP_FAILURE_RETRY(select(socketfd+1,NULL,&wfds,NULL,NULL))<0) ERR("select");
			if(getsockopt(socketfd,SOL_SOCKET,SO_ERROR,&status,&size)<0) ERR("getsockopt");
			if(0!=status) ERR("connect");
		}
	}
	return socketfd;
}
ssize_t bulk_read(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(read(fd,buf,count));
		if(c<0) return c;
		if(0==c) return len;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}
ssize_t bulk_write(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(write(fd,buf,count));
		if(c<0) return c;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}

void print_answer(char question[MAX_STRING_SIZE]){
	printf("%s", question);
}
void do_client(int fd, char* test, char* question)
{
    char *line = NULL;
    size_t len = 0;
    int i = 0;
    char user[MAX_STRING_SIZE];
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    while(*test != '\n')
    {
        if(bulk_read(fd,(char *)test,sizeof(char)) < (int)sizeof(char)) ERR("read:");
        print_answer(test);
    }
    while(1)
    {
        test[0] = '\0';
        question[0] = '\0';
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        while(*test != '\n')
        {
            if(read(STDIN_FILENO, user, MAX_STRING_SIZE) > 0)
                printf("Nie teraz!\n");
            if(bulk_read(fd,(char *)test,sizeof(char)) < (int)sizeof(char)) ERR("read:");
            if(i == 0)
                stpcpy(question,test);
            else
                strcat(question,test);
            i++;
            fflush(stdin);
        }
        fcntl(STDIN_FILENO, F_SETFL, flags);
        print_answer(question);

         if(strlen(question) >= 7)
             if(strcmp(&question[strlen(question)- 7],"KONIEC\n") == 0)
                return;
        getline(&line, &len, stdin);
        if(bulk_write(fd,(char *)&line[0],sizeof(char[1]))<0) ERR("write:");
    }
}
void usage(char * name){
	fprintf(stderr,"USAGE: %s\n",name);
}
int main(int argc, char** argv) {
	int fd;
    
    char* test = malloc(sizeof(char));
    char* question = malloc(sizeof(char[MAX_STRING_SIZE]));
	if(argc!=3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	fd=connect_socket(argv[1],argv[2]);

    do_client(fd, test,question);

	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}
