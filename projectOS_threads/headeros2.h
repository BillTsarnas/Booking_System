#ifndef HEADEROS2_H

#define HEADEROS2_H

#include <stdio.h> /*basic input/ouput functions in c*/
#include <stdlib.h> /*rand() and exit() are declared here*/
#include <unistd.h> /*sleep() and other standard unix functions are declared here*/
#include <sys/types.h> /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <errno.h> /* for the EINTR constant */
#include <sys/wait.h> /* for the waitpid() system call */
#include <sys/un.h> /* for Unix domain sockets */
#include <sys/ipc.h> /*many ipc constants (like shared memory mode bits, key constants, control commands..*/
#include <fcntl.h> /*semaphore creation constansts (like O_RDWR)*/
#include <time.h> //time() declaration
#include <pthread.h> //thread creation and manipulation functions

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

	struct sent{
	//this struct is used for the communication between a phone and a bank terminal thread
	//a new struct sent object is created by the phone thread, and then using pthread_create
	//a pointer to it is passed as an argument when the bank thread is created
	char zoneTEMP[15]; //the selected seat zone is passed
        int nOfTick, bal;  //the number of tickets and account balance is passed
	int client_sock;   //the socket descriptor of the client is passed
	int check_result;  //if the card check was ok, the bank process changes the value to 1. elsewhere, it remains 0 (failed transaction)
	};	


/*-----------------GLOBAL VARIABLES (visible to all the threads)--------------------------------------------*/

struct sharedInfo shm; //this holds the shared data between the threads, such as seats reserved and the seat plan of the theatre

int listenfd, connfd, *new_sock; /* Socket descriptors. */

pthread_attr_t attr; /*thread attributes*/

struct timespec tim, tim2, Tcardcheck;

int shared; //shared resourceS

int     countTHL, countBANK; //counters to simulate how many phones and bank terminals are available. they are combined
			     //with a condition variable and a mutex to simulate a posix semaphore (how it works)

pthread_cond_t myconvarTHL, myconvarBANK; //condition variable declaration

pthread_mutex_t mutexTHL, mutexBANK, mutexMEMORY; //mutex declaration

int FailedOrders, OverallOrders; //number of failed and overall orders

struct sigaction act; //this is a struct used for signal handling

/*-------------------FUNCTION DECLARATIONS----------------------------------------------------------------------*/

void sig_int( int signo );

void sig_alrm( int signo );

void message_processor(char *id, char *zone,char *numOfTickets, char *balance, char *buffer);

void card_checker(int *result, char c, int nOfTick, int bal, int socket);

void seat_allocator(char c, int ident, int nOfTick, int socket);

void *thread_functionBANK( void *infoFromParent );

void *thread_functionTHL( void *sock_desc );















#endif
