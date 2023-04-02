#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFF_SIZE 1024
#define SMALL_BUFF 100
void *request_handler(void*arg);
void send_data(FILE*fp,char * ct,char* file_name);
char* content_type(char* file);
void send_error(FILE *fp);
void error_handling(char *message);

int main(int argc, char *argv[]){

        int serv_sock, clnt_sock;
        struct sockaddr_in serv_addr, clnt_addr;
        socklen_t clnt_addr_size;
        pthread_t t_id;

        if(argc!=2)//포트를 입력하지 않았다면 입력하라는 메시지 출력
        {
                printf("Usage : %s <port>\n", argv[0]); 
                exit(EXIT_FAILURE);
        }

        serv_sock = socket(PF_INET, SOCK_STREAM, 0); // 서버 소켓 생성

				/*서버 소켓에 할당할 정보들*/
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(atoi(argv[1]));

				/* 빈 서버소켓에 ip와 port 설정*/
        if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ==-1)
                error_handling("bind() error");
				
				/*클라이언트가 접속 가능한 상태(대기열 20)*/
        if(listen(serv_sock,20) == -1)
                error_handling("listen() error");

        while(1)
        {
                clnt_addr_size = sizeof(clnt_addr);
	            clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,(socklen_t*)&clnt_addr_size); //클라이언트의 연결을 허용

                printf("Connection Request : %s:%d\n",
                inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port)); // ip 와 port 를 출력
					
								/* 멀티쓰레드를 이용해 동시에 여러클라이언트와 접속*/			
                pthread_create(&t_id,NULL,request_handler, &clnt_sock); 
                pthread_detach(t_id); //쓰레드 자원해제
        }
				
        close(serv_sock); //모든 작업이 끝나면 서버 소켓 종료
        return 0;
}
void *request_handler(void *arg) //클라이언트 소켓을 넘겨받음
{
        int clnt_sock = *((int*)arg); //클라이언트는 웹 브라우저가 됨. 
        char req_line[SMALL_BUFF];
        FILE* clnt_read;
        FILE* clnt_write;

        char method[10];
        char ct[15];
        char file_name[30];

        clnt_read = fdopen(clnt_sock, "r"); //클라이언트로부터 데이터를 읽기 위한 스트림
        clnt_write = fdopen(dup(clnt_sock), "w"); //클라이언트에게 데이터를 쓰기 위한 스트

        fgets(req_line, SMALL_BUFF, clnt_read); // 여기에 입력된 값을 req_line에 저장하고, clnt_read을 넣음(읽기모드의 파일 포인터) 요청메시지를 받음

        if(strstr(req_line, "HTTP/")==NULL) // 'HTTP /' 문자열 검색
        {
                send_error(clnt_write); //만약 'HTTP /' 이 없다면 오류메시지 출력
                fclose(clnt_read);
                fclose(clnt_write);
                return NULL;
        }

        strcpy(method, strtok(req_line, " /"));
        if(strcmp(method, "GET")!=0) //strcmp 같으면 0을 반환함.
        {
                send_error(clnt_write); //send_error 함수 호출
                fclose(clnt_read); //read 스트림 닫음
                fclose(clnt_write); //write 스트림 닫음 
                return NULL;
        }
		strcpy(file_name, strtok(NULL, " /")); //문자열을 자르고, file_name 에 복사 
        strcpy(ct, content_type(file_name));  //content_type함수값을 ct에 복사
        fclose(clnt_read);  //입력을 위한 스트림  종료

        send_data(clnt_write, ct, file_name); //클라이언트한테 데이터 송신
		return NULL;
}
void send_data(FILE * fp, char* ct, char*file_name)
{
        char protocol[] = "HTTP/1.0 200 OK\r\n"; //응답 헤더 메시지
        char server[] = "Server:Linux Web Server\r\n";
        char cnt_len[] = "Content-lenght:2048\r\n";
        char cnt_type[SMALL_BUFF]; // 콘텐츠 타입 문자배열 선언
        char buf[BUFF_SIZE];

        FILE* send_file;  //응답 메시지를 보낼 파일 포인터

        sprintf(cnt_type, "Content-type :%s\r\n\r\n", ct);  // 2번째 인자에 콘텐츠 타입을 넣고, cnt_tpye 에 문자열 저장
        send_file = fopen(file_name, "r"); //클라이언트가 요청한 파일 열기
        if(send_file == NULL) 
        {
                send_error(fp); //에러 메시지
                fclose(fp);
                return;
        }

        fputs(protocol, fp); //파일에 쓸 문자열을 넣고, 출력모드의 파일포인터를 넘긴다.
        fputs(server, fp);
        fputs(cnt_len,fp);
        fputs(cnt_type, fp);
				
				while(fgets(buf, BUFF_SIZE, send_file) != NULL) //널을 만날때가지 요청한 데이터 전송
        {
                fputs(buf,fp);
                fflush(fp);
        }
        fflush(fp); //스트림 비우기
        fclose(fp); //파일 닫기 
}

char *content_type(char* file) //콘텐츠 타입 구분
{
        char extension[SMALL_BUFF];
        char file_name[SMALL_BUFF];

        strcpy(file_name, file);
        strtok(file_name, ".");
        strcpy(extension, strtok(NULL, "."));
	
        if(!strcmp(extension, "html")||!strcmp(extension, "htm")) //html 형식인지를 검사
                return "text/html";
        else
                return "text/plain";
}
void send_error(FILE *fp) //요청 오류 발생시 오류메세지를 전달
{
        char protocol[]="HTTP/1.0 400 Bad Request\r\n";
        char server[]="Server:Linux Web Server\r\n";
        char cnt_len[]="Content-length:2048\r\n";
        char cnt_type[]="Content-type:text/html\r\n\r\n";
        char content[]= "<html><head><title>NETWORK</title><head>"
                "<body><font size+=10><br>오류 발생 요청 파일명 및 요청 방식 확인"
                "</font></body></html>";

				/*헤더 정보 전송*/
        fputs(protocol,fp);
        fputs(server,fp);
        fputs(cnt_len,fp);
        fputs(cnt_type,fp);
        fflush(fp);
}
void error_handling(char *message) //error 메시지 함수
{
        fputs(message,stderr);
        fputc('\n', stderr);
        exit(EXIT_FAILURE);
}
