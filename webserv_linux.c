#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100
#define REQ_SIZE 32
void* request_handler(void* arg);
void send_data(FILE* fp, char* ct, char* file_name);
char* content_type(char* file);
void send_error(FILE* fp);
void error_handling(char* message);
void bind_and_listen(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port);
int main(int argc, char *argv[]){
		int serv_sock, clnt_sock;
		struct sockaddr_in serv_adr;
		struct sockaddr_in clnt_adr;
		socklen_t clnt_adr_size;
		char buf[BUF_SIZE];
		pthread_t t_id;
		if(argc!=2){
				printf("Usage : %s <port>\n", argv[0]);
				exit(1);
		}

		serv_sock = socket(PF_INET, SOCK_STREAM, 0);
		if(serv_sock == -1)
        error_handling("socket() error");
        
		bind_and_listen(serv_sock, &serv_adr, 20, atoi(argv[1]));

		while(1){
				clnt_adr_size=sizeof(clnt_adr);
				clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, (socklen_t*)&clnt_adr_size);
				printf("Connection Requeset : %s:%d\n", inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));
				pthread_create(&t_id, NULL, request_handler, &clnt_sock);
				pthread_detach(t_id);
		}
		close(serv_sock);
		return 0;
}

void bind_and_listen(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port)
{
    memset(serv_adr, 0, sizeof(struct sockaddr_in));
    serv_adr->sin_family = AF_INET;
    serv_adr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr->sin_port = htons(port);
    if(bind(serv_sock, (struct sockaddr*)serv_adr, sizeof(struct sockaddr_in)) == -1)
        error_handling("bind() error");
    if(listen(serv_sock, backlog) == -1)
        error_handling("listen() error");
}
void* request_handler(void *arg)
{
		int clnt_sock = *((int*)arg);
		char req_line[SMALL_BUF];
		FILE* clnt_read;
		FILE* clnt_write;

		char method[REQ_SIZE];
		char ct[REQ_SIZE];
		char file_name[REQ_SIZE];

		clnt_read = fdopen(clnt_sock, "r");
		clnt_write = fdopen(dup(clnt_sock), "w");
		fgets(req_line, SMALL_BUF, clnt_read);
		
		if(strstr(req_line, "HTTP/")==NULL)
        {
                send_error(clnt_write);
                fclose(clnt_read);
                fclose(clnt_write);
                return 0; 
        }

        strcpy(method, strtok(req_line, " /"));
        strcpy(file_name, strtok(NULL, " /"));
		strcpy(ct, content_type(file_name));

		if(strcmp(method, "GET")!=0)
        {
                send_error(clnt_write);
                fclose(clnt_read);
                fclose(clnt_write);
                return 0;
        }

		fclose(clnt_read);
		send_data(clnt_write, ct, file_name);
		return 0;
}

void send_data(FILE* fp, char* ct, char* file_name)
{
		char protocol[] = "HTTP/2.0 200 OK\r\n";
		char server[] = "Server:Linux Web Server \r\n";
		char cnt_len[] = "Content-length:2048\r\n";
		char cnt_type[SMALL_BUF];
		char buf[BUF_SIZE];
		FILE* send_file;

		sprintf(cnt_type, "Content-type:%s\r\n\r\n", ct);
		send_file=fopen(file_name, "r");
		if(send_file == NULL)
		{
				send_error(fp);
				return;
		}

		fputs(protocol, fp);
		fputs(server, fp);
		fputs(cnt_len, fp);
		fputs(cnt_type, fp);
		
		while(fgets(buf,BUF_SIZE, send_file) != NULL)
		{
				fputs(buf, fp);
				fflush(fp);
		}

		fflush(fp);
		fclose(fp);
}

char* content_type(char* file)
{
		char extension[SMALL_BUF];
		char file_name[SMALL_BUF];
		strcpy(file_name, file);
		strtok(file_name, ",");
		strcpy(extension, strtok(NULL, "."));

		if(!strcmp(extension, "html")||!strcmp(extension, "htm"))
				return "text/html";
		else
				return "text/plain";
}

void send_error(FILE* fp)
{
		char protocol[] = "HTTP/2.0 400 Bad Request\r\n";
		char server[] = "Server:Linux Web Server \r\n";
		char cnt_len[] = "Content-length:2048\r\n";
		char cnt_type[] = "Content-type:text/html\r\n\r\n";
		char content[] = "<html><head><title>NETWORK</title></head>"
				"<body><font size=+5><br>WORK OUT! Please Check Your Acces Root!"
				"</font></body></html>";

		fputs(protocol, fp);
		fputs(server, fp);
		fputs(cnt_len, fp);
		fputs(cnt_type, fp);
		fflush(fp);
}

void error_handling(char* message)
{
		fputs(message,stderr);
		fputc('\n', stderr);
		exit(1);
}
