#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>


#define BUFF_SIZE 1024
#define SMALL_BUFF 100
#define SERV_SIZE 32


void* handle_client_request(void* arg);
void send_data(FILE* fp, char* ct, char* file_name);
char* content_type(char* file);
void send_error(FILE* fp);
void error_handling(char* message);
void setup_listening_socket(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port);
void handle_client_connection(int serv_sock, struct sockaddr_in clnt_addr);


int main(int argc, char* argv[]) {
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		return 0;
	}

	if ((serv_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		error_handling("socket() error");
	setup_listening_socket(serv_sock, &serv_addr, 20, atoi(argv[1]));

	while (1) {
		handle_client_connection(serv_sock, clnt_addr);
	}
	close(serv_sock);
	return 0;
}

/*������ ��Ʈ�� ���� ������ �����ϰ� ������ ��α� ũ���� ���� ����*/
void setup_listening_socket(int serv_sock, struct sockaddr_in* serv_adr, int backlog, int port) {
	memset(serv_adr, 0, sizeof(struct sockaddr_in));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(port);

	if (bind(serv_sock, (struct sockaddr*)serv_adr, sizeof(struct sockaddr_in)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, backlog) == -1)
		error_handling("listen() error");
}

/*���ο� Ŭ���̾�Ʈ ������ �����ϰ�, ���ο� �����带 ����� Ŭ���̾�Ʈ ��û�� ó����*/
void handle_client_connection(int serv_sock, struct sockaddr_in clnt_addr) {
	int clnt_sock;
	pthread_t t_id;
	socklen_t clnt_addr_size;
	clnt_addr_size = sizeof(clnt_addr);
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, (socklen_t*)&clnt_addr_size);

	printf("Connection Request : %s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
	pthread_create(&t_id, NULL, handle_client_request, &clnt_sock);
	pthread_detach(t_id);
}

/*http get ��û�� ó����*/
void* handle_client_request(void* arg) {
	int clnt_sock = *((int*)arg);
	char req_line[SMALL_BUFF];
	char method[SERV_SIZE];
	char ct[SERV_SIZE];
	char file_name[SERV_SIZE];

	FILE* clnt_read = fdopen(clnt_sock, "r");
	FILE* clnt_write = fdopen(dup(clnt_sock), "w");
	fgets(req_line, SMALL_BUFF, clnt_read);

	/*HTTP ����� �ƴѰ�� ����*/
	if (strstr(req_line, "HTTP/") == NULL) {
		send_error(clnt_write);
		fclose(clnt_read);
		fclose(clnt_write);
		return 0;
	}
	strcpy(method, strtok(req_line, " /"));

	/*GET �޼ҵ尡 �ƴѰ�� ����*/
	if (strcmp(method, "GET") != 0) {
		send_error(clnt_write);
		fclose(clnt_read);
		fclose(clnt_write);
		return 0;
	}

	/*���� ����*/
	strcpy(file_name, strtok(NULL, " /"));
	strcpy(ct, content_type(file_name));
	send_data(clnt_write, ct, file_name); //�Է¹��� �����Ϳ� �ش��ϴ� ������ ������

	fclose(clnt_read);
	return 0;
}
/*http ����� �ް� ������ ������ �����Ѵ�.*/
void send_data(FILE* fp, char* ct, char* file_name) {
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char server[] = "Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-lenght:2048\r\n";
	char cnt_type[SMALL_BUFF];
	char buf[BUFF_SIZE];
	FILE* send_file;

	sprintf(cnt_type, "Content-type :%s\r\n\r\n", ct);
	send_file = fopen(file_name, "r");
	if (send_file == NULL) {
		send_error(fp);
		fclose(fp);
		return;
	}
	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);

	while (fgets(buf, BUFF_SIZE, send_file) != NULL) {
		fputs(buf, fp);
		fflush(fp);
	}
	fflush(fp);
	fclose(fp);
}

/*����� �о�ͼ� ����� ���Ͽ�û ����� html�̸� html ��ȯ�ϰ� �ƴϸ� text ��ȯ*/
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

/*������ �������� ȭ�� ���*/
void send_error(FILE* fp) {
	char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
	char server[] = "Server:Linux Web Server\r\n";
	char cnt_len[] = "Content-length:2048\r\n";
	char cnt_type[] = "Content-type:text/html\r\n\r\n";
	char content[] = "<html><head><title>NETWORK</title><head>""<body><font size+=10><br>���� �߻� ��û ���ϸ� �� ��û ��� Ȯ��""</font></body></html>";

	fputs(protocol, fp);
	fputs(server, fp);
	fputs(cnt_len, fp);
	fputs(cnt_type, fp);
	fputs(content, fp);
	fflush(fp);
}

/*���� �޽��� ���*/
void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	return;
}
