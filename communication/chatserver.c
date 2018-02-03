#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAXCLIENTS 5
#define PORT 10140
int main(int argc,char **argv) {
	int sock;
	struct sockaddr_in svr;
	struct sockaddr_in clt;
	struct hostent *cp;
	int clen;
	char rbuf[1024];
	char * p_rbuf;
	char wbuf[1024];
	int nbytes;
	int reuse;
	int max_csock=0;
	int num_client = 0; //参加クライアント数
	char tmp_wbuf[50];

	int test;

	typedef struct t_c_sock{
		char clientname[1024];
		int c_sock;
		struct t_c_sock * next;
	} c_record;

	c_record * new;
	c_record * back = NULL;
	c_record * tmp;
	c_record * front = NULL;
	c_record * tmp_message;
	c_record * del;

/* simple talk */
	int name_flag = 0;
	int flag_break = 0;
	int tmp_csock;
	fd_set rfds;/* select()で用いるファイル記述子集合*/
	struct timeval tv;   /* select()が返ってくるまでの待ち時間を指定する変数*/

	struct tm data;
	time_t t;
/*	(c1)	*/
/*ソケットの生成*/
	if ((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) {
		perror("socket");
		exit(1);
	}
/*ソケットアドレス再利用の指定*/
	reuse=1;
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0) {
		perror("setsockopt");
		exit(1);
	}
/* client受付用ソケットの情報設定*/
	bzero(&svr,sizeof(svr));
	svr.sin_family=AF_INET;
	svr.sin_addr.s_addr=htonl(INADDR_ANY);
/*受付側のIPアドレスは任意*/
	svr.sin_port=htons(PORT);
/*ポート番号10140を介して受け付ける*/
/*ソケットにソケットアドレスを割り当てる*/
	if(bind(sock,(struct sockaddr *)&svr,sizeof(svr))<0) {
		perror("bind");
		exit(1);
	}
/*待ち受けクライアント数の設定*/
	if (listen(sock,MAXCLIENTS)<0) {
/*待ち受け数に5を指定*/
		perror("listen");
		exit(1);
	}
	num_client = 0;
	/*	(c1)	*/


do{
	/*
	if(back != NULL){
		tmp = back;
		while(1){
			printf("address:%p  c_sock:%d clientname:%s next:%p\n",tmp, tmp -> c_sock, tmp -> clientname, tmp -> next);
			if(tmp -> next == NULL) break;
			tmp = tmp -> next;
		}
	}
	*/
	FD_ZERO(&rfds);     /* rfdsを空集合に初期化*/
	FD_SET(0,&rfds);    /*標準入力*/
	FD_SET(sock,&rfds);
	max_csock = sock;
	if(back != NULL){
		tmp = back;
		while(1){
			FD_SET(tmp -> c_sock,&rfds);
			if(max_csock < tmp -> c_sock){
				max_csock = tmp -> c_sock;
			}
			if(tmp -> next == NULL) break;
			tmp = tmp -> next;
		}
	}
	/*監視する待ち時間を1秒に設定*/
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	/*標準入力とソケットからの受信を同時に監視する*/
	if(select(max_csock+1,&rfds,NULL,NULL,&tv)>0) {
		if(num_client > 0){
			tmp = back;
			while(1){
				if(FD_ISSET(tmp -> c_sock,&rfds)){
					memset(rbuf, 0, strlen(rbuf));
					rbuf[1023] = '\0';
					if ( ( nbytes = read(tmp -> c_sock,rbuf,sizeof(rbuf)) ) < 0) {
						perror("read");
					} else if (nbytes <= 0){
						flag_break=1;
					} else {
							if(strncmp(rbuf,"/list",5)  == 0){
								memset(wbuf, 0, sizeof(wbuf));
								wbuf[1023] = '\0';
								tmp_message = back;
								sprintf(wbuf, "users are as follows\n");
								while(1){
									memset(tmp_wbuf, '\0', sizeof(tmp_wbuf));
									tmp_wbuf[49] = '\0';
									sprintf(tmp_wbuf, "[%s]\n", tmp_message -> clientname);
									strcat(wbuf, tmp_wbuf);
									if(tmp_message -> next == NULL) break;
									tmp_message = tmp_message -> next;
								}
								write(tmp -> c_sock, wbuf, sizeof(wbuf));
							}else{
								tmp_message = back;
								while(1){
									memset(wbuf, 0, strlen(wbuf));
									wbuf[1023] = '\0';
									t = time(NULL);
									localtime_r(&t, &data);
									sprintf(wbuf, "[%s][%04d/%02d/%02d %02d:%02d:%02d]>%s", tmp -> clientname,data.tm_year + 1900,
									data.tm_mon + 1 ,data.tm_mday ,data.tm_hour, data.tm_min, data.tm_sec, rbuf);
									write(tmp_message -> c_sock,wbuf, sizeof(wbuf));
									if(tmp_message -> next == NULL) break;
									tmp_message = tmp_message -> next;
								}
						}
					}
					if(flag_break == 1){
						flag_break = 0;
						del = back;
						if(tmp == back){
							if(num_client == 1){
								free(back);
								back = NULL;
								num_client--;
								max_csock = sock;
							}else{
								del = back -> next;
								tmp_message = back;
								memset(wbuf, '\0', sizeof(wbuf));
								wbuf[1023] = '\0';
								memset(tmp_wbuf, '\0', sizeof(tmp_wbuf));
								tmp_wbuf[49] = '\0';
								strcpy(tmp_wbuf, tmp -> clientname);
								t = time(NULL);
								localtime_r(&t, &data);
								sprintf(wbuf, "user %s left at %04d/%02d/%02d %02d:%02d:%02d\n", tmp_wbuf ,data.tm_year + 1900,
								data.tm_mon + 1 ,data.tm_mday ,data.tm_hour, data.tm_min, data.tm_sec);
								while(1){
									write(tmp_message -> c_sock,wbuf, sizeof(wbuf));
									if(tmp_message -> next == NULL) break;
									tmp_message = tmp_message -> next;
								}
								free(back);
								back = del;
								num_client--;
								max_csock = sock;
							}
						}else{
							while((del -> next) -> c_sock != tmp -> c_sock){
								del = del -> next;
							}
							front = del;
							del -> next = tmp -> next;
							tmp_message = back;
							memset(wbuf, '\0', sizeof(wbuf));
							wbuf[1023] = '\0';
							memset(tmp_wbuf, '\0', sizeof(tmp_wbuf));
							rbuf[1023] = '\0';
							strcpy(tmp_wbuf, tmp -> clientname);
							t = time(NULL);
							localtime_r(&t, &data);
							sprintf(wbuf, "user %s left at %04d/%02d/%02d %02d:%02d:%02d\n", tmp_wbuf ,data.tm_year + 1900,
							data.tm_mon + 1 ,data.tm_mday ,data.tm_hour, data.tm_min, data.tm_sec);
							while(1){
								write(tmp_message -> c_sock,wbuf, sizeof(wbuf));
								if(tmp_message -> next == NULL) break;
								tmp_message = tmp_message -> next;
							}
							free(tmp);
							num_client--;
							max_csock = sock;
						}
					}
				}
				if(tmp -> next == NULL) break;
				tmp = tmp -> next;
			}
		}
		if(FD_ISSET(sock,&rfds)){
			clen = sizeof(clt);
			if ( ( tmp_csock = accept(sock,(struct sockaddr *)&clt,&clen) ) <0 ) {
				perror("accept");
				exit(2);
			}
	/*クライアントのホスト情報の取得*/
			cp = gethostbyaddr((char *)&clt.sin_addr,sizeof(struct in_addr),AF_INET);
			printf("[%s]\n",cp->h_name);
			if(num_client < MAXCLIENTS){
				write(tmp_csock,"REQUEST ACCEPTED\n",17);
				if((new = (c_record * )malloc(sizeof(c_record))) == NULL){
					printf("malloc error\n");
				}else{
					new -> c_sock = tmp_csock;
					new  -> next = NULL;
					if ( ( nbytes = read(new -> c_sock ,rbuf,sizeof(rbuf))) < 0) {
						perror("read");
					}
					p_rbuf = rbuf;
					for(int i = 0;;i++){
						if(*(p_rbuf + i) == '\n'){
							rbuf[i] = '\0';
							break;
						}
					}
					if(num_client > 0){
						tmp = back;
						while(1){
							if(strcmp(rbuf, tmp -> clientname) == 0){
								name_flag = 1;
								break;
							}
							if(tmp -> next == NULL) break;
							tmp = tmp -> next;
						}
					}
					if(name_flag == 1){
						name_flag = 0;
						write(new -> c_sock,"USERNAME REJECTED\n",sizeof("USERNAME REJECTED\n") );
						printf("username %s rejected.\n",rbuf);
						close(new -> c_sock);
						free(new);
					}else{
						if(num_client == 0){
							back = new;
							front = new;
						}else{
							front -> next = new;
							front = new;
						}
						test = write(new -> c_sock,"USERNAME REGISTERED\n",20);
						strcpy(new -> clientname, rbuf);
						printf("username %s registered.\n",rbuf);
						num_client++;
						printf("the number of clients:%d\n",num_client);

						tmp_message = back;
						memset(wbuf, '\0', sizeof(wbuf));
						t = time(NULL);
						localtime_r(&t, &data);
						sprintf(wbuf, "user %s joined at %04d/%02d/%02d %02d:%02d:%02d\n", new -> clientname,data.tm_year + 1900,
						data.tm_mon + 1 ,data.tm_mday ,data.tm_hour, data.tm_min, data.tm_sec);
						while(1){
							write(tmp_message -> c_sock,wbuf, sizeof(wbuf));
							if(tmp_message -> next == NULL) break;
							tmp_message = tmp_message -> next;
						}
					}
				}
			}else{
				write(tmp_csock,"REQUEST REJECTED\n",sizeof("REQUEST REJECTED\n"));
				close(tmp_csock);
			}
		}
	}
}while(1);
}
