#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<dirent.h>
#include<sys/types.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define MAX_ID 15		//최대 id, password 사이즈
#define MAX_LENGTH 100	//최대 client 수

void *handle_clnt(void *arg);
void error_handling(char *msg);
void server_init();
int login(int clnt_sock,char *clnt_id);

pthread_mutex_t mutx;

char id[MAX_LENGTH][MAX_ID];
char password[MAX_LENGTH][MAX_ID];
int max_user=-1;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	if(argc!=2)
	{
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}
	server_init();
	pthread_mutex_init(&mutx,NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM,0);

	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_adr,sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock,10)==-1)
		error_handling("listen() error");
	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
		pthread_create(&t_id,NULL,handle_clnt,(void*)&clnt_sock);
		pthread_detach(t_id);
	}
	close(serv_sock);
	return 0;
}
void *handle_clnt(void *arg)
{
	int clnt_sock=*((int*)arg);
	char clnt_id[MAX_ID];

	login(clnt_sock, clnt_id);

	close(clnt_sock);
	return NULL;
}
int login(int clnt_sock,char *clnt_id){
	char buf[BUF_SIZE]={0,};
	char send_temp[BUF_SIZE]={0,};
	char temp_id[MAX_ID];
	char temp_password[MAX_ID];
	int read_cnt=0;

	while(1){
		sleep(1);
		write(clnt_sock,"1 : Login \t 2 : Sign-Up\n",sizeof("1 : Login \t 2 : Sign-Up\n"));
		read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt-1]='\0';
		if(strcmp(buf,"1")==0){
			printf("1왔다\n");
			strcpy(send_temp,"ID : ");
			write(clnt_sock,send_temp,strlen(send_temp));
			read_cnt = read(clnt_sock,temp_id,sizeof(temp_id));
			temp_id[read_cnt-1]='\0';
			strcpy(send_temp,"PASSWORD : ");
			write(clnt_sock,send_temp,strlen(send_temp));
			read_cnt = read(clnt_sock,temp_password,sizeof(temp_password));
			temp_password[read_cnt-1]='\0';
			for(int i = 0; i<=max_user;i++){
				if(strcmp(id[i],temp_id)==0){
					if(strcmp(password[i],temp_password)==0){
						sprintf(buf,"Hello! %s\n",temp_id);
						write(clnt_sock,buf,sizeof(buf));
						strcpy(clnt_id,temp_id);
						return 1;
					}
				}
			}
			write(clnt_sock,"ERROR! : Not valid ID or PASSWORD\n",sizeof("ERROR! : Not valid ID or PASSWORD\n"));
		}
		else if(strcmp(buf,"2")==0){

		}
		else{
			write(clnt_sock,"ERROR! : 1 or 2\n",sizeof("ERROR! : 1 or 2\n"));
		}
	}

	return -1;
}
void server_init(){
	DIR *dir_ptr = NULL;
	struct dirent *file = NULL;
	FILE *fp;
	//char root[]="./Server";
	char password_file[MAX_LENGTH]={0,};
	char password_temp[MAX_ID]={0,};

	if((dir_ptr = opendir(".//Server")) == NULL){
		error_handling("Not valid directory.\n");
		exit(1);
	}
	while((file = readdir(dir_ptr)) != NULL){
		if(strcmp(file->d_name,".") != 0 && strcmp(file->d_name,"..") != 0){
			strcpy(id[++max_user],file->d_name);
		}
	}
	for(int i=0;i<=max_user;i++){
		sprintf(password_file,"%s%s%s",".//Server//",id[i],"//password.dat");
		fp=fopen(password_file,"r");
		fgets(password_temp,MAX_ID,fp);
		strcpy(password[i],password_temp);
		fclose(fp);
	}
	closedir(dir_ptr);
}
void error_handling(char *msg)
{
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}
