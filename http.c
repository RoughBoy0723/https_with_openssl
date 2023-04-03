#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFF_SIZE 1024
#define SMALL_BUFF 100
#define SERV_SIZE 32
void *request_handler(void*arg);
void send_data(FILE*fp,char * ct,char* file_name);
char* content_type(char* file);
void send_error(FILE *fp);
void error_handling(char *message);
void bind_and_listen(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port);
void accept_connection(int serv_sock, struct sockaddr_in clnt_addr,int clnt_sock,socklen_t clnt_addr_size);
int error_message(int argc,char *argv[]);
int main(int argc, char *argv[]){
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;
	error_message(argc,argv);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	bind_and_listen(serv_sock, &serv_addr, 20, atoi(argv[1]));
	accept_connection(serv_sock, clnt_addr,clnt_sock,clnt_addr_size);
	return 0;
}
int error_message(int argc,char *argv[]){
	if(argc!=2){
                 printf("Usage : %s <port>\n", argv[0]);
                 return 0;
         }
}
void bind_and_listen(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port){	
	memset(serv_adr, 0, sizeof(struct sockaddr_in));
	serv_adr->sin_family = AF_INET;
	serv_adr->sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr->sin_port = htons(port);
   	if(bind(serv_sock, (struct sockaddr*)serv_adr, sizeof(struct sockaddr_in)) == -1)
    		error_handling("bind() error");
    	if(listen(serv_sock, backlog) == -1)
    		error_handling("listen() error");
}
void accept_connection(int serv_sock, struct sockaddr_in clnt_addr,int clnt_sock,socklen_t clnt_addr_size){
	pthread_t t_id;
	while(1){
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, (socklen_t*)&clnt_addr_size);
		printf("Connection Request : %s:%d\n",inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
		pthread_create(&t_id, NULL, request_handler, &clnt_sock);
		pthread_detach(t_id);
	}
	close(serv_sock);
}
void *request_handler(void *arg){
	int clnt_sock = *((int*)arg);  
	char req_line[SMALL_BUFF];
	FILE* clnt_read;
	FILE* clnt_write;
	char method[SERV_SIZE];
	char ct[SERV_SIZE];
	char file_name[SERV_SIZE];

	clnt_read = fdopen(clnt_sock, "r"); 
	clnt_write = fdopen(dup(clnt_sock), "w");
	fgets(req_line, SMALL_BUFF, clnt_read); 
	
	if(strstr(req_line, "HTTP/")==NULL){
		send_error(clnt_write); 
		fclose(clnt_read);
		fclose(clnt_write);
		return 0;
	}
	strcpy(method, strtok(req_line, " /"));
	if(strcmp(method, "GET")!=0){
		send_error(clnt_write); 
		fclose(clnt_read); 
		fclose(clnt_write);  
		return 0;
	}
	strcpy(file_name, strtok(NULL, " /"));  
	strcpy(ct, content_type(file_name));  
	fclose(clnt_read);  
	send_data(clnt_write, ct, file_name); 

	return 0;
}
void send_data(FILE * fp, char* ct, char*file_name){
	char protocol[] = "HTTP/1.0 200 OK\r\n"; 
	char server[] = "Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-lenght:2048\r\n";
	char cnt_type[SMALL_BUFF]; 
	char buf[BUFF_SIZE];
	FILE* send_file;  

	sprintf(cnt_type, "Content-type :%s\r\n\r\n", ct);  
	send_file = fopen(file_name, "r"); 
	if(send_file == NULL){
		send_error(fp); 
		fclose(fp);
		return;
	}
	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len,fp);
	fputs(cnt_type, fp);

	while(fgets(buf, BUFF_SIZE, send_file) != NULL){
		fputs(buf,fp);
		fflush(fp);
	}
	fflush(fp); 
	fclose(fp); 
}
char *content_type(char* file){
	char extension[SMALL_BUFF];
	char file_name[SMALL_BUFF];

	strcpy(file_name, file);
	strtok(file_name, ".");
	strcpy(extension, strtok(NULL, "."));

	return "text/plain";
}
void send_error(FILE *fp){
	char protocol[]="HTTP/1.0 400 Bad Request\r\n";
	char server[]="Server:Linux Web Server\r\n";
	char cnt_len[]="Content-length:2048\r\n";
	char cnt_type[]="Content-type:text/html\r\n\r\n";
	char content[]= "<html><head><title>NETWORK</title><head>""<body><font size+=10><br>오류 발생 요청 파일명 및 요청 방식 확인""</font></body></html>";

	fputs(protocol,fp);
	fputs(server,fp);
	fputs(cnt_len,fp);
	fputs(cnt_type,fp);
	fflush(fp);
}
void error_handling(char *message){
	fputs(message,stderr);
	fputc('\n', stderr);
	return ;
}
