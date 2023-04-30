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

int main(int argc, char *argv[]){
	int listen_fd;
	struct sockaddr_in server_addr;
	
	SSL_CTX *ssl_ctx;
    	SSL *ssl;
   	X509 *cert;
    	const SSL_METHOD *method;

    	char buf[1024];
    	char http_response[1024];

    	// initialize SSL library
    	SSL_library_init();

    	// create SSL context
    	method = TLS_server_method();
    	ssl_ctx = SSL_CTX_new(method);

    	// set SSL options
    	SSL_CTX_set_ecdh_auto(ssl_ctx, 1);
    	SSL_CTX_use_certificate_file(ssl_ctx, CERT_FILE, SSL_FILETYPE_PEM);
    	SSL_CTX_use_PrivateKey_file(ssl_ctx, KEY_FILE, SSL_FILETYPE_PEM);

    	// create socket
    	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    	if (listen_fd == -1) {
        	perror("socket");
        	return 1;
    	}

    	// set server address
    	memset(&server_addr, 0, sizeof(server_addr));
    	server_addr.sin_family = AF_INET;
    	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    	server_addr.sin_port = htons(atoi(argv[1]));

    	// bind socket
    	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        	perror("bind");
        	return 1;
    	}

    	// listen on socket
    	if (listen(listen_fd, 10) == -1) {
        	perror("listen");
        	return 1;
   	}

	while (1) {
        	int conn_fd;
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

        	conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
       		if (conn_fd == -1) {
            		perror("accept");
			continue;
       		}
        	// create SSL object
		ssl = SSL_new(ssl_ctx);
		SSL_set_fd(ssl, conn_fd);

        	// perform SSL handshake
        	if (SSL_accept(ssl) == -1) {
            		perror("SSL_accept");
            		SSL_free(ssl);
            		close(conn_fd);
            		continue;
        	}
        	// read data from client
       		memset(buf, 0, sizeof(buf));
        	SSL_read(ssl, buf, sizeof(buf));

        	// if GET request is for /index.html, send file to client
       		if (strncmp(buf, "GET /index.html", 15) == 0) {
            		FILE *fp = fopen("index.html", "r");
			if (fp == NULL) {
                		SSL_write(ssl, "HTTP/1.1 404 Not Found\r\n\r\n", strlen("HTTP/1.1 404 Not Found\r\n\r\n"));
                		SSL_write(ssl, "404 Not Found", strlen("404 Not Found"));
            		}
			else{
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
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(conn_fd);
	}
	SSL_CTX_free(ssl_ctx);
	return 0;
}
