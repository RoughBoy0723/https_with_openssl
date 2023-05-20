#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 443
#define MAX_BUF_SIZE 1024

int main() {
    SSL_library_init();
    SSL_load_error_strings();

    SSL_CTX* ctx = SSL_CTX_new(TLSv1_2_server_method());
    if (!ctx) {
        printf("Failed to create SSL context\n");
        return -1;
    }

    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        printf("Failed to load server certificate\n");
        SSL_CTX_free(ctx);
        return -1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0 ) {
        printf("Failed to load server private key\n");
        SSL_CTX_free(ctx);
        return -1;
    }

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Failed to create socket: %s\n", strerror(errno));
        SSL_CTX_free(ctx);
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Failed to bind socket: %s\n", strerror(errno));
        SSL_CTX_free(ctx);
        return -1;
    }

    if (listen(server_fd, 10) == -1) {
        printf("Failed to listen to socket: %s\n", strerror(errno));
        SSL_CTX_free(ctx);
        return -1;
    }

    while (1) {
        printf("Waiting for client connection...\n");

        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            printf("Failed to accept client connection: %s\n", strerror(errno));
            SSL_CTX_free(ctx);
            close(server_fd);
            return -1;
        }

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0) {
            printf("SSL accept failed\n");
            SSL_CTX_free(ctx);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        printf("SSL connection established\n");

        char buffer[MAX_BUF_SIZE] = {0};
        int bytes_read = SSL_read(ssl, buffer, MAX_BUF_SIZE);
        if (bytes_read <= 0) {
            printf("Failed to read data from client\n");
            SSL_CTX_free(ctx);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        printf("Received message from client: %s\n", buffer);

        char* message = "Hello, client!";
        int bytes_written = SSL_write(ssl, message, strlen(message));
        if (bytes_written <= 0) {
        (ssl);
        close(new_sockfd);
        continue;
    }

    // SSL 소켓 연결 종료
    SSL_shutdown(ssl);
    SSL_free(ssl);

    // 클라이언트 소켓 종료
    close(new_sockfd);
	}

// SSL 컨텍스트 해제
SSL_CTX_free(ctx);

// 소켓 종료
close(sock_fd);

return 0;
}

