#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<signal.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);
void sig_int(int signo);

char msg[BUF_SIZE];

int sock;

int main(int argc, char *argv[])
{

	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void *thread_return;
	struct sigaction act;

	act.sa_flags=0;
	sigemptyset(&act.sa_mask);
	act.sa_handler = sig_int;
	sigaction(SIGINT,&act,0);

	if(argc!=3)
	{
		printf("Usage : %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	sock=socket(PF_INET,SOCK_STREAM,0);
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if(connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
		error_handling("connect() error");
	
	pthread_create(&snd_thread,NULL,send_msg,(void*)&sock);
	pthread_create(&rcv_thread,NULL,recv_msg,(void*)&sock);
	pthread_join(snd_thread,&thread_return);
	pthread_join(rcv_thread,&thread_return);
	close(sock);
	return 0;
}
void *send_msg(void *arg)
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	while(1)
	{
		fgets(name_msg,BUF_SIZE,stdin);
		write(sock,name_msg,strlen(name_msg));
	}
	return NULL;
}
void *recv_msg(void *arg)
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE+BUF_SIZE];
	char opt[NAME_SIZE+BUF_SIZE];
	char * opt_pt;
	int str_len;
	while(1)
	{
		str_len=read(sock,name_msg,NAME_SIZE+BUF_SIZE-1);
		if(str_len==-1)
			return(void*)-1;
		name_msg[str_len]=0;

		strcpy(opt,name_msg);
		opt[7]='\0';
		opt_pt=&name_msg[7];
		if(strcmp(opt,"*#205*#")==0){
			if(strcmp(opt_pt,"clear")==0){
				system("clear");
			}
			else if(strcmp(opt_pt,"exit")==0){
				close(sock);
				exit(1);
			}
			fflush(stdout);
		}
		else{
			fputs(name_msg,stdout);
			fflush(stdout);
		}
	}
	return NULL;
}
void error_handling(char *msg)
{
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}
void sig_int(int signo){
	printf("\n[Error] : If you want to exit, follow the normal shutdown routine\n");
}