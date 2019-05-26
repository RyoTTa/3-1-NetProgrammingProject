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
#include<sys/stat.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define MAX_ID 15		//최대 id, password 사이즈
#define MAX_LENGTH 100	//최대 client 수

void *handle_clnt(void *arg);
void error_handling(char *msg);
void server_init();		//서버 시작시 초기화

int login(int clnt_sock,char *clnt_id);	//로그인
int sign_up(int clnt_sock,char *clnt_id);	//회원가입
int mail_function(int clnt_sock,char *clnt_id); //메일 기능 메뉴
int mail_open(int clnt_sock,char *clnt_id, char mail_lit[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]);	//메일 출력

void sys_write(char *buf,int clnt_sock);	//[SYSTEM] 출력
void error_write(char *buf,int clnt_sock);	//[ERROR] 출력
void opt_write(char *buf,int clnt_sock);	//clnt opt 출력


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

	if(!login(clnt_sock, clnt_id)){
		error_handling("[SERVER] Login ERROR");
	}
	while(1){
		mail_function(clnt_sock, clnt_id);
	}


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
		opt_write("clear",clnt_sock);
		sleep(1);
		sys_write("1 : Login \t 2 : Sign-Up\n[COMMAND] : ",clnt_sock);
		read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt-1]='\0';
		if(strcmp(buf,"1")==0){
			sys_write("ID : ",clnt_sock);
			read_cnt = read(clnt_sock,temp_id,sizeof(temp_id));
			temp_id[read_cnt-1]='\0';
			sys_write("PASSWORD : ",clnt_sock);
			read_cnt = read(clnt_sock,temp_password,sizeof(temp_password));
			temp_password[read_cnt-1]='\0';
			for(int i = 0; i<=max_user;i++){
				if(strcmp(id[i],temp_id)==0){
					if(strcmp(password[i],temp_password)==0){
						sprintf(buf,"Hello! %s\n",temp_id);
						sys_write(buf,clnt_sock);
						strcpy(clnt_id,temp_id);
						return 1;
					}
				}
			}
			error_write("Not valid ID or PASSWORD\n",clnt_sock);
		}
		else if(strcmp(buf,"2")==0){
			sign_up(clnt_sock, clnt_id);
		}
		else{
			error_write("1 or 2\n",clnt_sock);
		}
	}

	return -1;
}

int sign_up(int clnt_sock,char *clnt_id){
	char buf[BUF_SIZE];
	char temp_id[BUF_SIZE]={0,};
	char temp_password[BUF_SIZE]={0,};
	int read_cnt=0;
	FILE *fp;

	sleep(1);
	opt_write("clear",clnt_sock);
	sleep(1);
	sys_write("Sign-Up\t ID and PASSWORD must be less than 14 alphabet\n",clnt_sock);
	sys_write("ID : ",clnt_sock);
	read_cnt = read(clnt_sock,temp_id,sizeof(temp_id));
	temp_id[read_cnt-1]='\0';
	sys_write("PASSWORD : ",clnt_sock);
	read_cnt = read(clnt_sock,temp_password,sizeof(temp_password));
	temp_password[read_cnt-1]='\0';
	if(strlen(temp_id) > 14 || strlen(temp_password) > 14){
		error_write("ID and PASSWORD must be less than 14 alphabet\n",clnt_sock);
		return -1;
	}
	while(1){
		sprintf(buf,"Check ID : %s,  PASSWORD : %s (Y/N)? ",temp_id,temp_password);
		sys_write(buf,clnt_sock);
		read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt-1]='\0';
		if(strcmp(buf,"Y")==0){
			pthread_mutex_lock(&mutx);
			for(int i=0;i<=max_user;i++){
				if(strcmp(temp_id,id[i])==0){
					sprintf(buf,"ID : %s is overlaped\n",temp_id);
					sys_write(buf,clnt_sock);
					pthread_mutex_unlock(&mutx);
					return -1;
				}
			}
			sprintf(buf,"%s%s",".//Server//",temp_id);
			mkdir(buf,0777);
			strcpy(id[++max_user],temp_id);
			strcpy(password[max_user],temp_password);
			sprintf(buf,"%s%s%s",".//Server//",temp_id,"//password.dat");
			fp=fopen(buf,"w+");
			fputs(temp_password,fp);
			fclose(fp);
			pthread_mutex_unlock(&mutx);
			return 1;
		}
		else if(strcmp(buf,"N")==0){
			return -1;
		}
		else{
			error_write("Y or N\n",clnt_sock);
		}
	}
	return 1;
}

int mail_function(int clnt_sock,char *clnt_id){
	char buf[BUF_SIZE];
	int read_cnt=0;
	FILE *fp;
	char mail_folder[MAX_LENGTH][MAX_LENGTH];
	char mail_list[MAX_LENGTH][3][MAX_LENGTH]={0,};
	int mail_count = -1;
	
	sleep(1);
	opt_write("clear",clnt_sock);
	sleep(1);
	sys_write("1 : Mail Open \t 2 : Mail Send\n[COMMAND] : ",clnt_sock);
	read_cnt = read(clnt_sock,buf,sizeof(buf));
	buf[read_cnt-1]='\0';

	if(strcmp(buf,"1")==0){
		mail_open(clnt_sock,clnt_id, mail_list,&mail_count,mail_folder);
	}
	else if(strcmp(buf,"2")==0){
		
	}
	else if(strcmp(buf,"3")==0){
		
	}
	else{
		error_write("1, 2 or 3\n",clnt_sock);
	}
}

int mail_open(int clnt_sock,char *clnt_id, char mail_lit[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]){	//메일 출력
	DIR *dir_ptr = NULL;
	struct dirent *file = NULL;
	FILE *fp;
	char file_temp[MAX_LENGTH]={0,};
	int i=0;

	sprintf(file_temp,"%s%s",".//Server//",clnt_id);
	if((dir_ptr = opendir(file_temp)) == NULL){
		error_handling("Not valid directory.\n");
		exit(1);
	}
	while((file = readdir(dir_ptr)) != NULL){
		if(strcmp(file->d_name,".") != 0 && strcmp(file->d_name,"..") !=0 && strcmp(file->d_name,"password.dat") != 0){
			strcpy(mail_folder[++(*mail_count)],file->d_name);
		}
	}
	for(i=0;i<=*mail_count;i++){
		printf("%s\n",mail_folder[i]);
	}
	

}

void sys_write(char *buf,int clnt_sock){
	char temp[BUF_SIZE]={0,};

	sprintf(temp,"%-10s %s","[SYSTEM]",buf);
	write(clnt_sock,temp,strlen(temp));
}

void error_write(char *buf,int clnt_sock){
	char temp[BUF_SIZE]={0,};

	sprintf(temp,"%-10s %s","[ERROR]",buf);
	write(clnt_sock,temp,strlen(temp));
}

void opt_write(char *buf,int clnt_sock){
	char temp[BUF_SIZE]={0,};

	sprintf(temp,"%s%s","*#205*#",buf);
	write(clnt_sock,temp,strlen(temp));
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

