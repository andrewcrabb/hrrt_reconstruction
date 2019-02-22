// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//
//#include "mpi.h"
#include <stdio.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>
#include <xmmintrin.h>
#include "Ws2tcpip.h"
#include "Wspiapi.h"
#include "bmpi.h"

int sendlen=38*1024*1024;

	int optval=1024*8*8;
//	int optlen=4;
//	int slen=8960;
// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.
#define FUNCPTR unsigned __stdcall 
#define START_THREAD(th,func,arg,id) th = (HANDLE)_beginthreadex( NULL, 0, &func, &arg, 0, &id )
#define Wait_thread(threads) {WaitForSingleObject(threads, INFINITE );CloseHandle(threads);}
#define Term_thread(threads) {WaitForSingleObject(threads, 0 );CloseHandle(threads);}
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
typedef struct {
	SOCKET ssend;
	SOCKET srecv;
	SOCKET *sptrsrc;
	SOCKET *sptrdest;
	char *sendbuf;
	char *recvbuf;
	int n;
	int nsocksrc;
	int nsockdest;
	int src;
	int dest;
	int type;
	int oper;
	int typesize;
} COMMSTRUCT;

int mynode;
int totalnode;
int *numchannel;
int prenode,   nextnode;

HANDLE sendhandles[100];
HANDLE recvhandles[100];
int currsendhandle=0;
int currrecvhandle=0;
COMMSTRUCT sendcs[100];
COMMSTRUCT recvcs[100];

SOCKET **sd;
SOCKET listensock;
SOCKET nextnodesock,prenodesock;
HANDLE synchandle;


char **nodename;
int write_qnum=0;
int read_qnum=0;
int curr_qnum=0;
int syncq[SYNCQSIZE];

int BMPI_GetMynode()
{
	return mynode;
}
int BMPI_GetTotalnode()
{
	return totalnode;
}
void RecvQ()
{
	int rc=0;

	if(curr_qnum<0){
		LOG_EXIT("SYNCQERROR It should not be happened curr_qnum is negative!!!!!!!!!!!!\n");
	}
	rc=recv(prenodesock,(char *) &syncq[write_qnum],sizeof(int),0);
	if(rc==SOCKET_ERROR){
		printf("socketerror recvq {}",WSAGetLastError());
	}
	curr_qnum++;
	write_qnum++;
	write_qnum=write_qnum%SYNCQSIZE;
	//			initrunning==0;
	//			break;
}


FUNCPTR RecvSyncQ(void *arg)
{
	RecvQ();
	return 0;
}

void cleanup()
{
//	Term_thread(synchandle);
//	DeleteCriticalSection( (LPCRITICAL_SECTION) &criticalsec1 ); 
//	DeleteCriticalSection( (LPCRITICAL_SECTION) &criticalsec2 ); 
	WSACleanup();

	printf("cleaned\n");
}


int recvfromq()
{
	int ret;
	int arg;
	unsigned int id;

	Wait_thread(synchandle);
	if(curr_qnum<=0)
		LOG_ERROR(" Recvfromq : curr_qnum is less or equal to zero {}",curr_qnum);
	ret=syncq[read_qnum];
	syncq[read_qnum]=-5;
	read_qnum++;
	read_qnum=read_qnum%SYNCQSIZE;
	curr_qnum--;

	START_THREAD(synchandle,RecvSyncQ,arg,id);
	return ret;
}

void SetMynode(int node,int total)
{
	mynode=node;
	totalnode=total;
}

int allocSocket()
{
	int i;
	sd=(SOCKET **) calloc(totalnode,sizeof(SOCKET *));
	numchannel=(int *) calloc(totalnode,sizeof(int));
	for(i=0;i<totalnode;i++){
		sd[i]=(SOCKET *) calloc(MAXNIC,sizeof(SOCKET));
	}

	if(sd==NULL) return 0;

	return 1;
}



void SendTCP(SOCKET s,char *buf,int num)
{
	int i;
	int bytesRecv,bytesSent;
	int brecv;
	int error;
	bytesRecv=num;
	bytesSent=0;

	brecv=(num<optval)?num:optval;
	for(;;){
		i= send( s, &buf[bytesSent],brecv,0 );
		if(i>0){
			bytesRecv-=i;
			bytesSent+=i;
		} else {
			error=WSAGetLastError();
			LOG_ERROR("SendTCP Error {}",error);
			Sleep(1);
			if(error==10054) 
				LOG_EXIT("Error");
		}
		brecv=(bytesRecv<optval)?bytesRecv:optval;
		if(bytesRecv<=0)break;
	}
}

void RecvTCP(SOCKET s,char *buf,int num)
{
	int i;
	int bytesRecv,bytesSent;
	int brecv;
	int error;

	bytesRecv=num;
	bytesSent=0;

	brecv=(num<optval)?num:optval;
	for(;;){
		i= recv( s, &buf[bytesSent],brecv,0 );
		if(i>0){
			bytesRecv-=i;
			bytesSent+=i;
		} else {
			error=WSAGetLastError();
			LOG_ERROR("RecvTCP Error {}",error);
			Sleep(1);
			if(error==10054) 
				LOG_EXIT("Error");
//			if(WSAGetLastError()==10054) break;
		}
		brecv=(bytesRecv<optval)?bytesRecv:optval;
		if(bytesRecv<=0)break;
	}
}

void RecvTCPoper(SOCKET s,char *buf,int num,int type,int typesize,int oper)
{
	int i;
	int bytesRecvLeft,bytesRecv;
	int brecv;
	int error;
	float *buffer_type_float;
	double *buffer_type_double;
	short *buffer_type_short;
	int *buffer_type_int;
	__m128 *buffer_type_m128;

	float *type_float;
	double *type_double;
	short *type_short;
	int *type_int;
	__m128 *type_m128;

	char *buffer;
	int recv_left;
	int recv_element=0;
	int curr_element=0;
	bytesRecvLeft=num;
	bytesRecv=0;
	brecv=(num<optval)?num:optval;

//	optval=optval/16+16
//	buffer=(char *) calloc(optval*2,1);
	buffer=(char *) _mm_malloc((optval/BMPI_MAX_SOCKET_BUFFER_SIZE+1)*BMPI_MAX_SOCKET_BUFFER_SIZE,16);
	
	if(type==BMPI_TYPE_FLOAT) buffer_type_float=(float *) buffer;
	else if(type==BMPI_TYPE_SHORT) buffer_type_short=(short *) buffer;
	else if(type==BMPI_TYPE_DOUBLE) buffer_type_double=(double *) buffer;
	else if(type==BMPI_TYPE_INT) buffer_type_int=(int *) buffer;
	else if(type==BMPI_TYPE_M128) buffer_type_m128=(__m128 *) buffer;

	if(type==BMPI_TYPE_FLOAT) type_float=(float *) buf;
	else if(type==BMPI_TYPE_SHORT) type_short=(short *) buf;
	else if(type==BMPI_TYPE_DOUBLE) type_double=(double *) buf;
	else if(type==BMPI_TYPE_INT) type_int=(int *) buf;
	else if(type==BMPI_TYPE_M128) type_m128=(__m128 *) buf;
	

	recv_left=0;
	for(;;){
		if(oper==BMPI_NONE) i= recv( s, &buf[bytesRecv],brecv,0 );
		else	i= recv( s, &buffer[recv_left],brecv,0 );
		if(i>0){
			recv_element=(i+recv_left)/typesize;
			recv_left=(i+recv_left)%typesize;
			bytesRecvLeft-=i;
			bytesRecv+=i;
			if(type==BMPI_TYPE_M128){
				if(oper==BMPI_ADD){
					
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_add_ps(type_m128[i],buffer_type_m128[i]);
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_mul_ps(type_m128[i],buffer_type_m128[i]);
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_sub_ps(type_m128[i],buffer_type_m128[i]);
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_mul_ps(type_m128[i],buffer_type_m128[i]);
					} 
				}
			}else	if(type==BMPI_TYPE_FLOAT){
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]+=buffer_type_float[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]*=buffer_type_float[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]-=buffer_type_float[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]/=buffer_type_float[i];
					} 
				}
			} else if(type==BMPI_TYPE_SHORT){
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]+=buffer_type_short[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]*=buffer_type_short[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]-=buffer_type_short[i];
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]/=buffer_type_short[i];
					} 
				}
			} else if(type==BMPI_TYPE_DOUBLE){
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]+=buffer_type_double[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]*=buffer_type_double[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]-=buffer_type_double[i];
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]/=buffer_type_double[i];
					} 
				}
			} else if(type==BMPI_TYPE_SHORT){
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]+=buffer_type_int[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]*=buffer_type_int[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]-=buffer_type_int[i];
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]/=buffer_type_int[i];
					} 
				}
			} 
		} else {
			error=WSAGetLastError();
			LOG_ERROR("RecvTCP Error {}",error);
			Sleep(1);
			if(error==10054)
				LOG_EXIT("Error");
		}

		brecv=(bytesRecvLeft<optval)?bytesRecvLeft:optval;
		if(bytesRecvLeft<=0)break;
	}
	_mm_free(buffer);
}

void RecvSendTCPoper(COMMSTRUCT *arg)
{
	int i;
	int bytesRecvLeft,bytesRecv;
	int bytesSentLeft,bytesSent;
	int brecv;
	int error;
	float *buffer_type_float;
	double *buffer_type_double;
	short *buffer_type_short;
	int *buffer_type_int;
	__m128 *buffer_type_m128;
	float *type_float;
	double *type_double;
	short *type_short;
	int *type_int;
	__m128 *type_m128;
	char *buffer;
	int recv_left;
	int recv_element=0;
	int curr_element=0;
	int num;
	int type;
	int typesize;
	int oper;
	int bsent;

	oper=arg->oper;
	type=arg->type;
	num=arg->n;
	bytesRecvLeft=num;
	bytesSentLeft=num;
	bytesRecv=0;
	bytesSent=0;
	brecv=(num<optval)?num:optval;
	bsent=(num<optval)?num:optval;
	typesize=1;
//	optval=optval/16+16
//	buffer=(char *) calloc(optval*2,1);
	buffer=(char *) _mm_malloc((optval/BMPI_MAX_SOCKET_BUFFER_SIZE+1)*BMPI_MAX_SOCKET_BUFFER_SIZE,16);
//	buffer=(char *) _mm_malloc(optval*2,16);

	if(type==BMPI_TYPE_FLOAT){
		buffer_type_float=(float *) buffer;
		type_float=(float *) arg->recvbuf;
		typesize=4;
	} else if(type==BMPI_TYPE_SHORT){
		buffer_type_short=(short *) buffer;
		type_short=(short *) arg->recvbuf;
		typesize=2;
	}
	else if(type==BMPI_TYPE_DOUBLE){
		buffer_type_double=(double *) buffer;
		type_double=(double *) arg->recvbuf;
		typesize=8;
	}
	else if(type==BMPI_TYPE_INT){
		buffer_type_int=(int *) buffer;
		type_int=(int *) arg->recvbuf;
		typesize=4;
	}
	else if(type==BMPI_TYPE_M128){
		buffer_type_m128=(__m128 *) buffer;
		type_m128=(__m128 *) arg->recvbuf;
		typesize=16;
	}

	recv_left=0;
	for(;;){
		if(oper==BMPI_NONE) i= recv( arg->srecv, &(arg->recvbuf[bytesRecv]),brecv,0 );
		else i= recv( arg->srecv, &buffer[recv_left],brecv,0 );
		if(i>0){
			recv_element=(i+recv_left)/typesize;
			recv_left=(i+recv_left)%typesize;
			bytesRecvLeft-=i;
			bytesRecv+=i;
			if(type==BMPI_TYPE_M128){
				if(oper==BMPI_ADD){
					
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_add_ps(type_m128[i],buffer_type_m128[i]);
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_mul_ps(type_m128[i],buffer_type_m128[i]);
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_sub_ps(type_m128[i],buffer_type_m128[i]);
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_m128[curr_element]=_mm_mul_ps(type_m128[i],buffer_type_m128[i]);
					} 
				}
			}else	if(type==BMPI_TYPE_FLOAT){
				
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]+=buffer_type_float[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]*=buffer_type_float[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]-=buffer_type_float[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_float[curr_element]/=buffer_type_float[i];
					} 
				}
			} else if(type==BMPI_TYPE_SHORT){
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]+=buffer_type_short[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]*=buffer_type_short[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]-=buffer_type_short[i];
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]/=buffer_type_short[i];
					} 
				}
			} else if(type==BMPI_TYPE_DOUBLE){
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]+=buffer_type_double[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]*=buffer_type_double[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]-=buffer_type_double[i];
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_double[curr_element]/=buffer_type_double[i];
					} 
				}
			} else if(type==BMPI_TYPE_SHORT){
				if(oper==BMPI_ADD){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]+=buffer_type_int[i];
					} 
				} else	if(oper==BMPI_MUL){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]*=buffer_type_int[i];
					} 
				} else	if(oper==BMPI_SUB){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]-=buffer_type_int[i];
					} 
				} else	if(oper==BMPI_DIV){
					for(i=0;i<recv_element;i++,curr_element++){
						type_short[curr_element]/=buffer_type_int[i];
					} 
				} 
			} else {
			} 
		} else {
			error=WSAGetLastError();
			LOG_ERROR("RecvSendTCP Recv1 Error {} {}",bytesRecv,error);
			Sleep(1);
			if(error==10054)
				LOG_EXIT("Error");
		}

		brecv=(bytesRecvLeft<optval)?bytesRecvLeft:optval;
		if(bytesRecvLeft<=0)break;
		bsent=((bytesRecv-bytesSent)<optval)?(bytesRecv-bytesSent):optval;
		if(bsent<optval) continue;
		
		i=send(arg->ssend,&(arg->recvbuf[bytesSent]),bsent,0);
		if(i>0){
			bytesSentLeft-=i;
			bytesSent+=i;
		} else {
			error=WSAGetLastError();
			printf("RecvSendTCP Send1 Error {} {} {}",bytesSent,num,error);
			Sleep(1);
			if(error==10054)
				LOG_EXIT("Error");
		}
	}
	bsent=(bytesSentLeft<optval)?bytesSentLeft:optval;
	if(bytesSentLeft>0){
		for(;;){
			i= send(arg->ssend, &(arg->recvbuf[bytesSent]),bsent,0 );
			if(i>0){
				bytesSentLeft-=i;
				bytesSent+=i;
			} else {
				error=WSAGetLastError();
				LOG_INFO("RecvSendTCP Send2 Error {} {} {} {} {} ",error,bytesSent,bytesSentLeft,bsent,i);
				Sleep(1000);
				if(error==10054)
					LOG_EXIT("Error");
				LOG_EXIT("Error");
			}
			bsent=(bytesSentLeft<optval)?bytesSentLeft:optval;
			if(bytesSentLeft<=0)break;
		}
	}
	_mm_free(buffer);
}

void SetTCPWindow(SOCKET s,int val)
{
	int rc;
	rc=setsockopt(s,SOL_SOCKET,SO_SNDBUF,(char *)&val,sizeof(val));
	if(rc!=0){
		LOG_ERROR("snd Error setsock {}",WSAGetLastError());
	}
	rc=setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char *)&val,sizeof(val));
	if(rc!=0){
		LOG_ERROR("rec Error setsock {}",WSAGetLastError());
	}
	val=1;
	setsockopt(s,IPPROTO_TCP,TCP_NODELAY, (char *)&val, sizeof(val));
}

FUNCPTR SendWrapTCP(void *arg)
{
	COMMSTRUCT *parg=(COMMSTRUCT *)arg;
	SendTCP(parg->ssend,parg->sendbuf,parg->n);
	return 0;
}

FUNCPTR RecvWrapTCP(void *arg)
{
	COMMSTRUCT *parg=(COMMSTRUCT *)arg;
	if(parg->oper==0){
		RecvTCP(parg->srecv,parg->recvbuf,parg->n);
	} else {
		RecvTCPoper(parg->srecv,parg->recvbuf,parg->n,parg->type,parg->typesize,parg->oper);
	}
	return 0;
}

FUNCPTR RecvSendWrapTCP(void *arg)
{
	COMMSTRUCT *parg=(COMMSTRUCT *)arg;
	RecvSendTCPoper(parg);
	return 0;
}

FUNCPTR SendWrap(void *arg)
{
	COMMSTRUCT *parg=(COMMSTRUCT *)arg;
	int i;
	int size;
	int nsize;
	char *buf;
	SOCKET *psock;
	unsigned int id;
	int nsize2;
	COMMSTRUCT arg2[MAXNIC];
	HANDLE handles[MAXNIC];

	size=parg->n;
	buf=parg->sendbuf;
	nsize=size/BMPI_MAX_SOCKET_BUFFER_SIZE;
	nsize=(nsize/parg->nsockdest)*BMPI_MAX_SOCKET_BUFFER_SIZE;
	nsize2=size-(nsize)*(parg->nsockdest-1);
	psock=parg->sptrdest;

	for(i=0;i<parg->nsockdest;i++){
		arg2[i].sendbuf=&buf[i*nsize];
		if(i== parg->nsockdest-1) arg2[i].n=nsize2;
		else  arg2[i].n=nsize;
		arg2[i].ssend=psock[i];
		START_THREAD(handles[i],SendWrapTCP,arg2[i],id);
	}
	WaitForMultipleObjects(parg->nsockdest,handles,1,INFINITE);
	return 1;
}

FUNCPTR RecvWrap(void *arg)
{
	COMMSTRUCT *parg=(COMMSTRUCT *)arg;
	int i;
	int size;
	int nsize;
	char *buf;
	SOCKET *psock;
	unsigned int id;
	int nsize2;
	COMMSTRUCT arg2[MAXNIC];
	HANDLE handles[MAXNIC];

	size=parg->n;
	buf=parg->recvbuf;
	nsize=size/BMPI_MAX_SOCKET_BUFFER_SIZE;
	nsize=(nsize/parg->nsocksrc)*BMPI_MAX_SOCKET_BUFFER_SIZE;
	nsize2=size-(nsize)*(parg->nsocksrc-1);
	psock=parg->sptrsrc;

	for(i=0;i<parg->nsocksrc;i++){
		arg2[i].recvbuf=&buf[i*nsize];
		if(i== parg->nsocksrc-1) arg2[i].n=nsize2;
		else  arg2[i].n=nsize;
		arg2[i].srecv=psock[i];
		arg2[i].oper=parg->oper;
		arg2[i].type=parg->type;
		arg2[i].typesize=parg->typesize;
		START_THREAD(handles[i],RecvWrapTCP,arg2[i],id);
	}
	WaitForMultipleObjects(parg->nsocksrc,handles,1,INFINITE);
	return 1;
}

FUNCPTR RecvSendWrap(void *arg)
{
	COMMSTRUCT *parg=(COMMSTRUCT *)arg;
	int i;
	int size;
	int nsize;
	char *recvbuf;
	SOCKET *psocksend;
	SOCKET *psockrecv;
	unsigned int id;
	int nsize2;
	COMMSTRUCT arg2[MAXNIC];
	HANDLE handles[MAXNIC];

	
	size=parg->n;
	recvbuf=parg->recvbuf;

	nsize=size/BMPI_MAX_SOCKET_BUFFER_SIZE;
	nsize=(nsize/parg->nsocksrc)*BMPI_MAX_SOCKET_BUFFER_SIZE;
	nsize2=size-(nsize)*(parg->nsocksrc-1);
	psockrecv=parg->sptrsrc;
	psocksend=parg->sptrdest;

	for(i=0;i<parg->nsocksrc;i++){
		arg2[i]=*parg;
		arg2[i].recvbuf=&recvbuf[i*nsize];
		if(i== parg->nsocksrc-1) arg2[i].n=nsize2;
		else  arg2[i].n=nsize;
		arg2[i].srecv=psockrecv[i];
		arg2[i].ssend=psocksend[i];

		START_THREAD(handles[i],RecvSendWrapTCP,arg2[i],id);
	}
	WaitForMultipleObjects(parg->nsocksrc,handles,1,INFINITE);
	return 1;
}

void Init_winsock()
{
    WSADATA wsaData;
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if ( iResult != NO_ERROR ){
		LOG_ERROR(" at WSAStartup()\n");
		LOG_EXIT("Error");
	}
}

int CreateSocket(int node,int num)
{
	int i;

//	sd[node]=(SOCKET *) calloc(num,sizeof(SOCKET));
	for(i=0;i<num;i++){
		sd[node][i] = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if ( sd[node][i] == INVALID_SOCKET ) {
			LOG_ERROR( "Error at socket(): {}", WSAGetLastError() );
			WSACleanup();
			return 0;
		}
	}
	return 1;
}

int	CreateSocket(SOCKET *sock)
{
	*sock= socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( *sock == INVALID_SOCKET ) {
		LOG_ERROR("at socket(): {}", WSAGetLastError() );
		WSACleanup();
		return 0;
	}
	return 1;
}

int CreateSocketBindListen(SOCKET *sock,int port)
{
	int i;
	sockaddr_in service;
	i=0;
	*sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(*sock==INVALID_SOCKET){
		LOG_ERROR("at socket(): {}", WSAGetLastError() );
		WSACleanup();
		return 0;
	}
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = 0;//inet_addr(ip);
	service.sin_port = htons(port);
	if ( bind( *sock, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR ) {
		LOG_ERROR( "bind() failed. {}", WSAGetLastError() );
		closesocket(*sock);
		return 0;
	}
	if ( listen( *sock, MAXNIC*totalnode ) == SOCKET_ERROR ){
		LOG_ERROR("listening on socket.\n");
		return 0;
	}

	return 1;
}

int ConnectToHostByMaxNIC(int destnode,int port)
{
	int i,j;
	char str[100];
	int numofnic;
	int numconn;
	int numpossconn;
	int tmp,tmp2,rc;
	unsigned long ipaddrhost[100];
	unsigned long ipaddrhostcandidate[100];
	unsigned long ipaddrlocal[100];
	SOCKET *sock;
	sockaddr_in service;

	hostent* thisHost;
	in_addr *pinAddr;

	numchannel[destnode]=0;

	gethostname(str,100);
	thisHost = gethostbyname(str);
	for(numofnic=0;;numofnic++){
		pinAddr=((in_addr *)thisHost->h_addr_list[numofnic]);
		if(pinAddr==NULL) break;
		LOG_INFO("ip address {} {}",inet_ntoa(*pinAddr),inet_addr(inet_ntoa(*pinAddr)));
		ipaddrlocal[numofnic]=inet_addr(inet_ntoa(*pinAddr));
	}
	thisHost=gethostbyname(nodename[destnode]);
	for(numconn=0;;numconn++){
		pinAddr=((in_addr *)thisHost->h_addr_list[numconn]);
		if(pinAddr==NULL) break;
		ipaddrhost[numconn]=inet_addr(inet_ntoa(*pinAddr));
	}

	numpossconn=0;
	for(i=0;i<numofnic;i++){
		for(j=0;j<numconn;j++){
			if((ipaddrlocal[i]&0x00ffffff) == (ipaddrhost[j]&0x00ffffff)){
				ipaddrhostcandidate[numpossconn]=ipaddrhost[j];
				numpossconn++;
			} else {
			}
		}
	}
	
	j=0;
	CreateSocket(destnode,numpossconn);
	sock=sd[destnode];

	for(i=0;i<numpossconn;i++){	
	    service.sin_family = AF_INET;
		service.sin_addr.s_addr = ipaddrhostcandidate[j];
	    service.sin_port = htons( port);
		if (connect( sock[0], (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR) {
			LOG_ERROR( "Failed to connect. {} {} {}",i,ipaddrhostcandidate[0],j);
			LOG_ERROR("error {}",WSAGetLastError());
		} else {
			SetTCPWindow(sock[0],optval);
			break;
		}
	}
	tmp2=4;
	rc=getsockopt(sock[0],SOL_SOCKET,SO_SNDBUF,(char *)&tmp,&tmp2);
	if(tmp!=optval){
		LOG_ERROR(" value {} {}",tmp,optval,rc,WSAGetLastError());
	}

	numpossconn=numpossconn-i;
//	LOG_ERROR("connect 1 {} {}",i,numpossconn);
	if(numpossconn==0) {
		LOG_ERROR(" total num of connections {}",j);	
		return 0;
	} else {
		send(sock[0],(char *) &mynode,sizeof(mynode),0);
		recv(sock[0],(char *) &tmp,sizeof(tmp),0);
		if(tmp==ACCEPTOK) send(sock[0],(char *) &numpossconn,sizeof(numpossconn),0);
		else if(tmp==ABORTACCEPT){
			closesocket(sock[0]);
			return 0;
		}
	}
	j++;

	for(i=1;i<numpossconn;i++){
		service.sin_addr.s_addr = ipaddrhostcandidate[j];
		if (connect( sock[j], (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR) {
			LOG_ERROR( "Failed to connect. {} %x {}",i,ipaddrhostcandidate[j],j);
			tmp=CHANGECONNECTNUM;
			send(sock[0],(char *)&tmp,sizeof(tmp),0);
		} else {
			send(sock[j],(char *) &mynode,sizeof(mynode),0);
			recv(sock[j],(char *) &tmp,sizeof(tmp),0);
			if(tmp==ACCEPTOK){
				send(sock[0],(char *)&tmp,sizeof(tmp),0);
				SetTCPWindow(sock[j],optval);
				rc=getsockopt(sock[j],SOL_SOCKET,SO_SNDBUF,(char *)&tmp,&tmp2);
				if(tmp!=optval){
					LOG_ERROR(" value {} {}",tmp,optval,rc,WSAGetLastError());
				}
				j++;
			}
			else if(tmp==ABORTACCEPT){
				LOG_ERROR("connection nic {}/{}",i,numpossconn);
				closesocket(sock[j]);
				return 0;
			}
		}
	}
	numchannel[destnode]=j;

	LOG_INFO("total num of connections {}",j);
	return 1;
}



int ConnectToHostByOneNIC(SOCKET *sock,int destnode,int port)
{
	int j;
	int tmp;
	unsigned long ipaddrhost;
	sockaddr_in service;
	hostent* thisHost;
	in_addr *pinAddr;

	thisHost = gethostbyname(nodename[destnode]);
	pinAddr=((in_addr *)thisHost->h_addr_list[0]);
	ipaddrhost=inet_addr(inet_ntoa(*pinAddr));

	j=0;

    service.sin_family = AF_INET;
	service.sin_addr.s_addr = ipaddrhost;
    service.sin_port = htons( port);
	if (connect( *sock, (SOCKADDR*) &service, sizeof(service) ) == SOCKET_ERROR) {
		LOG_ERROR(" to One connect (node: {}). {}",destnode,ipaddrhost);
		LOG_ERROR("{}",WSAGetLastError());
	} else {
		SetTCPWindow(*sock,optval);
//		break;
	}

	send(*sock,(char *) &mynode,sizeof(mynode),0);
	recv(*sock,(char *) &tmp,sizeof(tmp),0);
	if(tmp==ABORTACCEPT){
		closesocket(*sock);
	//	free(thisHost);
		return 0;
	} else if(tmp!=ACCEPTOK){
	//	free(thisHost);
		return 0;
	}

	//free(thisHost);
	return 1;
}


typedef struct {
	SOCKET *asock;
	SOCKET lsock;
	int node;
	int nic;
} AcceptArg;

int AcceptFromClientbyOneNIC(SOCKET *sock,SOCKET listensock,int srcnode)
{
	int tmp;

	*sock=accept(listensock,NULL,NULL);
	recv(*sock,(char *) &tmp,sizeof(tmp),0);
	if(tmp!=srcnode){
//		LOG_ERROR("syncport accpet : it is supposed to be node %d, but it is came from node {}",srcnode,tmp);
		tmp=ABORTACCEPT;
		send(*sock,(char *) &tmp,sizeof(tmp),0);
		closesocket(*sock);
		return 0;
	} else {
		tmp=ACCEPTOK;
		send(*sock,(char *) &tmp,sizeof(tmp),0);
		SetTCPWindow(*sock,optval);
	}
	return 1;
}

FUNCPTR AcceptWrap(void *arg)
{
	AcceptArg *parg=(AcceptArg *) arg;
	int tmp;

	*(parg->asock)=accept(parg->lsock,NULL,NULL);
	recv(*(parg->asock),(char *) &tmp,sizeof(tmp),0);
	if(tmp!=parg->node){
	//	LOG_ERROR("accpet nic {} : it is supposed to be node %d, but it is came from node {}",parg->nic,parg->node,tmp);
		tmp=ABORTACCEPT;
		send(*(parg->asock),(char *) &tmp,sizeof(tmp),0);
		closesocket(*(parg->asock));
		return 0;
	} else {
		tmp=ACCEPTOK;
		send(*(parg->asock),(char *) &tmp,sizeof(tmp),0);
		SetTCPWindow(*(parg->asock),optval);
	}
	return 1;
}

int AcceptFromClient(SOCKET listensock,int node)
{
	int i;
	int numposscon=MAXNIC;
	sockaddr addr;
	int tmp;
	HANDLE handle;
	unsigned int id;
	AcceptArg acceptarg;
	int tmp2;
	int quitflag;
	SOCKET *asocket;


	tmp=sizeof(addr);


//	CreateSocket(node,MAXNIC);
//	CreateSocket(node,3);
	asocket=sd[node];

	acceptarg.lsock=listensock;
	acceptarg.node=node;
	acceptarg.asock=&asocket[0];
	acceptarg.nic=0;

	if(!AcceptWrap(&acceptarg)){
		return 0;
	}
	recv(asocket[0],(char *)&numposscon,sizeof(numposscon),0);
	numchannel[node]=1;

	quitflag=0;
	

	for(i=1;i<numposscon;i++){
		acceptarg.asock=&asocket[i];
		acceptarg.nic=i;
		START_THREAD(handle,AcceptWrap,acceptarg,id);
		tmp=0;
		while(1){
			tmp2=recv(asocket[0],(char *) &tmp,sizeof(tmp),0);
			if(tmp2<=0){
				Sleep(1);
			} else {
				if(tmp==ABORTACCEPT) {
					Term_thread(handle);
					numposscon=1;
					break;
				} else if(tmp==CHANGECONNECTNUM){
					numposscon--;
				} else if(tmp==ACCEPTOK){
					Wait_thread(handle);
					numchannel[node]++;
					break;
				}
			}
		}
	}

	return 1;
}


void BMPI_Sync2()
{
	int i;
	clock_t c1,c2;

	c1=clock();
	if(mynode==0) {
		i=send(sd[1][0],(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("send sync error {} {}",i,WSAGetLastError());
		}
		i=recv(sd[totalnode-1][0],(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("recv sync error {} {}",i,WSAGetLastError());
		}
	} else {
		i=recv(sd[mynode-1][0],(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("recv sync error {} {}",i,WSAGetLastError());
		}
		if(mynode!=totalnode-1) i=send(sd[mynode+1][0],(char *) &i,sizeof(i),0);
		else i=send(sd[0][0],(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("send sync error {} {}",i,WSAGetLastError());
		}
	}
	c2=clock();
	printf("BMPI_sysc() time %f\n",(c2-c1+0.0) /CLOCKS_PER_SEC);

}

void BMPI_Sync3()
{
	int i;
	clock_t c1,c2;

	c1=clock();
	if(mynode==0) {
		i=send(nextnodesock,(char *) &mynode,sizeof(i),0);
		if(i<=0) {
			printf("send sync error {} {}",i,WSAGetLastError());
		}
		i=recvfromq();
		if(i!=(mynode-1+totalnode)%totalnode) {
			printf("recv sync error {} {}",i,mynode);
		}
		i=send(nextnodesock,(char *) &mynode,sizeof(i),0);
		i=recvfromq();
	} else {
//		i=recv(prenodesock,(char *) &i,sizeof(i),0);
		i=recvfromq();
	//	printf("it is from  node {}",i);
		if(i!=(mynode-1+totalnode)%totalnode) {
			printf("recv sync error {} {}",i,mynode);
		}
	//	printf("before sending to nextnode\n");
		i=send(nextnodesock,(char *) &mynode,sizeof(i),0);
//		Sleep(100);
	//	printf("after sending to nextnode\n");
		if(i<=0) {
			printf("send sync error {} {}",i,WSAGetLastError());
		}
		i=recvfromq();
		i=send(nextnodesock,(char *) &mynode,sizeof(i),0);
	}
	c2=clock();
//	printf("BMPI_sysc3() time %f\n",(c2-c1+0.0) /CLOCKS_PER_SEC);

}

void BMPI_Sync()
{
	int i;
	clock_t c1,c2;

	c1=clock();
	if(mynode==0) {
		i=send(nextnodesock,(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("send sync error {} {}",i,WSAGetLastError());
		}
		i=recv(prenodesock,(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("recv sync error {} {}",i,WSAGetLastError());
		}
	} else {
		i=recv(prenodesock,(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("recv sync error {} {}",i,WSAGetLastError());
		}
		i=send(nextnodesock,(char *) &i,sizeof(i),0);
		if(i<=0) {
			printf("send sync error {} {}",i,WSAGetLastError());
		}
	}
	c2=clock();
//	printf("BMPI_sysc3() time %f\n",(c2-c1+0.0) /CLOCKS_PER_SEC);

}

void GenerateSyncSocket()
{
	SOCKET listensock;

	CreateSocket(&nextnodesock);
	CreateSocket(&prenodesock);

	CreateSocketBindListen(&listensock,SYNCPORT);


	if(mynode==0){
		ConnectToHostByOneNIC(&nextnodesock,mynode+1,SYNCPORT);
		AcceptFromClientbyOneNIC(&prenodesock,listensock,totalnode-1);
	} else if(mynode!=totalnode-1) {
		AcceptFromClientbyOneNIC(&prenodesock,listensock,mynode-1);
		ConnectToHostByOneNIC(&nextnodesock,mynode+1,SYNCPORT);
	} else if(mynode==totalnode-1){
		AcceptFromClientbyOneNIC(&prenodesock,listensock,mynode-1);
		ConnectToHostByOneNIC(&nextnodesock,0,SYNCPORT);
	} else {
		LOG_ERROR(" mynode {} {}",mynode,totalnode);
		LOG_EXIT("Error");
	}

	closesocket(listensock);
}

void BMPI_Init(int argc,char **argv)
{
	int numsock=1;
	int node;
	int lastnode;
	SOCKET listensock;
	char str[100];
	int i;
	char *configfile="z:\\machines.txt";
	unsigned int id;
	int arg;

	FILE *fp=fopen(configfile,"rt");
	if(fp==NULL) LOG_ERROR("reading file {}",configfile);
	fscanf(fp,"%d",&i);
	totalnode=i;
	nodename=(char **) calloc(totalnode,sizeof(char *));
	for(i=0;i<totalnode;i++){
		nodename[i]=(char *) calloc(100,sizeof(char));
		fscanf(fp,"%s",nodename[i]);
		LOG_INFO("nodename {} {}",i,nodename[i]);
	}
	fclose(fp);
//	atexit(cleanup);
	Init_winsock();

	gethostname(str,100);
	LOG_INFO("str {}",str);

	for(i=0;i<totalnode;i++){
		if(!strcmp(str,nodename[i])) break;
	}
	mynode=i;
	
	if(mynode==0) {
		prenode=totalnode-1;
	} else prenode=mynode-1;

	if(mynode==totalnode-1){
		nextnode=0;
	} else {
		nextnode=mynode+1;
	}
	LOG_INFO("mynode {}",mynode);


	lastnode=totalnode-1;

	allocSocket();

    // Create a socket.
	GenerateSyncSocket();
	CreateSocketBindListen(&listensock,DATAPORT);

	for(node=0;node<mynode;node++){
		LOG_INFO("accept from {} {}",node,mynode);
		AcceptFromClient(listensock,node);
	}
	
	if(mynode!=0) recv(prenodesock,(char *)&i,sizeof(i),0);

	for(node=mynode+1;node<totalnode;node++){
		printf("connect to {} {} {}",node,mynode,nodename[node]);
		ConnectToHostByMaxNIC(node,DATAPORT);
	}
//	if(mynode==0) Sleep(1500);
	LOG_INFO("i={}",i);
	if(mynode!=totalnode-1) send(nextnodesock,(char *)&i,sizeof(i),0);
	closesocket(listensock);

//	BMPI_Sync();

	int tmp,tmp2=4,rc;
	for(node=0;node<totalnode;node++){
		if(node==mynode) continue;
		for(i=0;i<numchannel[node];i++){
			tmp=0;
			rc=getsockopt(sd[node][i],SOL_SOCKET,SO_SNDBUF,(char *)&tmp,&tmp2);
			if(tmp!=optval || rc<0){
				LOG_ERROR(" value {} {}",tmp,optval,rc,WSAGetLastError());
			}
		}
	}
//	Sleep(500);
	START_THREAD(synchandle,RecvSyncQ,arg,id);
	BMPI_Sync3();
	BMPI_Sync3();
	BMPI_Sync3();
	BMPI_Sync3();
	BMPI_Sync3();
	BMPI_Sync3();
	BMPI_Sync3();
	BMPI_Sync3();
//	Sleep(1000);
//	LOG_EXIT("Error");
}



void BMPI_Isend(char *buffer,int size,int dest)
{
	unsigned int id;

	if(dest==mynode) return;
	if(dest>=totalnode) return;

//	printf("currsendhandle {}",currsendhandle);
	sendcs[currsendhandle].sendbuf=buffer;
	sendcs[currsendhandle].n=size;
	sendcs[currsendhandle].nsockdest=numchannel[dest];
	sendcs[currsendhandle].sptrdest=sd[dest];
	sendcs[currsendhandle].dest=dest;
	
	START_THREAD(sendhandles[currsendhandle],SendWrap,sendcs[currsendhandle],id);

	currsendhandle++;
//	printf("currsendhandle {}",currsendhandle);

	return;
}
void BMPI_Isend(float *buffer,int size,int dest){
	BMPI_Isend(BMPI_TYPE_CAST buffer, size,dest);
}
void BMPI_Isend(short *buffer,int size,int dest){
	BMPI_Isend(BMPI_TYPE_CAST buffer, size,dest);
}
void BMPI_Isend(int *buffer,int size,int dest){
	BMPI_Isend(BMPI_TYPE_CAST buffer, size,dest);
}
void BMPI_Isend(double *buffer,int size,int dest){
	BMPI_Isend(BMPI_TYPE_CAST buffer, size,dest);
}
void BMPI_Isend(__m128 *buffer,int size,int dest){
	BMPI_Isend(BMPI_TYPE_CAST buffer, size,dest);
}

void BMPI_Irecv(char *buffer,int size,int src)
{
	unsigned int id;

	if(src==mynode) return;
	if(src>=totalnode) return;

	recvcs[currrecvhandle].recvbuf=buffer;
	recvcs[currrecvhandle].sendbuf=NULL;
	recvcs[currrecvhandle].n=size;
	recvcs[currrecvhandle].nsocksrc=numchannel[src];
	recvcs[currrecvhandle].sptrsrc=sd[src];
	recvcs[currrecvhandle].src=src;
	recvcs[currrecvhandle].oper=0;

	START_THREAD(recvhandles[currrecvhandle],RecvWrap,recvcs[currrecvhandle],id);

	currrecvhandle++;

	return;

}

void BMPI_Irecv(float *buffer,int size,int src)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src);
}
void BMPI_Irecv(short *buffer,int size,int src)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src);
}
void BMPI_Irecv(double *buffer,int size,int src)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src);
}
void BMPI_Irecv(int *buffer,int size,int src)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src);
}
void BMPI_Irecv(__m128 *buffer,int size,int src)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src);
}


void BMPI_Irecv(char *buffer,int size,int src,int type,int oper)
{
	unsigned int id;
	int typesize;

	if(src==mynode) return;
	if(src>=totalnode) return;

	recvcs[currrecvhandle].recvbuf=(char *) buffer;
	recvcs[currrecvhandle].nsocksrc=numchannel[src];
	recvcs[currrecvhandle].sptrsrc=sd[src];
	recvcs[currrecvhandle].src=src;

	if(type==BMPI_TYPE_NONE) typesize=1;
	else if(type==BMPI_TYPE_FLOAT) typesize=4;
	else if(type==BMPI_TYPE_INT) typesize=4;
	else if(type==BMPI_TYPE_SHORT) typesize=2;
	else if(type==BMPI_TYPE_DOUBLE) typesize=8;
	else if(type==BMPI_TYPE_M128) typesize=16;

	recvcs[currrecvhandle].n=size*typesize;
//	recvcs[currrecvhandle].n=size;
	recvcs[currrecvhandle].oper=oper;
	recvcs[currrecvhandle].type=type;
	recvcs[currrecvhandle].typesize=typesize;

	START_THREAD(recvhandles[currrecvhandle],RecvWrap,recvcs[currrecvhandle],id);

	currrecvhandle++;

	return;

}

void BMPI_Irecv(__m128 *buffer,int size,int src,int type,int oper)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src,type,oper);
}

void BMPI_Irecv(float *buffer,int size,int src,int type,int oper)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src,type,oper);
}

void BMPI_Irecv(double *buffer,int size,int src,int type,int oper)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src,type,oper);
}
void BMPI_Irecv(int *buffer,int size,int src,int type,int oper)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src,type,oper);
}

void BMPI_Irecv(short *buffer,int size,int src,int type,int oper)
{
	BMPI_Irecv(BMPI_TYPE_CAST buffer,size,src,type,oper);
}


void BMPI_Irecvsend(char *recvbuffer,int size,int src,int dest,int type,int oper)
{
	unsigned int id;
	int typesize;

	if(src==mynode) return;
	if(src>=totalnode) return;

	recvcs[currrecvhandle].recvbuf=(char *) recvbuffer;	
	recvcs[currrecvhandle].sendbuf=NULL;
	recvcs[currrecvhandle].nsocksrc=numchannel[src];
	recvcs[currrecvhandle].nsockdest=numchannel[dest];
	recvcs[currrecvhandle].sptrsrc=sd[src];
	recvcs[currrecvhandle].sptrdest=sd[dest];
	recvcs[currrecvhandle].src=src;
	recvcs[currrecvhandle].dest=dest;

	if(type==BMPI_TYPE_NONE) typesize=1;
	else if(type==BMPI_TYPE_FLOAT) typesize=4;
	else if(type==BMPI_TYPE_INT) typesize=4;
	else if(type==BMPI_TYPE_SHORT) typesize=2;
	else if(type==BMPI_TYPE_DOUBLE) typesize=8;
	else if(type==BMPI_TYPE_M128) typesize=16;

	recvcs[currrecvhandle].n=size*typesize;
	recvcs[currrecvhandle].oper=oper;
	recvcs[currrecvhandle].type=type;
	recvcs[currrecvhandle].typesize=typesize;

	START_THREAD(recvhandles[currrecvhandle],RecvSendWrap,recvcs[currrecvhandle],id);

	currrecvhandle++;

	return;
}
void BMPI_Irecvsend(float *recvbuffer,int size,int src,int dest,int type,int oper)
{
	BMPI_Irecvsend(BMPI_TYPE_CAST recvbuffer,size,src,dest,type,oper);
}
void BMPI_Irecvsend(double *recvbuffer,int size,int src,int dest,int type,int oper)
{
	BMPI_Irecvsend(BMPI_TYPE_CAST recvbuffer,size,src,dest,type,oper);
}
void BMPI_Irecvsend(short *recvbuffer,int size,int src,int dest,int type,int oper)
{
	BMPI_Irecvsend(BMPI_TYPE_CAST recvbuffer,size,src,dest,type,oper);
}
void BMPI_Irecvsend(int *recvbuffer,int size,int src,int dest,int type,int oper)
{
	BMPI_Irecvsend(BMPI_TYPE_CAST recvbuffer,size,src,dest,type,oper);
}
void BMPI_Irecvsend(__m128 *recvbuffer,int size,int src,int dest,int type,int oper)
{
	BMPI_Irecvsend(BMPI_TYPE_CAST recvbuffer,size,src,dest,type,oper);
}

void BMPI_Wait()
{
	clock_t c1,c2;
//	for(i=0;i<currrecvhandle;i++){Wait_thread(recvhandles[i]);}
//	for(i=0;i<currsendhandle;i++){Wait_thread(sendhandles[i]);}
	c1=clock();

	WaitForMultipleObjects(currrecvhandle,recvhandles,1,INFINITE);
	WaitForMultipleObjects(currsendhandle,sendhandles,1,INFINITE);
	currrecvhandle=0;
	currsendhandle=0;
	c2=clock();

}

void BMPI_WaitAllRecv()
{
//	clock_t c1,c2;

//	for(i=0;i<currrecvhandle;i++){Wait_thread(recvhandles[i]);}
	WaitForMultipleObjects(currrecvhandle,recvhandles,1,INFINITE);
	currrecvhandle=0;
}

void BMPI_WaitAllSend()
{
//	int i;
//	for(i=0;i<currsendhandle;i++){Wait_thread(sendhandles[i]);}
	WaitForMultipleObjects(currsendhandle,sendhandles,1,INFINITE);
	currsendhandle=0;
}


