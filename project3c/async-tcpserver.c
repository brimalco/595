#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_PENDING 30
#define MAX_LINE 20
#define MAX_CLIENTS 150

// struct for client server
typedef struct{
  int fd; //tracks the socket file descriptor
  int flag; //tracks where we are in the handshake
  int seq_num_check; //sequence number
} client_state;

// For example, my handle_first_shake took in the fd and returned y.
int handle_first_shake(client_state* arg);
void handle_second_shake(client_state* arg);

//need to give argument when running command in terminal
int main(int argc, char *argv[]) {
  int port = atoi(argv[1]);
  char *host_addr = "127.0.0.1";

  client_state client_state_array[100];

  for (int i=0; i<MAX_CLIENTS; i++) {
    client_state_array[i].flag = -1;
    client_state_array[i].fd = -1;
    client_state_array[i].seq_num_check = -1;
  }

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

  int j = 0;

  while(1){
    // connections can be pending if many concurrent client requests
    ////use fcntl() to disable blocking for the listening socket
    listen(s, MAX_PENDING);  

    //makes listen non-blocking
    fcntl(s, F_SETFL, O_NONBLOCK);

    /* wait for connection, then receive and print text */
    int new_s;
    socklen_t len = sizeof(sin);

    fd_set all_sockets, read_sockets;

    FD_ZERO(&all_sockets);
    FD_ZERO(&read_sockets);
    FD_SET(s, &all_sockets);
    int FD_MAX = s+1;


    while(1) {
      FD_ZERO(&read_sockets);
      //temp copy since select is destructive
      read_sockets = all_sockets;
    
      if (select(FD_MAX, &read_sockets, NULL, NULL, NULL) < 0) {
        perror("simplex-talk: select");
        fflush(stdout);
        break;
      }
    
      int check = -1;
      //just need to pass the address of the client state array to the helper function       
      for (int i=0; i < FD_MAX; i++){ 
          if(FD_ISSET(i, &read_sockets)){
            if (i == s){ //listening socket
              //new connection so we need to accept then do the first handshake
              if((new_s = accept(s, (struct sockaddr *)&sin, &len)) <0){
                perror("simplex-talk: accept");
                fflush(stdout);
                break;
              }
              fcntl(new_s, F_SETFL, O_NONBLOCK);

              //store new socket file into an unused client_state
              client_state_array[j].flag = 0;
              client_state_array[j].fd = new_s;
              j++;
              
              FD_SET(new_s, &all_sockets);
              if (new_s + 1 > FD_MAX){
                FD_MAX = new_s + 1;
              }
            } else{ //client socket
              //find the client_state for this socket in your client state
              //check which handshake to process
              for(int k = 0; k < j; k++){
                if(i == client_state_array[k].fd){
                    if(client_state_array[k].flag == 0){
                      client_state_array[k].flag = 1;
                      //pass a pointer to the client state array
                      check = handle_first_shake(&client_state_array[k]); //maybe use & for the address
                      client_state_array[k].seq_num_check = check;
                    } else if (client_state_array[k].flag == 1){
                        handle_second_shake(&client_state_array[k]); //maybe use & for the address
                        new_s = client_state_array[k].fd;
                        
                        close(new_s);
                        FD_CLR(new_s, &all_sockets);
                        
                        client_state_array[k].flag = -1;
                        client_state_array[k].fd = -1;
                        client_state_array[k].seq_num_check = -1;
                
                    }
                      
                }
              }
                //break;
            }
                                
          }
        
      }  //break;
    }
  
  }
  return 0;

}

//look up arrow notation
//look up tutorials for using structs

//look into memset to make sure there isn't junk in the buffer

int handle_first_shake(client_state* p_cs){ 
  int temp_s = p_cs->fd; 
  int len;
  char seq_num[MAX_LINE];
  char hello[] = "HELLO ";
  char msg[MAX_LINE] = "";
  memset(msg, '\0', sizeof(msg));
  const char delim[2] = " ";
  int temp;
  int check;

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
  check = atoi(seq_num);
  check++;
  strcat(msg, hello);
  strcat(msg, seq_num); 
  send(temp_s, msg, len, 0);

  memset(msg, '\0', sizeof(msg));
  //close(temp_s);

  return check;
}

void handle_second_shake(client_state* p_cs){
  int temp_s = p_cs->fd; 
  int check = p_cs->seq_num_check;

  char seq_num[MAX_LINE];
  char msg[MAX_LINE] = "";
  memset(msg, '\0', sizeof(msg));
  const char delim[2] = " ";
    
  recv(temp_s, msg, sizeof(msg), 0);
  fputs(msg, stdout);
  fflush(stdout);
  fputs("\n",stdout);
  fflush(stdout);
  strtok(msg, delim);
  strcpy(msg, strtok(NULL, delim));
  strcpy(seq_num, msg);
  if(atoi(seq_num) == check){

  }else{
    perror("Error Z: Incorrect Value");
    fflush(stdout);
  }
  memset(msg, '\0', sizeof(msg));
  //close(temp_s);
}