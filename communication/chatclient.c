#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define PORT 10140
int main(int argc,char **argv) {
	int nbytes;
	struct sockaddr_in host;
	struct hostent *hp;
	char rbuf[512];
	char wbuf[512];
	char username[98];
	int receive_n;
	int sock;
	/* simple talk */
	fd_set rfds;/* select()で用いるファイル記述子集合*/
	struct timeval tv;   /* select()が返ってくるまでの待ち時間を指定する変数*/

	if (argc != 3) {
		fprintf(stderr,"Usage: %s hostname message\n",argv[0]);
		exit(1);
	}
/*ソケットの生成*/

/*	(c1)	*/
	if ( ( sock = socket(AF_INET,SOCK_STREAM,0) ) < 0) {
		perror("socket");
		exit(1);
	}
/* host(ソケットの接続先)の情報設定*/
	host.sin_family=AF_INET;
	host.sin_port=htons(PORT);
	if ( ( hp = gethostbyname(argv[1]) ) == NULL ) {
		fprintf(stderr,"unknown host %s\n",argv[1]);
		exit(1);
	}
	bcopy(hp->h_addr, &host.sin_addr,hp->h_length);
/* 2.connect*/
	if(connect(sock, (struct sockaddr *)&host,sizeof(host)) < 0){
		fprintf(stderr,"failure to connect host\n");
		exit(1);
	}else{
		printf("connected to %s\n", argv[1]);
	}
/*	(c1)	*/

/*	(c2)	*/
if ( ( nbytes = read(sock,rbuf,17 * sizeof(char))) < 0) {
	perror("read");
}
if(strncmp(rbuf, "REQUEST ACCEPTED\n", 17) != 0){
	close(sock);
	exit(1);
}else{
	printf("join request accepted\n");
}
/*	(c2)	*/

/*	(c3)	*/
strcpy(username, strcat(argv[2],"\n"));
write(sock,username,sizeof(username));
if ( ( nbytes = read(sock,rbuf,20 * sizeof(char))) < 0) {
	perror("read");
}
if(strncmp(rbuf, "USERNAME REGISTERED\n", 20) != 0){
	printf("username error.\n");
	close(sock);
	exit(1);
}else{
	printf("user name registered\n");
}
/*	(c3)	*/

/*	(c4)	*/
do{
	/*入力を監視するファイル記述子の集合を変数rfdsにセットする*/
	FD_ZERO(&rfds);     /* rfdsを空集合に初期化*/
	FD_SET(0,&rfds);    /*標準入力*/
	FD_SET(sock,&rfds);  /*クライアントを受け付けたソケット*/
	/*監視する待ち時間を1秒に設定*/
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	/*標準入力とソケットからの受信を同時に監視する*/
	if(select(sock+1,&rfds,NULL,NULL,&tv)>0) {
		if(FD_ISSET(0,&rfds)) {  /*標準入力から入力があったなら*/
			/*標準入力から読み込みクライアントに送信*/
			memset(rbuf, 0, strlen(rbuf));
			if(scanf("%s",rbuf) != EOF){
				write(sock,rbuf,sizeof(rbuf));
			}else{
				break;
			}
		}
		if(FD_ISSET(sock,&rfds)) { /*ソケットから受信したなら*/
			memset(rbuf, 0, strlen(rbuf));
			/*ソケットから読み込み端末に出力*/
			if ( ( nbytes = read(sock,rbuf,sizeof(rbuf)) ) < 0) {
				perror("read");
			}else if( nbytes == 0){
				break;
			}else{
				printf("%s\n",rbuf);
			}
		}
	}
}while(1);
/*	(c4)	*/

/*	(c5)	*/
close(sock);
exit(0);
/*	(c5)	*/
}
