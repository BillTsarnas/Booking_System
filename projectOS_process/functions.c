#include "headeros.h"

void sig_chld( int signo )  
//This function is called every time a SIGCHLD is recieved.
//It is used to avoid the generation of "zombie" processes
{
       pid_t pid;
       int stat;

       while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 )// In the case of a terminated child, performing a waitpid 
       //allows the system to release the resources associated with the child; if a waitpid is not performed, then the terminated child remains in a "zombie" state
       //If no child has exited, return immediately (WNOHANG)
       {
              printf( "Child %d terminated.\n", pid );
       }
}

void sig_int( int signo )
//This signal handler shuts down the server.
//all the statistical data collected from the runtime is presented and clean-up operations are performed
{
	int t, ov, fail;
	double d, AVwait, AVserve;
	/*-------SEAT PLAN DISPLAY-----------------------------*/
	printf("\nZone A: \n");	
	for(t = 0; t < 100; t++) printf("%d ", shm->planA[t]);
	printf("\n");
	printf("Zone B: \n");
	for(t = 0; t < 130; t++) printf("%d ", shm->planB[t]);
	printf("\n");
	printf("Zone C: \n");
	for(t = 0; t < 180; t++) printf("%d ", shm->planC[t]);
	printf("\n");
	printf("Zone D: \n");
	for(t = 0; t < 230; t++) printf("%d ", shm->planD[t]);
	printf("\n");
	printf("A:%d B:%d C:%d D:%d\n", shm->A, shm->B, shm->C, shm->D);

	/*-------FAILED/SUCCESSFUL ORDERS----SEMAPHORE CHECK--------------------*/
	sem_getvalue(Noverall, &ov); //we convert the value of the 2 semaphores we used for process counting to integers
	sem_getvalue(Nfail, &fail);
	int temp;
	sem_getvalue(Nthl, &temp); //we convert each synchronisation semaphore to integer, in order to check its value
	printf("nthl: %d\n", temp);
	sem_getvalue(Nbank, &temp);
	printf("nbank: %d\n", temp);
	sem_getvalue(mutex, &temp);
	printf("mutex: %d\n", temp);
	//if the value of the synchronisation semaphores is the same to the initial values, then synchronization procedure... went smoothly!

	d = (double)fail/(double)ov; //the percentage is calculated here
	printf("Overall orders: %d\nFailed orders: %d\nFail percentage: %f\n", ov, fail, d*100.0);

	/*-----AVERAGE WAIT/SERVE TIME PER CUSTOMER---------------*/
	AVwait = shm->waiting/(double)ov;
	AVserve = shm->serving/(double)ov;
	printf("Customers waited to connect for (average): %f secs\nCustomer (average) serve time: %f secs\n", AVwait, AVserve);

	/*----MONEY TRANSFER TABLE-------------------------------------------------*/
	printf("Money tranfered to theatre account (every 30 secs):\n");
	for(t = 0; t < shm->nOfTrans; t++) printf("->%d\n", shm->transactions[t]);

	printf("TOTAL MONEY GATHERED: %d\n", shm->theatreMoney);
	signal( SIGINT, sig_int );
	
	/*-----CLEAN-UP CODE--------------------------------------*/	
	printf("Cya buddy!\n");
	shmdt(shm); //the server is detached from the shared memory
	shmctl(shmid, IPC_RMID, NULL); //memory segment gets deleted
	sem_unlink("thlefwnhtes"); //ALL semaphores are deleted
	sem_unlink("bank terminals");
	sem_unlink("mem_access");
	sem_unlink("all the orders");
	sem_unlink("failed orders");
	exit(0); //normal termination
}



void sig_alrm( int signo ) 
//This function is executed every 30 secs using alarm(30); 
//It performs the money transactions from the company to the theatre.
{
	//char temp[50];	
	signal(SIGALRM, sig_alrm);
	shm->theatreMoney += shm->companyMoney; //we add up the money collected so far to the theatre account
	shm->transactions[shm->nOfTrans] = shm->companyMoney; //we list the money tranferred to the theatre
	shm->nOfTrans++; //the number of transactions is increased
	shm->companyMoney = 0; //company money is reset. it is be filled again when new reservations take place
	printf("Transaction completed! Theatre account: %d\n", shm->theatreMoney);
	printf("Company account: %d\n", shm->companyMoney);
	alarm(30);
}

void sig_term( int signo )
//This function is executed each time a reservation procedure fails, due to invalid bank card.
//A SIGTERM is invoked by the bank process, which sends it to its parent(using kill(), see serveros.c and card_checker function for more info). This handler terminates
//the process (thlefwnhths) that takes the SIGTERM signal.
{
	signal(SIGTERM, sig_term);
	sem_post(Nthl); //ATTENTION! we have to increase the "thlefwnhtes" semaphore by 1, because we are going to kill (async style) a waiting
			//"thlefwnths" process. Its "spot" must be freed, for a new "thlefwnhths" process
	sem_close(Nthl); //we close all the semaphores for this instance of the server
	sem_close(Nbank);
	sem_close(mutex);
	sem_close(Noverall);
	sem_close(Nfail);
	exit(0); //normal termination
}

void sig_tstp( int signo )//When a SIGTSTP is received, we actually do nothing (valid bank card, see serveros.c for more info)
{
	signal(SIGTSTP, sig_tstp);
}

	
void message_processor(char *id, char *zone,char *numOfTickets, char *balance) 
//this function is used to collect the id, zone,
//number of tickets and account balance from the string sent by the client. Each piece of information is separated by space.
//characters are read from the buffer until we find a space character. This happens for each word
{
	int k = 0, t = 0; /*string indexing*/
	k = 0;
	while(buffer[k] != ' ') {id[k] = buffer[k]; k++;} //id  is collected
	k++; //we point to the beginning of the next substring
	while(buffer[k] != ' ') {zone[t] = buffer[k]; k++; t++;} //zone is collected
	k++;
        t = 0;
        while(buffer[k] != ' ') {numOfTickets[t] = buffer[k]; k++; t++;} //numOfTickets is collected
	k++;
	t = 0;
	while(buffer[k] != ' ') {balance[t] = buffer[k]; k++; t++;} //balance is collected
}

void seat_allocator(char c, int ident, int nOfTick) //this function is called to allocate seats (CRITICAL SECTION CODE).
						    //shm->seats must be > 0
{
	int it = 0; //index variable for our plans
	int complete = 0; //"boolean" variable that indicates a successful reservation
		
	//we linear search each plan until we find an "empty" seat. There must be enough adjacent seats if nOfTick > 1
	//if there are not enough seats in the specified zone, a proper message is sent to the client

	if(c == 'A') {
		printf("A selected!\n");	    	
		for( it = 0; it < 100; it++){
			if((shm->planA[it] == 0) && ((it + nOfTick - 1) <= 99)){
				int j;
				for(j = 0; j < nOfTick; j++) shm->planA[it + j] = ident; //the id of the client gets stored at the seat(s) he specified
											 //for details, check the report
				shm->A += nOfTick; //the number of reserved seats in zone A is increased by 1
				complete = 1; //reservation is almost complete
				break; //stop searching for seats
			}
		}
		requiredVal = 50 * nOfTick; //we set the required value for the tickets here
			    }

	if(c == 'B') {  //same goes as above, if the client has chosen B, C or D zone
		printf("B selected!\n");	    	
		for( it = 0; it < 130; it++){
			if((shm->planB[it] == 0) && ((it + nOfTick - 1) <= 129)){
						int j;
						for(j = 0; j < nOfTick; j++) shm->planB[it + j] = ident;
						shm->B += nOfTick;
						complete = 1;
						break;
			}
		}
		requiredVal = 40 * nOfTick; //we set the required value for the tickets here
			    }

	if(c == 'C') {
		printf("C selected!\n");	    	
		for( it = 0; it < 180; it++){
			if((shm->planC[it] == 0) && ((it + nOfTick - 1) <= 179)){
						int j;
						for(j = 0; j < nOfTick; j++) shm->planC[it + j] = ident;
						shm->C += nOfTick;
						complete = 1;
						break;
			}
		}
		requiredVal = 35 * nOfTick; //we set the required value for the tickets here
			    }

	if(c == 'D') {
		printf("D selected!\n");	    	
		for( it = 0; it < 230; it++){
			if((shm->planD[it] == 0) && ((it + nOfTick - 1) <= 229)){
						int j;
						for(j = 0; j < nOfTick; j++) shm->planD[it + j] = ident;
						shm->D += nOfTick;
						complete = 1;
						break;
			}
		}
		requiredVal = 30 * nOfTick; //we set the required value for the tickets here
			    }

	if(complete == 1) { //reservation complete!
		shm->companyMoney += requiredVal; //the money from the tickets purchased goes to the company's account
		shm->seats = shm->seats - nOfTick; //the overall number of seats is decreased by 1
		char str[100]; //this string is used to form properly the message to the client
		sprintf(str, "Reservation complete!\nYour id: %d\nYour seats (in the specified zone): %d to %d\nMoney paid: %d", ident, it, it + nOfTick -1, requiredVal);
		write(connfd, str, 100); //we send the message to the client
		}
	else write(connfd, "There are not enough seats in the specified zone.", 50); //this is sent to the client if there are not enough seats in the specified zone
										     //note that this function is executed when there are generally seats left in the theatre
}

void card_checker(char c, int nOfTick, int bal) 
//this function is called by the bank child process to check the card validity
//IF the card is not valid (required value > customer balance), then using kill(getppid(), SIGTERM); the parent process (thlefwnths) gets a SIGTERM and is KILLED (see sig_term handler above).
//IF the card is OK, then a signal SIGTSTP is sent to the parent process (kill(getppid(), SIGTSTP);). The handler for this signal does actually nothing, so it enables the further execution of the parent
//In every case from above, a proper message is sent to the client using write()
{
				if(c == 'A') {
				requiredVal = 50 * nOfTick; //the required value to purchase the tickets is calculated
				if(requiredVal > bal) { //card not ok
						sem_post(Nfail); //we increase the value of the semaphore for the failed orders
						write(connfd, "Your balance value can't afford the reservation!", 50); //the proper message is sent to the client
						kill(getppid(), SIGTERM);}
				else {kill(getppid(), SIGTSTP);} //card ok
				}								//same goes below!!!!!!!!!
			    if(c == 'B') {
				requiredVal = 40 * nOfTick;
				if(requiredVal > bal) {
						sem_post(Nfail); 
						write(connfd, "Your balance value can't afford the reservation!", 50); 
						kill(getppid(), SIGTERM);}
				else {kill(getppid(), SIGTSTP);}
				}
			    if(c == 'C') {
				requiredVal = 35 * nOfTick;
				if(requiredVal > bal) {
						sem_post(Nfail); 
						write(connfd, "Your balance value can't afford the reservation!", 50); 
						kill(getppid(), SIGTERM);}
				else {kill(getppid(), SIGTSTP);}
				}
			    if(c == 'D') {
				requiredVal = 30 * nOfTick;
				if(requiredVal > bal) {
						sem_post(Nfail); 
						write(connfd, "Your balance value can't afford the reservation!", 50); 
						kill(getppid(), SIGTERM);}
				else {kill(getppid(), SIGTSTP);}
				}
}
