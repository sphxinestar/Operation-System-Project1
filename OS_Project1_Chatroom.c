#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "server.h"
//#include "proto.h"
#include "string.h"

#define LENGTH_NAME 31
#define LENGTH_MSG 101
#define LENGTH_SEND 201



//int protocol = -1; //如果protocol為0為TCP 為1為UDP
int port; //連接桿
char filename[256]; //存輸入的TXT檔案名字 以便之後找位置
char ip[256];//存IP

Clients *root,*current;


void error(const char *msg) //print error msg(總之函式裡面有)
{
    perror(msg);
    exit(0);
}

void send_to_all(Clients *list, char tmp_buffer[]){
	Clients *tmp = root->link;//現在server本身
	while(tmp != NULL){
	     if(list->data != tmp->data){//除了server之外的client
		printf("Data send to client %d: \"%s\" \n", tmp->data, tmp_buffer);
		send(tmp->data, tmp_buffer, LENGTH_SEND, 0);// 將資料send出去
	     }
	     tmp = tmp->link;//將temp指向下一個
        }
}

//處理server進來client的東西
void handler(void *client_port){ 
	int leave_flag = 0;
	char nickname[LENGTH_NAME] = {};
	char recv_buffer[LENGTH_MSG] = {};
	char send_buffer[LENGTH_SEND] = {};
	Clients *list = (Clients *)client_port;
	

	FILE *file; //開檔案啦
	//file = fopen(filename,"a");
	/*if(file == NULL){
		error("ERROR Fopen!");
	    }else{printf("successful file\n");}*/

	//如果沒收到資料或是資料過大的話就print以下東東,然後離開當下的client
	if (recv(list->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) >= LENGTH_NAME-1){
	     printf("%s no input.\n",list->IP);
	     leave_flag = 1;
	}else{ // 把nickname放到list->name裡面,分別在server和client顯示
	     strncpy(list->name, nickname, LENGTH_NAME);
	     printf("%s(%s)(%d) join the chatroom.\n", list->name, list->IP, list->data);
	     sprintf(send_buffer, "%s(%s) join the chatroom.", list->name, list->IP);
	     send_to_all(list,send_buffer);
	     strcat(send_buffer, "\n");
	     file = fopen(filename,"a"); //開檔案啦
	     fwrite(send_buffer,sizeof(char),strlen(send_buffer),file); //將send buffer資料寫進檔案
	     fclose(file);//關檔
	}
	
	while(1){//用無限迴圈來等訊息or leave flag傳進來
	     if(leave_flag){
		break;
	     }
	     int receive = recv(list->data, recv_buffer, LENGTH_MSG, 0);
	// 若recv的file descriptor 為0或著輸入的字串為exit的話則當前client離開連線
	     if (receive == 0 || strcmp(recv_buffer, "exit") == 0){
		printf("%s(%s)(%d) leave the chatroom.\n",list->name, list->IP, list->data);
		sprintf(send_buffer, "%s(%s) leave the chatroom.", list->name, list->IP);
		leave_flag = 1;
	     }
	//若recv的file descriptor > 0 且 recv_buffer中沒有訊息,則回到迴圈開頭
	     else if(recv > 0){
		if(strlen(recv_buffer) == 0){
		     continue;
		}
		// 將name,recv_buffer,ip等字接起來
		sprintf(send_buffer,"%s : %s from %s",list->name, recv_buffer, list->IP);
	     }
	// 離開
	     else{
		leave_flag = 1;
	     }
	//從server端傳資料給各個client
	send_to_all(list,send_buffer);
	
	//將資料寫進檔案	
	strcat(send_buffer, "\n");
	file = fopen(filename,"a");
	fwrite(send_buffer,sizeof(char),strlen(send_buffer),file);
	fclose(file);
	}
	//將現在的client從memory中清除
	close(list->data);
	if(list == current){
	     current = list->prev;
	     current->link = NULL;
	}else{
	     list->prev->link = list->link;
	     list->link->prev = list->prev;
	}
	free(list);
}


//server端接取ctrl+c signal時執行的function
void catch_ctrl_c(int sig){
	Clients *tmp;
	while(root != NULL){
		printf("\nClose socketfd : %d\n", root->data);
		close(root->data);
		tmp = root;
		root = root->link;
		free(tmp);
	}
	printf("Bye\n");
	exit(EXIT_SUCCESS);
}


void server(){

	     signal(SIGINT, catch_ctrl_c);

	     int sockfd, newsockfd;
	     socklen_t clilen;
	     socklen_t servlen;
	     char buffer[256];
	     struct sockaddr_in serv_addr, cli_addr;



	    // FILE *file;
	     //file = fopen(filename,"rb");//rb是開啟檔案給程式讀取wb下面有註解 一起使用的
	     sockfd = socket(AF_INET, SOCK_STREAM, 0);//建立一個 socket設定他的領域形式跟Protocal 通常是0 讓kernel默認
	     if (sockfd < 0){
	        error("ERROR opening socket");
	     }




	     /////bzero((char *) &serv_addr, sizeof(serv_addr)); //下面註解過了
	     servlen = sizeof(serv_addr);
	     clilen = sizeof(cli_addr);
	     bzero((char *) &serv_addr, servlen);
	     bzero((char *) &cli_addr, clilen);
	     bzero(buffer,256);



	     serv_addr.sin_family = AF_INET;//ip.port AF_INET
	     serv_addr.sin_addr.s_addr = INADDR_ANY;
	     serv_addr.sin_port = htons(port);//portno->port


	     bind(sockfd, (struct sockaddr *) &serv_addr, servlen); //connect是去別人家用資料 bind是把地址綁在自己身上的感覺
	     listen(sockfd,5);


	     getsockname(sockfd, (struct sockaddr*) &serv_addr, (socklen_t*) &servlen);
	     printf("Start server on : %s : %d\n",inet_ntoa(serv_addr.sin_addr),ntohs(serv_addr.sin_port));

	     //將當前server設為node,加進linked-list
	     root = newNode(sockfd, inet_ntoa(serv_addr.sin_addr));
	     current = root;



	while(1){

	     newsockfd = accept(sockfd,(struct sockaddr  *) &cli_addr,&clilen);//看不懂幹麻的可能是有關父子關係的東西總之不能accept就是這裡出問題
	     if (newsockfd < 0){
	          error("ERROR on accept");
	     }

	     getpeername(newsockfd,(struct sockaddr*) &cli_addr, &clilen);
	     printf("Client %s : %d come in.\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	   
	     // 把client加進linked-list  
	     Clients *c = newNode(newsockfd, inet_ntoa(cli_addr.sin_addr));
	     c->prev = current;
	     current->link = c;
	     current = c;
	     //create multithread
	     pthread_t id;
	     if (pthread_create(&id, NULL, (void *)handler, (void *)c) != 0) {
         	perror("Create pthread error!\n");
            	exit(EXIT_FAILURE);
             }	



	     bzero(buffer,256);
/*
             fseek(file,0,SEEK_END);
             int size = ftell(file);
             fseek(file,0,SEEK_SET);//成功紀錄檔案大小
 	     
	     while(!feof(file)){
	     	percent = fread(buffer,sizeof(char),1,file);
		write(newsockfd,buffer,percent);
                data = data + percent;
	   	if(data*20 >= size*count){
                        printf("%d%% ",count*5);
                        time_t timep;
                        time(&timep);
                        printf("%s",ctime(&timep));
                        count++;

	     	}
	     }
	

	     fclose(file);*/
	     //close(newsockfd);
	     //close(sockfd);
	}
	close(newsockfd);
	close(sockfd);
}


// CLIENT PART


volatile sig_atomic_t flag = 0;
int cli_sockfd = 0;
char cli_nickname[LENGTH_NAME] = {};


void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

// 處理接受訊息的地方
void recv_msg_handler() {
    char receiveMessage[LENGTH_SEND] = {};
    while (1) {
        int receive = recv(cli_sockfd, receiveMessage, LENGTH_SEND, 0);
        if (receive > 0) {
            printf("\r%s\n", receiveMessage);
            str_overwrite_stdout();
        } else if (receive == 0) {
            break;
        } else { 
            // -1 
        }
    }
}
// 處理傳送訊息的地方
void send_msg_handler() {
    char message[LENGTH_MSG] = {};
    while (1) {
        str_overwrite_stdout();
        while (fgets(message, LENGTH_MSG, stdin) != NULL) {
            str_trim_lf(message, LENGTH_MSG);//遇到換行,則把'\n'換成'\0'
            if (strlen(message) == 0) {
                str_overwrite_stdout();
            } else {
                break;
            }
        }
	//傳訊息到client
        send(cli_sockfd, message, LENGTH_MSG, 0);
        if (strcmp(message, "exit") == 0) {
            break;
        }
    }
    //接受Ctrl+C signal的地方
    catch_ctrl_c_and_exit(2);
}


void client(){

    	signal(SIGINT, catch_ctrl_c_and_exit);

    	// Naming
    	printf("Please enter your name: ");

    	if (fgets(cli_nickname, LENGTH_NAME, stdin) != NULL) {

    	    str_trim_lf(cli_nickname, LENGTH_NAME);
    	}
    	if (strlen(cli_nickname) < 2 || strlen(cli_nickname) >= LENGTH_NAME-1) {
    	    printf("\nName must be more than one and less than thirty characters.\n");
    	    exit(EXIT_FAILURE);
    	}


	    //int n, percent;//percent 紀錄傳送百分比
	    struct sockaddr_in serv_addr, cli_addr;//已經被定義好了這樣就可以使用connect()
	    int s_addrlen = sizeof(serv_addr);
	    int c_addrlen = sizeof(cli_addr);
	    struct hostent *server;
	    char buffer[256];
/*
	    FILE *file;//準備要來覆寫txt檔案 //'wb'
	    char filename[256] = "file_recv";//因為還沒有這檔案 fopen會幫我自動建立這檔案
	    file = fopen(filename,"wb");
	    if(file == NULL){
		error("ERROR Fopen!");
	    }else{printf("successful file\n");}
*/
	    cli_sockfd = socket(AF_INET, SOCK_STREAM, 0);//建立一個 socket設定他的領域形式跟Protocal 通常是0 讓kernel默認
	    if (cli_sockfd < 0){
	            error("ERROR opening socket");//代表創socket失敗
	    }

	    server = gethostbyname(ip);//argv[1]->ip Readme 說的
	    if (server == NULL) {
		    error("no such host!");
	    }
	    bzero((char *) &serv_addr, sizeof(serv_addr));//設定ip.port
	    serv_addr.sin_family = AF_INET;
	    serv_addr.sin_addr.s_addr = inet_addr(ip);
	    //bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	    serv_addr.sin_port = htons(port);//portno->port

	    if (connect(cli_sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
		    error("ERROR connecting");//connect 讓我們去server那取資料
	    }


	    bzero(buffer,256);
	    /*while(1){
	    percent = read(cli_sockfd,buffer,sizeof(char));
	    if(percent == 0){
	    	printf("Finished!\n");
			break;
	    }else if(percent < 0){
		error("ERROR transmission!\n");
	    }
	    percent = fwrite(buffer,sizeof(char),1,file);
	    }*/

	    //fclose(file);


	    // Names
	getsockname(cli_sockfd, (struct sockaddr*) &cli_addr, (socklen_t*) &c_addrlen);
    	getpeername(cli_sockfd, (struct sockaddr*) &serv_addr, (socklen_t*) &s_addrlen);
    	printf("Connect to Server : %s : %d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    	printf("You are: %s : %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	
	//傳訊息到client
    	send(cli_sockfd, cli_nickname, LENGTH_NAME, 0);

	//create multithread
	    pthread_t send_msg_thread;
    	if (pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0) {
    	    printf ("Create pthread error!\n");
    	    exit(EXIT_FAILURE);
    	}

    	pthread_t recv_msg_thread;
    	if (pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0) {
    	    printf ("Create pthread error!\n");
   		exit(EXIT_FAILURE);
   	}
	//接受離開flag
    	while (1) {
    	    if(flag) {
		flag = 0;
    	        printf("\nBye\n");
    	        break;
    	    }
    	}

	    close(cli_sockfd);
}

//main function
int main(int argc, char *argv[]){

	//用argv讀取ip,port,filename以及是server端或是client端
        strcpy(ip, argv[2]);
        port = atoi(argv[3]);

        if(strcmp(argv[1], "server") == 0){
                strcpy(filename, argv[4]);
		FILE *file;
		file = fopen(filename,"w");
		fclose(file);
                server();
        }else if(strcmp(argv[1], "client") == 0){
                client();
        }else{
                printf("Error client / server input!");
        }

        return 0;
}
