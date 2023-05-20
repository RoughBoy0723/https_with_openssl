#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#define PORT    "12345"
#define SERVER  "127.0.0.1" // 서버 ip 주소
#define CLIENT  "127.0.0.1" // 클라이언트 ip 주소

#define CLI_CERT "cert.pem"
#define CLI_PRIV "key.pem"
#define SRV_CERT "cert.pem"
#define SRV_PRIV "key.pem"

void printErr(const char *msg);
