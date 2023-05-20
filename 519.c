#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define ip "203.230.102.41"
#define CERT_FILE "cert1.pem"
#define KEY_FILE "privkey1.pem"
#define CHAIN_FILE "fullchain1.pem"
#define Open_FILE "Index.html"
#define MAX 1024

int clnt_sock;
SSL_CTX* setupServerCtx(void);
void bind_and_listen(int *serv_sock, struct sockaddr_in* serv_adr, int backlog, int port);
void accept_connection(int serv_sock,SSL_CTX* ctx);
void * serverThread(void *arg);
int handle_client_request(SSL* ssl);
void send_data_ssl(SSL* ssl, char* ct, char* file_name);
char* content_type(char* file);

int main(int argc,char *argv[]) {
	int serv_sock;
	SSL_CTX *ctx;
	struct sockaddr_in serv_addr;

	ctx=setupServerCtx();

	//socket
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	bind_and_listen(&serv_sock, &serv_addr, 1, 443);
	for(;;){
		accept_connection(serv_sock,ctx);
		//sleep(100);
	}

	SSL_CTX_free(ctx);
	return 0;
}
SSL_CTX* setupServerCtx(void){
	SSL_CTX *ctx;
    //SSL library memset-> 0
	SSL_library_init();

	ctx = SSL_CTX_new(TLS_server_method());
	if(SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) != 1)
		printf("cert err\n");
	if(SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)	!= 1)
		printf("private err\n");
	if(SSL_CTX_use_certificate_chain_file(ctx, CHAIN_FILE) != 1)
		printf("chain err\n");
	return ctx;
}
void bind_and_listen(int *serv_sock, struct sockaddr_in* serv_adr, int backlog, int port){
	memset(serv_adr, 0, sizeof(struct sockaddr_in));
	serv_adr->sin_family = AF_INET;
	serv_adr->sin_addr.s_addr = inet_addr(ip);
	serv_adr->sin_port = htons(port);

	if(	(bind(*serv_sock, (struct sockaddr*)serv_adr, sizeof(struct sockaddr_in))) == -1)	printf("bind error\n");
	if(	(listen(*serv_sock, backlog)) == -1)	printf("listen error\n");
}
void accept_connection(int serv_sock,SSL_CTX *ctx){
	SSL *ssl;

	struct sockaddr_in clnt_addr;
	pthread_t tid;
	socklen_t clnt_addr_size;
	clnt_addr_size = sizeof(clnt_addr);
	if(!(clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, (socklen_t*)&clnt_addr_size)))
		return ;
	ssl = SSL_new(ctx);
	SSL_set_fd(ssl,clnt_sock);
	pthread_create (&tid, NULL, serverThread, ssl);
}
void * serverThread(void *arg){
	SSL *ssl = (SSL *) arg;

	pthread_detach(pthread_self());

	fprintf(stderr, "SSL Connection opened\n");
	handle_client_request(ssl);
	fprintf(stderr, "SSL Connection closed\n");
	SSL_shutdown(ssl);
	SSL_free(ssl);
}
int handle_client_request(SSL *ssl) {
	char req_line[MAX];
	char method[MAX];
	char ct[MAX];
	char file_name[MAX];
	char buf[MAX];

	if (SSL_accept(ssl) <= 0){
		printf("hand_shake error\n");
		return 0;
	}

	SSL_set_options(ssl, SSL_OP_IGNORE_UNEXPECTED_EOF);

	memset(req_line, 0, sizeof(req_line));
	SSL_read(ssl, req_line, sizeof(req_line));	
    /*HTTP 헤더가 아닌경우 종료*/
	if (strstr(req_line, "HTTP/") == NULL) {
		printf("NOT HTTPS\n");
		return 0;
	}
	strcpy(method, strtok(req_line, " /"));

    /*GET 메소드가 아닌경우 종료*/
	if (strcmp(method, "GET") != 0) {
		printf("NOT GET\n");
		return 0;
	}

    /*파일 전송*/
	strcpy(file_name, "Index.html");
	strcpy(ct, content_type(file_name));
	send_data_ssl(ssl, ct, file_name); //입력받은 데이터에 해당하는 파일을 전송함
	return 1;
}

/*http 헤더를 받고 지정된 파일을 전송한다.*/
void send_data_ssl(SSL* ssl, char* ct, char* file_name) {
	char protocol[] = "HTTP/1.1 200 OK\r\n";
	char server[] = "Server:Linux Web Server\r\n";
	char cnt_type[MAX];
	FILE* send_file;

	sprintf(cnt_type, "Content-type: %s\r\n", ct);
	send_file = fopen(file_name, "r");
	if (send_file == NULL) {
		printf("send file=NULL\n");
		return ;
    }

	fseek(send_file, 0, SEEK_END);
	long file_size = ftell(send_file);
	fseek(send_file, 0, SEEK_SET);
	char buf[file_size+MAX];
	char cnt_len[MAX];

	sprintf(cnt_len, "Content-Length: %ld\r\n", file_size);

	SSL_write(ssl, protocol, strlen(protocol));
	SSL_write(ssl, cnt_len, strlen(cnt_len));
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	SSL_write(ssl, server, strlen(server));
	strcpy(cnt_type, "Content-type: application/javascript\r\n");
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	strcpy(cnt_type, "Content-type: application/wasm\r\n");
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	strcpy(cnt_type, "Content-type: application/octet-stream\r\n");
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	strcpy(cnt_type, "Content-type: image/png\r\n");
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	strcpy(cnt_type, "Content-type: text/css\r\n");
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	strcpy(cnt_type, "Content-encoding: gzip\r\n\r\n");
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	
	fread(buf, 1, file_size, send_file);
    SSL_write(ssl, buf, file_size);
	fclose(send_file);
	close(clnt_sock);
}

/*헤더를 읽어와서 헤더의 파일요청 방식이 html이면 html 반환하고 아니면 text 반환*/
char* content_type(char* file) {
	char extension[MAX];
	char file_name[MAX];

	strcpy(file_name, file);
	strtok(file_name, ".");
	strcpy(extension, strtok(NULL, "."));
	
	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
		return "text/html";
	else
		return "text/plain";
}
