#ifndef HEADEROS_H

#define HEADEROS_H

#include <stdio.h> /*basic input/ouput functions in c*/
#include <stdlib.h> /*rand() and exit() are declared here*/
#include <unistd.h> /*sleep() and other standard unix functions are declared here*/
#include <sys/types.h> /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <errno.h> /* for the EINTR constant */
#include <sys/wait.h> /* for the waitpid() system call */
#include <sys/un.h> /* for Unix domain sockets */
#include <sys/ipc.h> /*many ipc constants (like shared memory mode bits, key constants, control commands..*/
#include <sys/shm.h> /*shared mem functions*/
#include <semaphore.h> /*semaphore stuff*/
#include <fcntl.h> /*semaphore creation constansts (like O_RDWR)*/
#include <time.h> //time() declaration

#define LISTENQ 20 /* Size of the request queue. */
#define UNIXSTR_PATH "socketfile" /*socket file path*/


struct sharedInfo{ //we can organise the shared info (in the sh. m.) in a struct
	int companyMoney; //the amount of money in the company's account. It is transfered to the theatre's account every 30 secs
	int theatreMoney; //the amount of money in the theatre's account
	int seats; //the overall number of free seats in the theatre
	int planA[100]; //this is the seat plan for each zone. If planX[i] == 0, seat i in zone X is free, else is occupied
	int planB[130];
	int planC[180];
	int planD[230];
	int A, B, C, D; //number of reserved seats in each zone
	double waiting, serving; //these 2 variables are used in order to calculate the average wait and serve time for each customer
	int transactions[150]; //lists the transactions from the company account to the theatre account. 
	int nOfTrans; //this variable counts the transactions from the company to the theatre
};

 
/*-----------------GLOBAL VARIABLES--------------------------------------------*/
 int shmid, rc; /*shared memory id, key and pointer*/
 struct sharedInfo *shm;
 key_t key;

 sem_t *mutex, *Nthl, *Nbank, *Noverall, *Nfail; //we will use these semaphores

 int listenfd, connfd; /* Socket descriptors. */

 char buffer[50]; /*a temporary buffer that holds our message from the client*/

 int requiredVal; //money paid from the customer

 struct sigaction act; //this is a struct used for signal handling


/*-----------------FUNCTION DECLARATIONS-----------------------------------------*/
void sig_chld( int signo );

void sig_int( int signo );

void sig_alrm( int signo );

void sig_term( int signo );

void sig_tstp( int signo );

void message_processor(char *id1, char *zone1,char *numOfTickets1, char *balance1 );

void seat_allocator(char c, int ident, int nOfTick);

void card_checker(char c, int nOfTick, int bal);

#endif
