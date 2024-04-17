#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_LINE 20

int main (int argc, char *argv[]) {
  char* host_addr = argv[1];
  int port = atoi(argv[2]);
  char* seq_num = argv[3];

  /* Open a socket */
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0){
    perror("simplex-talk: socket");
    fflush(stdout);
    exit(1);
  }

  /* Config the server address */
  struct sockaddr_in sin;
  sin.sin_family = AF_INET; 
  sin.sin_addr.s_addr = inet_addr(host_addr);
  sin.sin_port = htons(port);
  // Set all bits of the padding field to 0
  memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));

  /* Connect to the server */
  if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0){
    perror("simplex-talk: connect");
    fflush(stdout);
    close(s);
    exit(1);
  }


  char msg[MAX_LINE] = "";
  char hello[] = "HELLO ";
  const char delim[2] = " ";
  int flag = atoi(seq_num);
  
  int temp;
  strcat(msg, hello);
  strcat(msg, seq_num);
  int len = strlen(msg)+1;
  send(s, msg, len, 0);
  flag++;
  recv(s, msg, sizeof(msg), 0);
  fputs(msg, stdout);
  fflush(stdout);
  fputs("\n",stdout);
  fflush(stdout);
  strtok(msg, delim);
  strcpy(msg, strtok(NULL, delim));
  strcpy(seq_num, msg); 
  strcpy(msg,"");
  if(atoi(seq_num) == flag){
    temp = atoi(seq_num);
    temp++;
    sprintf(seq_num, "%i", temp);
    strcat(msg, hello);
    strcat(msg, seq_num); 
    send(s, msg, len, 0);
  } else{
    perror("Error Y: Incorrect Value");
    fflush(stdout);
    close(s);
    exit(1);
  }
  
  close(s);

  return 0;
}
