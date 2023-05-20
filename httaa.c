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
#define PORT_NUM 3000
#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"

int main() {
    int listen_fd;
    struct sockaddr_in server_addr;

    SSL_CTX *ssl_ctx;
    SSL *ssl;
    X509 *cert;
    const SSL_METHOD *method;

    char buf[1024];

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
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    server_addr.sin_port = htons(PORT_NUM);

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
        socklen_t client_addr_len;

        // accept connection
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

        // send response to client
        SSL_write(ssl, "Hello, world!", strlen("Hello, world!"));

        // close connection
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(conn_fd);
    }

    // cleanup
    SSL_CTX_free(ssl_ctx);
    return 0;
}
