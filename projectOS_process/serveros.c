#include "headeros.h" //this header includes all the declarations of the functions used by the server
			//it also includes some global variables used


int main( int argc, char **argv )
{ 
	
	/*--------------SHARED MEMORY SETUP-------------------------------------------------------------------*/
      
	key = 5678; //setting the key for our sahred memory. All the children of the server will have an identical copy of this variable
		    //so they will be able to access the sh. m. without a problem...

        if ((shmid = shmget(key, sizeof(struct sharedInfo), IPC_CREAT | 0666)) < 0){ //the sh m seg is created here
		perror("shmget");
		exit(1);
 	}

        if ((shm = shmat(shmid, NULL, 0)) == (struct sharedInfo *) -1){ //the server gets attached to the segment
		perror("shmat");
		exit(1);
	}

        shm->companyMoney = 0;  //we start inserting data into the segment
        shm->theatreMoney = 0;
        shm->seats = 640; //number of free seats
        int i;
        for(i = 0; i < 100; i++) shm->planA[i] = 0; //all seats in every zone are empty
        for(i = 0; i < 130; i++) shm->planB[i] = 0;
        for(i = 0; i < 180; i++) shm->planC[i] = 0;
        for(i = 0; i < 230; i++) shm->planD[i] = 0;
      
        shm->A = 0; shm->B = 0; shm->C = 0; shm->D = 0; //number of reserved seats in each zone
        shm->waiting = (double)0; //we reset our time variables. They will be used to calculate average times
        shm->serving = (double)0;

        for(i = 0; i < 150; i++) shm->transactions[i] = 0; //we initialize with 0 our transactions
        shm->nOfTrans = 0; //number of money transactions

	
	/*--------------SEMAPHORE SETUP------------------------------------------------------------------------*/
	/*here we create and set a starting value to our semaphores (user create, read and write)*/
        mutex = sem_open("mem_access", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1); //memory mutex
        Nthl =  sem_open("thlefwnhtes", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 10); //semaphore to simulate theatre's phone system
        Nbank = sem_open("bank terminals", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 4); //semaphore to simulate bank terminals
        Noverall = sem_open("all the orders", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0); //semaphore to count the number of all the orders
        Nfail = sem_open("failed orders", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);//semaphore to count the number of failed orders
      
	/*--------------SIGNAL SETUP--------------------------------------------------------------------------*/

        pid_t childpid; //pid of the theatre phone - thlefwnhths (child process)

	//here we set the values for act structure, declared in the header file. This structure is needed to call
	//sigaction(SIGALRM, &act, NULL), which is similar to signal() function.
	//sigaction will be used to handle only the SIGALRM signal	
	act.sa_handler = &sig_alrm;  //we set the handler to be sig_alrm function. Take a look at functions.c
	sigemptyset(&(act.sa_mask)); //(act.sa_mask = 0;) this is used to block other signals that can be raised by other processes. We won't need this.
	act.sa_flags = SA_RESTART;  // Restart interrupted system calls. This enables the periodical activation of the signal using alarm()
      
	signal( SIGCHLD, sig_chld ); /* Avoid "zombie" process generation. */
        signal( SIGINT, sig_int ); /* Clear-up code and termination using Ctrl-C is enabled*/
        signal( SIGTERM, sig_term ); //used for bank - theatre communication
        signal(SIGTSTP, sig_tstp); //used for bank - theatre communication
        sigaction(SIGALRM, &act, NULL);  //SIGALRM will be used for the money transactions

	/*--------------SOCKET SETUP--------------------------------------------------------------------------*/        

	socklen_t clilen;
        struct sockaddr_un cliaddr, servaddr; /* Structs for the client and server socket addresses. */	
	listenfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); /* Create the server's endpoint */

	/* if there is in the disk a socketfile with the same name (from a previous execution of the server) it will get deleted */
        unlink( UNIXSTR_PATH ); /* Remove any previous socket with the same filename. */

        bzero( &servaddr, sizeof( servaddr ) ); /* Zero all fields of servaddr. */
        servaddr.sun_family = AF_LOCAL; /* Socket type is local (Unix Domain). */
        strcpy( servaddr.sun_path, UNIXSTR_PATH ); /* Define the name of this socket. */

	/* Create the file for the socket and register it as a socket. */
        bind( listenfd, ( struct sockaddr* ) &servaddr, sizeof( servaddr ) );

        listen( listenfd, LISTENQ ); /* Create request queue of size LISTENQ. */
	alarm(30); /*from now on, every 30 secs a transaction from the company to the theatre happens*/

        for ( ; ; ) { //this infinite for loop is used by the server to constantly accept new  client requests from the listen queue!!
        	
	      clilen = sizeof( cliaddr ); //we will need this to accept a request, see below

              connfd = accept( listenfd, ( struct sockaddr * ) &cliaddr, &clilen ); /* Copy next request from the queue to connfd and remove it from the queue. */

              if ( connfd < 0 ) { //request accept failure!
                     if ( errno == EINTR ) /* Something interrupted us. */
                            continue; /* Back to for()... */
                     else {
                            fprintf( stderr, "Accept Error\n" );
                            exit( 0 );
                     }
              }

              childpid = fork(); /* Spawn a child to handle the request */

              if ( childpid == 0 ) { /* Child process. This process will simulate the theatre phones (thlefwnhths)*/
                            close( listenfd ); /* Close listening socket. */
			    char id[15], zone[15], numOfTickets[15], balance[15]; //these buffers will keep the information from the "connection" string buffer
                            int nOfTick = 0, ident = 0, bal = 0; //the int values of the above string buffers
			    read(connfd, buffer, sizeof(buffer)); //we read the message from the client program, into the "connection" buffer
			    message_processor(id, zone, numOfTickets, balance); //we process the message from the client. See functions.c for more details
			    nOfTick = atoi(numOfTickets); //string to int conversion
			    bal = atoi(balance);
		            ident = atoi(id);
			    time_t start = time(NULL); //now we start counting the customer WAIT time
			    sem_wait(Nthl); //semaphore lock. If it is 0, we wait until a spot (or more) is free (Nthl>=1). This wait time is counted.
			    time_t end = time(NULL);
			    shm->waiting += difftime(end, start);
			    /*thlefwnhths connection-----------------------------------------------------------------------------------------------------------*/
			    if(shm->seats == 0) write(connfd, "Theatre seats are all full!", 50);
			    else{
			    sem_post(Noverall); //o ari8mos twn synolikwn paraggeliwn au3anetai kata 1
			    //int CardOK = 0; //"boolean" var indicating the validity of the card
			    pid_t Nchildpid;
			    Nchildpid = fork(); //a new child process for the bank
			    




			    if(Nchildpid == 0){ //BANK PROCESS
			    	sem_wait(Nbank); //we lock a bank terminal

			    	/*bank terminal connection---------------------------------------------------------------------------------------------------------*/

			    	time_t startOS = time(NULL); //now we start counting the customer SERVE time
			    	sleep(2); 
			    	time_t endOS = time(NULL);
			    	shm->serving += difftime(endOS, startOS);
			    	
				card_checker(zone[0], nOfTick, bal); //card check is done here

			    
			    	/*-----------------------END BANK TERMINAL-----------------------------------------------------------------------------------------------------*/
			    
			    	sem_post(Nbank);  //we free a bank terminal for an other process
			    	sem_close(Nbank); //we close the semaphore for this instance of the server
			    	sem_close(Nthl);
			    	sem_close(mutex);
			    	sem_close(Noverall);
			    	sem_close(Nfail);
			    	exit(0);
				} //end if(Nchildpid == 0) bank process ends here!
			    






			    
			    //here, the execution of the phone process (thlefwnhths) continues...

			    pause(); //wait a response (signal) from the child process. IF the signal is SIGTERM, this process will end.
					//else, if the signal is SIGTSTP, the reservation procedure can continue.

			    //we will simulate the time taken to find seats here (t_seatfind) using the sleep(4) function call. Notice that we wait for 4 seconds,
			    //not 6, in order to simulate the "parallel" execution of the bank process and this process (for more details check the report)

			    time_t startOS = time(NULL); //now we start counting the customer SERVE time
			    sleep(4); 
			    sleep(4); //sleep function must be called 2 times here, in order to have a 4 seconds sleep
			    //this happens because of a small "bug" caused by the combined invocation of pause() and sleep(4)
			    time_t endOS = time(NULL);
			    shm->serving += difftime(endOS, startOS); //we add up the time we count to the serving variable.
			    						//at the end of the server, the average time will be calculated using shm->serving
			    
			    sem_wait(mutex); //we lock the shared memory
			    /*shared memory access (critical section code!)-------------------------------------------------------------------------------------*/
			   
			    //here the actual allocation of seats is performed. Only one process can have access at a time.
			    seat_allocator(zone[0], ident, nOfTick);
			    
                            /*END SHARED MEMORY CRITICAL SECTION--------------------------------------------------------------------------------------------------------*/
			    
			    sem_post(mutex); //we unlock the shared memory
			   }//END else (when shm->seats > 0)
                            
			    /*END thlefwnhths connection----------------------------------------------------------------------------------------------------------------*/
			    sem_post(Nthl); //we free a phone for a new client "connection"
			    

			    sem_close(Nthl); //we close the semaphores for this running instance of the server
			    sem_close(Nbank);
			    sem_close(mutex);
			    sem_close(Noverall);
			    sem_close(Nfail);

                            exit( 0 ); /* Terminate child process. Theatre phone (thlefwnhths) process ends here*/
                     }

               close(connfd); /* Parent closes connected socket */
       }
}
