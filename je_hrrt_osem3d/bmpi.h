#define BMPI_NONE 0
#define BMPI_ADD 1
#define BMPI_SUB 2
#define BMPI_MUL 3
#define BMPI_DIV 4
#define BMPI_TYPE_NONE 0
#define BMPI_TYPE_FLOAT 1
#define BMPI_TYPE_INT 2
#define BMPI_TYPE_SHORT 3
#define BMPI_TYPE_DOUBLE 4
#define BMPI_TYPE_M128 5
#define BMPI_TYPE_CAST (char *)

#define BMPI_MAX_SOCKET_BUFFER_SIZE 16


#define ABORTACCEPT -1
#define CHANGECONNECTNUM -2
#define ACCEPTOK -3
#define MAXNIC 8
#define DATAPORT 5002
#define SYNCPORT 5011
#define OPTIMALTIME 1024*8*8
#define SYNCQSIZE 16

void SetMynode(int node,int total);
int BMPI_GetMynode();
int BMPI_GetTotalnode();
void BMPI_Sync();
void BMPI_Init();
void BMPI_Isend(char *buffer,int size,int dest);
void BMPI_Isend(float *buffer,int size,int dest);
void BMPI_Isend(short *buffer,int size,int dest);
void BMPI_Isend(int *buffer,int size,int dest);
void BMPI_Isend(double *buffer,int size,int dest);
void BMPI_Isend(__m128 *buffer,int size,int dest);

void BMPI_Irecv(char *buffer,int size,int src);

void BMPI_Irecv(float *buffer,int size,int src);
void BMPI_Irecv(short *buffer,int size,int src);
void BMPI_Irecv(double *buffer,int size,int src);
void BMPI_Irecv(int *buffer,int size,int src);
void BMPI_Irecv(__m128 *buffer,int size,int src);


void BMPI_Irecv(char *buffer,int size,int src,int type,int oper);
void BMPI_Irecv(__m128 *buffer,int size,int src,int type,int oper);
void BMPI_Irecv(float *buffer,int size,int src,int type,int oper);
void BMPI_Irecv(double *buffer,int size,int src,int type,int oper);
void BMPI_Irecv(int *buffer,int size,int src,int type,int oper);
void BMPI_Irecv(short *buffer,int size,int src,int type,int oper);
void BMPI_Irecvsend(char *recvbuffer,int size,int src,int dest,int type,int oper);
void BMPI_Irecvsend(float *recvbuffer,int size,int src,int dest,int type,int oper);
void BMPI_Irecvsend(double *recvbuffer,int size,int src,int dest,int type,int oper);
void BMPI_Irecvsend(short *recvbuffer,int size,int src,int dest,int type,int oper);
void BMPI_Irecvsend(int *recvbuffer,int size,int src,int dest,int type,int oper);
void BMPI_Irecvsend(__m128 *recvbuffer,int size,int src,int dest,int type,int oper);
void BMPI_Wait();
void BMPI_WaitAllRecv();
void BMPI_WaitAllSend();


