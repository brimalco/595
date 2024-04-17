#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

/* This is a reference socket server implementation that prints out the messages
 * received from clients. */

#define MAX_PENDING 10
#define MAX_LINE 20

void *handshake(void *arg);

//need to give argument when running command in terminal
int main(int argc, char *argv[]) {
  int port = atoi(argv[1]);
  char *host_addr = "127.0.0.1";
  pthread_t tids[100];
  int i;
  int id[100];

  for (i=0; i<100; i++) id[i] = i;

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

    /* wait for connection */
    int new_s;
    socklen_t len = sizeof(sin);
    
    while(1) {
      if((new_s = accept(s, (struct sockaddr *)&sin, &len)) <0){
      perror("simplex-talk: accept");
      fflush(stdout);
      break;
      }

      tids[i] = id[i];

      int *pclient = malloc(sizeof(int));
       
      *pclient = new_s;
      pthread_create(&tids[i], NULL, handshake, (void*)pclient);

      //close(new_s);

    }
      
  }
  return 0;
}

void *handshake(void* p_new_s){ //void* arg
  int temp_s = *(int*)p_new_s;
  free(p_new_s);
  // int temp_s; // =  malloc(sizeof(int));
  // if((temp_s = socket(PF_INET, SOCK_STREAM, 0)) <0) {
  //         perror("simplex-talk: socket");
  //         fflush(stdout);
  //         exit(1);
  // }

  //int id = *((int*) arg);
  //int i;
  int len;

  //for (i=0; i<3; i++) {
    char seq_num[MAX_LINE];
    char hello[] = "HELLO ";
    char msg[MAX_LINE] = "";
    const char delim[2] = " ";
    int flag;
    int temp;

    len = recv(temp_s, msg, sizeof(msg), 0);
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
    send(temp_s, msg, len, 0);
    recv(temp_s, msg, sizeof(msg), 0);
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
      //break;
    }
  //}
  close(temp_s);
  pthread_exit(NULL);
}