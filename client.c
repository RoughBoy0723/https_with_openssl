// client.c
#include "common.h"

// callback 함수
void cbFunction(const SSL *ssl, const char *line) {
	const char *keylog_file = getenv("SSLKEYLOGFILE");
	if (keylog_file) {
		BIO *keylog_bio = BIO_new_file(keylog_file, "a");
		if (!keylog_bio) {
				printErr ("cbFunction for keylog: BIO_new_file() error");
				return ;
		}
		BIO_printf(keylog_bio, "%s\n", line);
		BIO_flush(keylog_bio);
	}
}

SSL_CTX *setupClientCtx(void) {
	SSL_CTX *ctx;

	ctx = SSL_CTX_new(TLS_client_method());
	if (SSL_CTX_use_certificate_chain_file(ctx, CLI_CERT) != 1)
		printErr("SSL_CTX_use_certificate_chain_file() error");
	
	if (SSL_CTX_use_PrivateKey_file(ctx, CLI_PRIV, SSL_FILETYPE_PEM) != 1)
		printErr("SSL_CTX_use_PrivateKey_file() error");
	
	// for wireshark captured TLS packet analysis
	SSL_CTX_set_keylog_callback(ctx, cbFunction);

	return ctx;
}

int clientLoop(SSL * ssl) {
	int err, nwritten;
	char buf[80];

	for (;;) {
		if (!fgets(buf, sizeof(buf), stdin))
			break;
		for (nwritten = 0; nwritten < sizeof(buf); nwritten += err) {
			err = SSL_write(ssl, buf + nwritten, strlen(buf) - nwritten);
			if (err <= 0)
				return 0;
		}
		printf ("write [%d] bytes.\n", nwritten);
	}
	//return 1;
	return (SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) ? 1 : 0;
}

int main(int argc, char *argv[]) {
	BIO *conn;
	SSL *ssl;
	SSL_CTX *ctx;

	ctx = setupClientCtx();
	conn = BIO_new_connect(SERVER ":" PORT);
	if (!conn)
			printErr("BIO_new_connect() error");

	if (BIO_do_connect(conn) <= 0)
			printErr("BIO_do_connect() error");

	if (!(ssl = SSL_new(ctx)))
		printErr("SSL_new() error");

	// void SSL_set_bio(SSL *ssl, BIO *rbio, BIO *wbio);
	SSL_set_bio(ssl, conn, conn);		// for read, write

	if (SSL_connect(ssl) <= 0)
		printErr("SSL_connect() error");
	fprintf(stderr, "SSL connection opened\n");

	if (clientLoop(ssl))
		SSL_shutdown(ssl);
	else
		SSL_clear(ssl);
	fprintf(stderr, "SSL connection closed\n");

	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}


