#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

/* This is a reference socket server implementation that prints out the messages
 * received from clients. */

#define MAX_PENDING 10
#define MAX_LINE 20

//need to give argument when running command in terminal

int main(int argc, char *argv[]) {
  int port = atoi(argv[1]);
  char *host_addr = "127.0.0.1";


  /*setup passive open*/
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0) {
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

  /* Bind the socket to the address */
  if((bind(s, (struct sockaddr*)&sin, sizeof(sin)))<0) {
    perror("simplex-talk: bind");
    fflush(stdout);
    exit(1);
  }

  while(1){
    // connections can be pending if many concurrent client requests
    listen(s, MAX_PENDING);  

    /* wait for connection, then receive and print text */
    int new_s;
    socklen_t len = sizeof(sin);
    char seq_num[MAX_LINE];
    char hello[] = "HELLO ";
    char msg[MAX_LINE] = "";
    const char delim[2] = " ";
    int flag;
    int temp;
    while(1) {
      if((new_s = accept(s, (struct sockaddr *)&sin, &len)) <0){
        perror("simplex-talk: accept");
        fflush(stdout);
        break;
      }

      len = recv(new_s, msg, sizeof(msg), 0);
      fputs(msg, stdout);
      fflush(stdout);
      fputs("\n",stdout);
      fflush(stdout);   
      strtok(msg, delim);
      strcpy(msg, strtok(NULL, delim));
      strcpy(seq_num, msg); 
      strcpy(msg,"");
      temp = atoi(seq_num);  
      temp++;
      sprintf(seq_num, "%i", temp);
      flag = atoi(seq_num);
      flag++;
      strcat(msg, hello);
      strcat(msg, seq_num); 
      send(new_s, msg, len, 0);
      recv(new_s, msg, sizeof(msg), 0);
      fputs(msg, stdout);
      fflush(stdout);
      fputs("\n",stdout);
      fflush(stdout);
      strtok(msg, delim);
      strcpy(msg, strtok(NULL, delim));
      strcpy(seq_num, msg);
      if(atoi(seq_num) == flag){

      }else{
        perror("Error Z: Incorrect Value");
        fflush(stdout);
        break;
      }
    }

    close(new_s);
  }
  

  return 0;
}
