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

#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"
#define MAX 1024
#define domain "min.hanb.at"

//#define CERT_FILE "cert1.pem"
//#define KEY_FILE "privkey1.pem"
SSL_CTX* setupServerCtx(void);
void * serverThread(void *arg);
void* handle_client_request(SSL* ssl);
void send_data_ssl(SSL* ssl, char* ct, char* file_name);
char* content_type(char* file);
void send_error(SSL* ssl);
void error_handling(char* message);

int main(int argc,char *argv[]) {
	SSL_CTX *ctx;
	SSL *ssl;
	pthread_t tid;
	BIO *acc,*client;
	char *port;
	//SSL_CTX SET
	ctx=setupServerCtx();
	//PORT NUM SET
	port=(char *)malloc(sizeof(char)*10);
	strcpy(port,argv[1]);
	//BIO set port
	if (!(acc = BIO_new_accept(port)))
		error_handling("BIO_new_accept() error");
	//first do_accept > socket create, bind
	if (BIO_do_accept(acc) <= 0)
		error_handling("1st BIO_do_accept() error");

	for (;;) {
		//second do_accept > wait client
		if (BIO_do_accept(acc) <= 0)
			error_handling("2nd BIO_do_accept() error");
		//latest client pop
		client = BIO_pop(acc);
		if (!(ssl = SSL_new(ctx)))
			error_handling("SSL_new() error");
		//client input/output set
		SSL_set_bio(ssl, client, client);
		pthread_create (&tid, NULL, serverThread, ssl);
	}
	SSL_CTX_free(ctx);
	BIO_free(acc);
}
SSL_CTX* setupServerCtx(void){
	SSL_CTX *ctx;
    //SSL library memset-> 0
	SSL_library_init();
	
	ctx = SSL_CTX_new(TLS_server_method());

	if(SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) != 1)
		error_handling("cert err");

	if(SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)	!= 1)
		error_handling("private err");
	return ctx;
}
void * serverThread(void *arg){
	SSL *ssl = (SSL *) arg;

	pthread_detach(pthread_self());
	//Server-Client Hand-Shake
	if (SSL_accept(ssl) <= 0){
		error_handling("Hand-Shake err");
		return 0;
	}
	fprintf(stderr, "SSL Connection opened\n");
	handle_client_request(ssl);
	fprintf(stderr, "SSL Connection closed\n");
}
void* handle_client_request(SSL *ssl) {
	char req_line[MAX];
	char method[MAX];
	char ct[MAX];
	char file_name[MAX];
	char buf[MAX];
	// treats an unexpected EOF from the peer as if normally shutdown
	SSL_set_options(ssl, SSL_OP_IGNORE_UNEXPECTED_EOF);

	SSL_read(ssl, req_line, MAX);	
    //not HTTP header close
	if (strstr(req_line, "HTTP/")== NULL) {
		send_error(ssl);
		SSL_shutdown(ssl);
		SSL_free(ssl);
		return 0;
	}
	strcpy(method, strtok(req_line, " /"));

    //not GET method close
	if (strcmp(method, "GET") != 0) {
		send_error(ssl);
		SSL_shutdown(ssl);
		SSL_free(ssl);
		return 0;
	}

    //file communication
	strcpy(file_name, strtok(NULL, " /"));
	strcpy(ct, content_type(file_name));
	//data type check
	send_data_ssl(ssl, ct, file_name);
}

//data transmit
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
		send_error(ssl);
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
	SSL_shutdown(ssl);
	SSL_free(ssl);
}
//file header check-> html or plain
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
void send_error(SSL* ssl) {
	char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
	char server[] = "Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-length:2048\r\n";
	char cnt_type[] = "Content-type:text/html\r\n\r\n";
	char content[] = "<html><head><title>NETWORK</title><head>""<body><font size+=10><br>error occur""</font></body></html>";
	SSL_write(ssl, protocol, strlen(protocol));
	SSL_write(ssl, server, strlen(server));
	SSL_write(ssl, cnt_len, strlen(cnt_len));
	SSL_write(ssl, cnt_type, strlen(cnt_type));
	SSL_write(ssl, content, strlen(content));
}
void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	return ;
}
