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

#define IP_ADDR "127.0.0.1"
#define PORT_NUM 5992
#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"
#define MAX 1024
SSL_CTX *ctx;
void * serverThread(void *arg);
void setupServerCtx(void);
void bind_and_listen(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port);
void accept_connection(int serv_sock, struct sockaddr_in clnt_addr);
int serverLoop(SSL * ssl);
void * serverThread(void *arg);
void* handle_client_request(void* arg);
void send_data_ssl(SSL* ssl, char* ct, char* file_name);
char* content_type(char* file);

int main(int argc,char *argv[]) {
    int serv_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    SSL *ssl;
    X509 *cert;

	setupServerCtx();

	//socket
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    bind_and_listen(serv_sock, &serv_addr, 20, atoi(argv[1]));

    while (1) {
		accept_connection(serv_sock, clnt_addr);
	}
	SSL_CTX_free(ctx);
}
void setupServerCtx(void){
    //SSL library memset-> 0
	SSL_library_init();
	const SSL_METHOD *method;
	method = TLS_server_method();
	ctx = SSL_CTX_new(method);
	if(SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) != 1)
		printf("cert err\n");
	if(SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)	!= 1)
		printf("private err\n");
}
void bind_and_listen(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port){
	memset(serv_adr, 0, sizeof(struct sockaddr_in));
	serv_adr->sin_family = AF_INET;
	serv_adr->sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr->sin_port = htons(port);

	if(	(bind(serv_sock, (struct sockaddr*)serv_adr, sizeof(struct sockaddr_in))) == -1)	printf("bind error\n");
    if(	(listen(serv_sock, backlog)) == -1)	printf("listen error\n");
}
void accept_connection(int serv_sock, struct sockaddr_in clnt_addr){
    int clnt_sock;
    SSL *ssl;
    pthread_t tid;
    socklen_t clnt_addr_size;
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, (socklen_t*)&clnt_addr_size);
	ssl = SSL_new(ctx);
	SSL_set_fd(ssl,clnt_sock);
	pthread_create (&tid, NULL, serverThread, ssl);
	pthread_detach(tid);
}
void * serverThread(void *arg){
	SSL *ssl = (SSL *) arg;

	if (SSL_accept(ssl) <= 0){
		printf("hand_shake error\n");
		//SSL_free(ssl);
	}
	fprintf(stderr, "SSL Connection opened\n");
	if(serverLoop(ssl))
		SSL_shutdown(ssl);
	else
		SSL_clear(ssl);

	fprintf(stderr, "SSL Connection closed\n");
	SSL_free(ssl);
}
int serverLoop(SSL * ssl){
	char buf[MAX];
	char http_response[MAX];
	memset(buf, 0, sizeof(buf));
	SSL_read(ssl, buf, sizeof(buf));
	
	// if GET request is for /index.html, send file to client
	if (strncmp(buf, "GET /index.html", 15) == 0) {
		FILE *fp = fopen("index.html", "r");
		if (fp == NULL) {
			SSL_write(ssl, "HTTP/1.1 404 Not Found\r\n\r\n", strlen("HTTP/1.1 404 Not Found\r\n\r\n"));
			SSL_write(ssl, "404 Not Found", strlen("404 Not Found"));
        }
		else {
			fseek(fp, 0, SEEK_END);
			int file_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			fread(http_response, 1, file_size, fp);
			fclose(fp);
			SSL_write(ssl, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"));
            sprintf(http_response, "Content-Length: %d\r\n", file_size);
        	SSL_write(ssl, http_response, strlen(http_response));
            SSL_write(ssl, "Content-Type: text/html\r\n\r\n", strlen("Content-Type: text/html\r\n\r\n"));
            SSL_write(ssl, http_response, file_size);
		}
	}
	else	printf("get error\n");
}
void* handle_client_request(void* arg) {
    SSL *ssl = (SSL *) arg;
    char req_line[MAX];
    char method[MAX];
    char ct[MAX];
    char file_name[MAX];
    char buf[MAX];

    SSL_read(ssl, req_line, MAX);

    /*HTTP 헤더가 아닌경우 종료*/
    if (strstr(req_line, "HTTP/") == NULL) {
        //send_error_ssl(ssl);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        return 0;
    }
    strcpy(method, strtok(req_line, " /"));

    /*GET 메소드가 아닌경우 종료*/
    if (strcmp(method, "GET") != 0) {
        //send_error_ssl(ssl);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        return 0;
    }

    /*파일 전송*/
    strcpy(file_name, strtok(NULL, " /"));
    strcpy(ct, content_type(file_name));
    send_data_ssl(ssl, ct, file_name); //입력받은 데이터에 해당하는 파일을 전송함

    SSL_shutdown(ssl);
    SSL_free(ssl);
    return 0;
}

/*http 헤더를 받고 지정된 파일을 전송한다.*/
void send_data_ssl(SSL* ssl, char* ct, char* file_name) {
    char protocol[] = "HTTP/1.0 200 OK\r\n";
    char server[] = "Server:Linux Web Server\r\n";
    char cnt_len[] = "Content-lenght:2048\r\n";
    char cnt_type[MAX];
	char buf[MAX];
    FILE* send_file;

    sprintf(cnt_type, "Content-type :%s\r\n\r\n", ct);
    send_file = fopen(file_name, "r");
    if (send_file == NULL) {
        //send_error_ssl(ssl);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        return;
    }

    SSL_write(ssl, protocol, strlen(protocol));
    SSL_write(ssl, server, strlen(server));
    SSL_write(ssl, cnt_len, strlen(cnt_len));
    SSL_write(ssl, cnt_type, strlen(cnt_type));

    while (fgets(buf, MAX, send_file) != NULL) {
        SSL_write(ssl, buf, strlen(buf));
    }

    fclose(send_file);
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
