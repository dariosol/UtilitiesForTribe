#ifndef LOOPETHERNET_H
#define LOOPETHERNET_H
 int z;  
char *srvr_addr = NULL;  
struct sockaddr_in adr_inet; // AF_INET  
struct sockaddr_in adr_clnt; // AF_INET  
int len_inet;                // length  
int fromDE4;                       // Socket  
char primitive[2048];   

int z1;  
char *srvr_addr1 = NULL;  
struct sockaddr_in adr_inet1; // AF_INET  
struct sockaddr_in adr_clnt1; // AF_INET  
int len_inet1;                // length  

int numberofpackets=0;
const int maxnumberofpackets=100000000;//4.8M trigger
void hexdump(const void * buf, size_t size);
void writefile(const void * buf, size_t size,FILE* pFile);

#endif
