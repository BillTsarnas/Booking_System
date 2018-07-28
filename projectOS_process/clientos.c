#include <stdio.h> /*basic input/ouput functions in c*/
#include <stdlib.h> /*rand() is declared here*/
#include <time.h>/*time is declared here*/
#include <unistd.h> /*sleep() and other standard unix functions are declared here*/
#include <sys/socket.h> /* basic socket definitions */
#include <sys/types.h> /* basic system data types */
#include <sys/un.h> /* for Unix domain sockets */
#include <signal.h> /*signal handling*/

#define UNIXSTR_PATH "socketfile"


void sig_alrm( int signo )
//this handler handles a SIGALRM. We periodically raise a SIGALRM using alarm(), evey 10 secs
//to type an apology message to the client
{
	signal(SIGALRM, sig_alrm); //we set the handler again for multpiple uses. We actually don't need to put it as it is implemented
				   //automatically on newer linux distros.
	printf("Sorry for waiting %d!\n", (int)getpid());
	alarm(10);
}


struct sigaction act;


int main( int argc, char **argv )
{
	if(argc != 2) { //if there arent 2 command line arguments present (1 for executable, plus 1 for auto/manual execution) the client program exits
		printf("The client program needs one argument!\n");
		exit(1);
		}  

	int sockfd; //socket descriptor used to connect to the server
	char buf[100]; //buffer that holds the message to/from the server
        struct sockaddr_un servaddr; /* Struct for the server socket address. */     
	sockfd = socket( AF_LOCAL, SOCK_STREAM, 0 ); /* Create the client's endpoint. */

        bzero( &servaddr, sizeof( servaddr ) ); /* Zero all fields of servaddr. */
        servaddr.sun_family = AF_LOCAL; /* Socket type is local (Unix Domain). */
        strcpy( servaddr.sun_path, UNIXSTR_PATH ); /* Define the name of this socket. */

        /*sigalrm setup- periodical raise of a sigalrm is enabled (SA_RESTART) see serveros.c for more comments on setting up a signal*/	
	act.sa_handler = &sig_alrm; //we set the handler to be sig_alrm()
	sigemptyset(&(act.sa_mask)); //act.sa_mask = 0;
	act.sa_flags = SA_RESTART;  // Restart interrupted system calls


	sigaction(SIGALRM, &act, NULL); //SIGALRM handling using sig_alrm( int signo ) is enabled


        if(strcmp(argv[1], "manual") == 0) { //execution using keyboard input


			int i = 0; //string index

        		printf("Welcome to the seat reservation app! Please wait to get connected...\n");
        		connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)); /* Connect the client's and the server's endpoint. ARXH EPIKOINWNIAS */

        		printf("Connection established! Now type your card ID, the specified zone and the number of tickets (separated by space) and your account balance:\n");

        		/*------------------------KEYBOARD INPUT-----------------------------------------------------------------------------------------------------------------*/
			while( !feof( stdin ) ) { //we get the keyboard input until ctrl-D or ctrl-C is hit
	       			buf[i] = getchar(); //a single character is read
	       			i++; //index is increased to read the next character
        			}
			//while in manual mode we dont check if the number of selected tickets is above 4. 1-4 ticket limit is implemented in auto mode (see below)
			//being able to reserve more tickets was useful while debugging the server code
			/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/
	 		write( sockfd, buf , sizeof( buf ) ); //we send the message to the server
	 		strcpy( buf, "");
	 		read(sockfd, buf, sizeof( buf )); //we read the response from the server
	 		printf("\nMessage: %s\n", buf);
         		close( sockfd ); //we close the connection socket
		} //end if(strcmp(argv[1], "manual") == 0)
		
	else if(strcmp(argv[1], "auto") == 0) { //execution using random values
			int randID, randNoft, charge, randBal; //used for random id, nOfTick and balance generation
			int randZone = 0;  //used for random zone generation
			char cZone; //selected zone
			srand (/*time(NULL) + */(unsigned int)getpid()); // It seeds the pseudo random number generator that rand() uses. SEED is different in every execution!
			randID = rand() % 9999 + 1; //we generate a random customer id (1-9999)
			randNoft = rand() % 4 + 1; //we generate a random number of tickets (1 - 4)
			randZone = rand() % 100 + 1; //we generate randomly the selected zone
			if((randZone >=1) && (randZone <= 10)) { charge = 50; cZone = 'A'; } //10% chance to select A
			else if((randZone >=11) && (randZone <= 30)) { charge = 40; cZone = 'B'; } //20% chance to select B
			else if((randZone >=31) && (randZone <= 60)) { charge = 35; cZone = 'C'; } //30% chance to select C
			else if((randZone >=61) && (randZone <= 100)) { charge = 30; cZone = 'D'; } //40% chance to select D
			randBal = rand() % 10 + ((randNoft * charge) - 1);	//we generate random user account balance (10% chance to be invalid - 1 euro less than the required amount)
			sprintf(buf, "%d %c %d %d ", randID, cZone, randNoft, randBal); //formatted string message to server
			connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)); ///* Connect the client's and the server's endpoint. ARXH EPIKOINWNIAS */
			write( sockfd, buf , sizeof( buf ) ); //we sent the string message to the server
			strcpy( buf, ""); //we clear the message buffer
			alarm(10); //after we send the message to the server, while waiting, every 10 sec a sorry message pops up
	 		read(sockfd, buf, sizeof( buf )); //we read the message sent by the server into the buffer
			printf("\npid: %d Message: %s\n", (int)getpid(), buf);
         		close( sockfd ); //we close the connection socket
		} //end if(strcmp(argv[1], "auto") == 0)
return 0;
}
