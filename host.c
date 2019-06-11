#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h> // for close
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h> 
#include <stdlib.h>

#define PORT 22222
#define CONNECTION_MAX 10
//http://hensa40.cutegirl.jp/archives/932
//http://www.eyes-software.co.jp/news/archives/7

int main(int argc, char** argv) {
  int fd;             // 接続待ち用ディスクリプタ
  int fd_array[CONNECTION_MAX];        // 通信用ディスクリプタの配列
  //struct servent *serv;
  struct sockaddr_in addr;
  socklen_t len = sizeof(struct sockaddr_in);
  struct sockaddr_in from_addr;
  char buf[2048];
  int cnt;
  int i;



  
  /* // 受信バッファを初期化する */
  /* memset(buf, 0, sizeof(buf)); */
  // 通信用ディスクリプタの配列を初期化する
  for (i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++){
    fd_array[i] = -1;
  }
  // ソケットを作成する
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket error\n");
    return -1;
  }
  
  // IPアドレス、ポート番号を設定
  addr.sin_family = PF_INET;
  addr.sin_port = ntohs(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;
  
  // バインドする
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind error");
    return -1;
  }
  // 接続待ち状態とする。待ちうけるコネクト要求は１個
  if (listen(fd, 20) < 0) {
    perror("listen error");
    return -1;
  }
  
  int fd_max;          // ディスクリプタの最大値
  fd_set fds_set;           // 接続待ち、受信待ちをするディスクリプタの集合
  struct timeval time_out;     // タイムアウト時間
  int val = 1;
  //int send_flag[CONNECTION_MAX] = {0};
  ioctl(fd, FIONBIO, &val);   
  for (i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++){
    ioctl(fd_array[i], FIONBIO, &val);     
  }
  while (1) {
    // 接続待ちのディスクリプタをディスクリプタ集合に設定する
    FD_ZERO(&fds_set);
    FD_SET(fd, &fds_set);
    fd_max = fd;
    
    // 受信待ちのディスクリプタをディスクリプタ集合に設定する
    for (i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++){
      if (fd_array[i] != -1){
        FD_SET(fd_array[i], &fds_set);
        if (fd_array[i] > fd_max) fd_max = fd_array[i];
      }
    }
    // タイムアウト時間を10secに指定する。
    time_out.tv_sec = 10;

    // 接続＆受信を待ち受ける
    cnt = select(fd_max+1, &fds_set, NULL, NULL, &time_out);
    //puts("select");
    if (cnt < 0){
      // シグナル受信によるselect終了の場合、再度待ち受けに戻る
      if (errno == EINTR) continue;
      // その他のエラーの場合、終了する。
      goto end;
    }
    else if (cnt == 0) {
      // タイムアウトした場合、再度待ち受けに戻る
      continue;
    }
    else {
      int connection_flag = 0;
      // 接続待ちディスクリプタに接続があったかを調べる
      if (FD_ISSET(fd, &fds_set)) {
        char message[1024] = {0};
        // 接続されたならクライアントからの接続を確立する
        for (int i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++) {
          if (fd_array[i] == -1) {
            if ((fd_array[i] = accept(fd, (struct sockaddr *)&from_addr, &len)) < 0) {
              goto end;
            }
            connection_flag = 1;
            sprintf(message, "socket:%d  connected.", fd_array[i]);
            printf("%s\n", message);
            sprintf(message, "%d番に人が参加しました。", fd_array[i]);

            break;
          }
        }
        if (connection_flag == 1) {
          for (int i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++) {
            if (fd_array[i] != -1) {
              /* puts("ss"); */
              if (message[0] - '0' == fd_array[i]) {
                sprintf(message, "あなたが参加しました。");
              }
              if (send(fd_array[i], message, sizeof(message), 0) < 0) {
                perror("send");
              }
              //puts("sended");
            }
          }
        }
      }
      int recieved_flag = 0;
      //printf("%ld\n", fds_set);

      for (int i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++) {
          // 受信待ちディスクリプタにデータがあるかを調べる
        if (FD_ISSET(fd_array[i], &fds_set)) {
          // データがあるならパケット受信する
          cnt = recv(fd_array[i], buf, sizeof(buf), 0);
          //printf("%d, %d\n", i, cnt);
          if (cnt > 0) {
            // パケット受信成功の場合
            fprintf(stdout, "suc:%s\n", buf);
            recieved_flag = 1;
            break;
          }
          else if (cnt == 0) {
            // 切断された場合、クローズする
            fprintf(stdout, "socket:%d  disconnected. \n", fd_array[i]);
            close(fd_array[i]);
            fd_array[i] = -1;
          }
          else {
            //printf("out\n");
            //fprintf(stdout, "a%s\n", buf);
            perror("error");
            goto end;
          }
        }
        //puts("ss");
      }

      if (recieved_flag == 1) {
        for (int i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++){
          if (fd_array[i] != -1) {
            if (send(fd_array[i], buf, sizeof(buf), 0) < 0) {
              perror("send");
            }
            printf("send to %d: %s\n", fd_array[i], buf);
          }
        }
      }
    }
    
    
  }
 end:
  // パケット送受信用ソケットクローズ
  for (int i = 0; i < sizeof(fd_array)/sizeof(fd_array[0]); i++){
    close(fd_array[i]);
  }
  // 接続要求待ち受け用ソケットクローズ
  close(fd);

  return 0;
}
