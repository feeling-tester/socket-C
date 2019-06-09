#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <EzGraph.h>

#define PORT 22222
//#define SERVER_IP "127.0.0.1"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
void timer_event(void);
void key_event(int);
const char* get_my_ip(const char *);
const char* get_device_name();

char input_buf[100] = {0};
const char* device_name;
const char* ip;
int fd;

int main(int argc, char** argv)
{

  const char* SERVER_IP;
  FILE *fp;
  char line[128];
  char *str;
  char key[128];
  char value[128];

  if ((fp = fopen("config", "r")) == NULL)
  {
    printf("File cannot open\n");
    exit(1);
  }

  while ((str = fgets(line, 128, fp)) != NULL) {
    sscanf(line, "%[^=]=%s", key, value);
    if (strcmp(key, "SERVER_IP") == 0) {
      SERVER_IP = value;        
    }
    /* printf("key   = %s\n", key); */
    /* printf("value = %s\n", value); */
  }

  fclose(fp);

  device_name = get_device_name();
  ip = get_my_ip(device_name);
  printf("%s\n", ip);
  struct sockaddr_in addr;
  // IPv4 TCP のソケットを作成する
  if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
 
  // 送信先アドレスとポート番号を設定する
  addr.sin_family = PF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = inet_addr(SERVER_IP);

  printf("接続中…\n");
  // サーバ接続（TCP の場合は、接続を確立する必要がある）
  while (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
    sleep(1);
  }
  printf("接続を確立しました\n");
  int val = 1;
  ioctl(fd, FIONBIO, &val);   

  /* // パケットを TCP で送信 */
  /* if(send(fd, "connected!", 17, 0) < 0) { */
  /*   perror("send"); */
  /*   return -1; */
  /* } */

  EzOpenWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
  EzDrawStringB(50,50,"入力してね");
  EzSetKeyHandler(key_event);
  EzSetTimerHandler(timer_event, 100); /* Every 1sec */
  EzShowBuffer();
  EzEventLoop();
  printf("通信を切断します。\n");
  ///これ以降は不要
  

  close(fd);
 
  return 0;
}

void timer_event(void){
  int cnt;
  char buf[2048] = {0};
  //printf("ssaa\n");
  cnt = recv(fd, buf, sizeof(buf), 0);
  if (cnt > 0) {
    // パケット受信成功の場合
    fprintf(stdout, "%s\n", buf);
  }
  else if (cnt == 0) {
    // 切断された場合、クローズする
    //close(fd_array[i]);
  }
  else {
    //printf("out\n");
    //fprintf(stdout, "a%s\n", buf);
    //perror("aaaa");
    //goto end;
  }  
}

void key_event(int n){
  //printf("Push '%c' (0x%02x) key\n", n, n);
  int len = strlen(input_buf);
  if (('a' <= n && n <= 'z') || ('A' <= n && n <= 'Z') ||
      ('0' <= n && n <= '9') || n == ',' || n == '.' ||
      n == '-' || n == ' ' || n == ':') {
    input_buf[len] = n;
    input_buf[len+1] = '\0';
  }
  else if (n == 0x08 && len > 0) { //Back Space
    input_buf[len-1] = '\0'; 
  }
  else if (n == 0x0d) { // Enter
    if (strcmp(input_buf, ":quit") == 0) {
      EzExitEventLoop();
    }
    else {
      char message[1050];
      strcpy(message, ip);
      strcat(message, ":");
      strcat(message, input_buf);
      if (send(fd, message, sizeof(message), 0) < 0) {
        perror("send");        
      }
      input_buf[0] = '\0';
    }
    //printf("%s\n", input_buf);
  }
  //printf("%s\n", input_buf);
  EzDrawStringB(50,50,"入力してね");
  EzDrawStringB(50,250,input_buf);
  
  EzShowBuffer();
 
  
}



const char* get_my_ip(const char* device_name) {
  //指定したデバイス名のIPアドレスを取得します。
  int s = socket(AF_INET, SOCK_STREAM, 0);
  
  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strcpy(ifr.ifr_name, device_name);
  ioctl(s, SIOCGIFADDR, &ifr);
  close(s);
  
  struct sockaddr_in addr;
  memcpy( &addr, &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr_in) );
  return inet_ntoa(addr.sin_addr);
}

const char* get_device_name() {
  char *tmpFile = "cmd.tmp";
  char cmd[100] = "ip a";
  sprintf(cmd, "%s > %s", cmd, tmpFile);
  if (system(cmd)) perror("");
  
  FILE *file;
  char line[200];
  static char device_list[30][60] = {{0}};
  int i = 0, j = 0;
  int device_num = 0;

  file = fopen(tmpFile, "r");

  while(fgets(line, 200, file)!=NULL){
    if ('0' <= line[0] && line[0] <= '9') {
      int colon_cnt = 0;
      int start_idx = 0, end_idx = 0;
      char device_name[60] = {0};
      for (j = 0; j < strlen(line); j++) {
        if (line[j] == ':' && colon_cnt == 0) {
          start_idx = j;
          colon_cnt++;
        }
        else if (line[j] == ':' && colon_cnt == 1) {
          end_idx = j;
          break;
        }
      }
      strncpy(device_name, line+start_idx+2, end_idx-start_idx-2);
      device_name[end_idx] = '\0';
      //printf("%s\n", device_name);
      strcpy(device_list[i], device_name);
      i++;
      device_num = i;
    }
  }
  fclose(file);
  
  remove(tmpFile);
  int interface_id = -1;
  do {
    interface_id = -1;
    //  printf("%d\n", device_num);
    for (i = 0; i < device_num; i++) {
      printf("%d : %s\n", i, device_list[i]);
    }
    printf("使っているインターフェースを教えて：");
    scanf("%d", &interface_id);
    if (interface_id <= -1 || device_num <= interface_id) {
      printf("もっかい！\n");
    }
  } while (interface_id <= -1 || device_num <= interface_id);
  printf("選択されたインターフェース：%s\n", device_list[interface_id]);
  return device_list[interface_id];
}
