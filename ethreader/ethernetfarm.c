
//Read data sent by L0TP to PC-Farm.
//They are stored in data.bin
//To compile:
//g++ -o ethernetfarm ethernetfarm.c

#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
 
#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<stdio.h> //For standard things
#include<stdlib.h>    //malloc
#include<string.h>    //strlen
#include <fstream>      // std::ofstream
#include <iostream>

#include "loopethernet.h"

using namespace std;

static void displayError(const char *on_what) {  
  fputs(strerror(errno),stderr);  
  fputs(": ",stderr);  
  fputs(on_what,stderr);  
  fputc('\n',stderr);  
  exit(1);  
}  
  
int main(int argc,char **argv) {  
   
  ofstream myFile ("dump.bin", ios::out | ios::binary);

 
  /* 
   * Create a UDP socket to use: 
   */  

  fromDE4 = socket(AF_INET,SOCK_DGRAM,0);  

  cout<<"Connected with socket? "<<fromDE4<<endl;

  if ( fromDE4 == -1 ) {  
    displayError("socket(1)");  
  }  
  /*---RECEIVING FROM DE4------------*/  
  memset(&adr_inet,0,sizeof adr_inet);  
  adr_inet.sin_family = AF_INET;
  adr_inet.sin_port = htons(58914);
  //  adr_inet.sin_port = htons(56016);  
  adr_inet.sin_addr.s_addr =  inet_addr("192.168.1.20");  
   
  if ( adr_inet.sin_addr.s_addr == INADDR_NONE ) {  
    displayError("bad address.");  
  }  
  len_inet = sizeof adr_inet;  
  
  
  z = bind(fromDE4, (struct sockaddr *)&adr_inet, len_inet);  
  if ( z == -1 ) {  
    displayError("bind(1)");  
  }  
     
  printf("waiting packets \n"); 
  /*******************RECEIVING FROM DE4***********************/ 
  while(1) {   
    z = recvfrom(fromDE4,            
		 primitive,        
		 sizeof primitive, // Max recv buf size   */
		 0,            
		 (struct sockaddr *)&adr_clnt, 
		 (socklen_t*)&adr_clnt);  

    if ( z < 0 ) displayError("recvfrom(2)");      
	
    myFile.write (primitive, z);

    numberofpackets+=1;
    if(numberofpackets%10000==0) printf("\n PACKET RECEIVED: %d, size %d\n", numberofpackets,z);
    if(numberofpackets > maxnumberofpackets) break;
  }
  close(fromDE4);
  myFile.close();
  cout<<"End acquisition"<<endl;
  return 0;  
}  
