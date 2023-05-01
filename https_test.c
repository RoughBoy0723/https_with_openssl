#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFF_SIZE 1024
#define SMALL_BUFF 100
#define SERV_SIZE 32

#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"

void* handle_client_request(void* arg);
void send_data(SSL* ssl, char* ct, char* file_name);
char* content_type(char* file);
void send_error(SSL* ssl);
void error_handling(char* message);
void setup_listening_socket(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port);
void handle_client_connection(int serv_sock, struct sockaddr_in clnt_addr);


int main(int argc, char* argv[]) {
	int serv_sock;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;

    SSL_CTX *ssl_ctx; // 구조체 선언
    SSL *ssl; //SSL 선언
   	X509 *cert; //공개키 인증서
    const SSL_METHOD *method;

    // initialize SSL library
   	SSL_library_init();

    // create SSL context
   	method = TLS_server_method();
   	ssl_ctx = SSL_CTX_new(method);

    SSL_CTX_set_ecdh_auto(ssl_ctx, 1);
   	SSL_CTX_use_certificate_file(ssl_ctx, CERT_FILE, SSL_FILETYPE_PEM);
   	SSL_CTX_use_PrivateKey_file(ssl_ctx, KEY_FILE, SSL_FILETYPE_PEM);

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		return 0;
	}

    // create socket
	if ((serv_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		error_handling("socket() error");

	setup_listening_socket(serv_sock, &serv_addr, 20, atoi(argv[1]));

	while (1) {
		handle_client_connection(serv_sock, clnt_addr, SSL_CTX, ssl);
	}
    SSL_CTX_free(ssl_ctx);
	close(serv_sock);
	return 0;
}

/*지정된 포트에 수신 소켓을 설정하고 지정된 백로그 크기의 수신 연결*/
void setup_listening_socket(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port) {
	memset(serv_adr, 0, sizeof(serv_adr);
	serv_adr->sin_family = AF_INET;
	serv_adr->sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr->sin_port = htons(port);

	if (bind(serv_sock, (struct sockaddr*)serv_adr, sizeof(struct sockaddr_in)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, backlog) == -1)
		error_handling("listen() error");
}

/*새로운 클라이언트 연결을 수신하고, 새로운 스레드를 만들어 클라이언트 요청을 처리함*/
void handle_client_connection(int serv_sock, struct sockaddr_in clnt_addr, SSL_CTX* ssl_ctx, SSL* ssl) {
	int clnt_sock;
	pthread_t t_id;
	socklen_t clnt_addr_size;
	clnt_addr_size = sizeof(clnt_addr);
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, (socklen_t*)&clnt_addr_size);
    if (clnt_sock == -1)
		error_handling("accept() error");

    // create SSL object
	ssl = SSL_new(ssl_ctx);
	SSL_set_fd(ssl, clnt_sock);

   	// perform SSL handshake
    if (SSL_accept(ssl) == -1) {
    	perror("SSL_accept");
        SSL_free(ssl);
       	close(clnt_sock);
       	continue;
    }

	printf("Connection Request : %s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
	pthread_create(&t_id, NULL, handle_client_request(ssl), &clnt_sock);
	pthread_detach(t_id);
}

/*http get 요청을 처리함*/
void* handle_client_request(SSL* ssl) {
    char req_line[SMALL_BUFF];
    memset(req_line, 0, sizeof(req_line));
    SSL_read(ssl, req_line, sizeof(breq_lineuf));

	char method[SERV_SIZE];
	char ct[SERV_SIZE];
	char file_name[SERV_SIZE];

	/*HTTP 헤더가 아닌경우 종료*/
	if (strstr(req_line, "HTTP/") == NULL) {
		send_error(ssl);
		SSL_free(ssl);
		return 0;
	}
	strcpy(method, strtok(req_line, " /"));

	/*GET 메소드가 아닌경우 종료*/
	if (strcmp(method, "GET") != 0) {
		send_error(ssl);
		SSL_free(ssl);
		return 0;
	}

	/*파일 전송*/
	strcpy(file_name, strtok(NULL, " /"));
	strcpy(ct, content_type(file_name));
	send_data(ssl, ct, file_name); //입력받은 데이터에 해당하는 파일을 전송함

	fclose(clnt_read);
	return 0;
}

/*http 헤더를 받고 지정된 파일을 전송한다.*/
void send_data(SSl* ssl, char* ct, char* file_name) {
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char server[] = "Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-lenght:2048\r\n";
	char cnt_type[SMALL_BUFF];
	char buf[BUFF_SIZE];
	FILE* send_file;

	sprintf(cnt_type, "Content-type :%s\r\n\r\n", ct);
	send_file = fopen(file_name, "r");
	if (send_file == NULL) {
		send_error(ssl);
		SSL_free(ssl);
		return;
	}
	SSL_write(ssl, protocol, strlen(protocol));
    SSL_write(ssl, server, strlen(server));
    SSL_write(ssl, cnt_len, strlen(cnt_len));
    SSL_write(ssl, cnt_type, strlen(cnt_type));

	while (fgets(buf, BUFF_SIZE, send_file) != NULL) {
		SSL_write(ssl, buf, strlen(buf));
	}
    SSL_shutdown(ssl);
	SSL_free(ssl);
}

/*헤더를 읽어와서 헤더의 파일요청 방식이 html이면 html 반환하고 아니면 text 반환*/
char* content_type(char* file) {
	char extension[SMALL_BUFF];
	char file_name[SMALL_BUFF];

	strcpy(file_name, file);
	strtok(file_name, ".");
	strcpy(extension, strtok(NULL, "."));

	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
		return "text/html";
	else
		return "text/plain";
}

/*에러가 생겼을시 화면 출력*/
void send_error(SSL* ssl) {
	char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
	char server[] = "Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-length:2048\r\n";
	char cnt_type[] = "Content-type:text/html\r\n\r\n";
	char content[] = "<html><head><title>NETWORK</title><head>""<body><font size+=10><br>오류 발생 요청 파일명 및 요청 방식 확인""</font></body></html>";
    
    SSL_write(ssl, protocol, strlen(protocol));
    SSL_write(ssl, server, strlen(server));
    SSL_write(ssl, cnt_len, strlen(cnt_len));
    SSL_write(ssl, cnt_type, strlen(cnt_type));
    SSL_write(ssl, content, strlen(content));
}

/*오류 메시지 출력*/
void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	return;
}