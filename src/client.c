/*
    ********************************************************************
    Odsek:          Elektrotehnika i racunarstvo
    Departman:      Racunarstvo i automatika
    Katedra:        Racunarska tehnika i racunarske komunikacije (RT-RK)
    Predmet:        Osnovi Racunarskih Mreza 1
    Godina studija: Treca (III)
    Skolska godina: 2017/2018
    Semestar:       Zimski (V)
	Autori:			Damjan Glamočić (RA65/2015) i Mihailo Marković (RA191/2015)
	
    Ime fajla:      client.c
    Opis:           TCP/IP klijent

    Platforma:      Raspberry Pi 2 - Model B
    OS:             Raspbian
    ********************************************************************
*/
/*
	********************************************************************
	Zadatak:
	Implementirati centralizovani sistem za razmenu tekstualnih poruka
	koji se sastoji iz klijentske i serverske aplikacije koje
	komuniciraju posredstvom TCP protokola. Po uspostavljanju veze
	klijent se prijavljuje na server slanjem korisnickog imena i
	lozinke. Posle uspesne autentifikacije, sve poruke klijenta
	prosledjuju se svim ostalim trenutno aktivnim klijentima. Prilikom
	prosledjivanja poruke potrebno je poslati i korisnicko ime klijenta
	koji je poslao poruku. Takodje je potrebno obavestiti sve aktivne
	klijente kada neko uspostavi ili raskine vezu sa serverom. Po
	raskidanju veze, klijent se izbacuje iz liste aktivnih klijenata.

	********************************************************************
*/

#include <stdio.h>      //for printf
#include <string.h>     //for strlen, strcnpy, strcmp, strcpy, strcat
#include <sys/socket.h> //for socket
#include <arpa/inet.h>  //for inet_addr
#include <fcntl.h>      //for open
#include <unistd.h>     //for close
#include <pthread.h>	//for pthread_creeate
#include <stdlib.h>		//for exit
#include <netdb.h>		//for hostent
#include <sys/ioctl.h>	//for size of terminal

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT   27015
#define DEFAULT_USERNAMELEN 20

/*Thread function for receiving messages*/
void *ReceiveHandler(void *pParam);
/*Function for logging in*/
void Login(char username[], char usernameTmp[], char password[]);
/*Function which prints received message*/
void PrintMessage(char message[]);

struct winsize w;			//struct for dimensions of terminal

int main(int argc , char *argv[])
{
	/*get dimensions of terminal*/
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	
	if(argc != 2)
	{
		puts("USAGE: ./client serverIpAddress\n");
		exit(EXIT_FAILURE);
	}

    int sock;													//main socket for communication with server
    struct sockaddr_in server;									//struct for server information
    char message[DEFAULT_BUFLEN + DEFAULT_USERNAMELEN + 2];		//username: messageTmp
	char messageTmp[DEFAULT_BUFLEN];							//temporary buffer for message
	char username[DEFAULT_USERNAMELEN];							//buffer for username
	char usernameTmp[DEFAULT_USERNAMELEN + 3];					//username123
	char password[DEFAULT_USERNAMELEN + 3];						//buffer for password
	char *address;												//server ip address
	char logOut[8];												//for checking if message is "#log out"
	pthread_t receiveThread;									//thread for receiving messages

	/*get address*/
	address = argv[1];
	
	/*create socket*/
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr(address);
    server.sin_family = AF_INET;
    server.sin_port = htons(DEFAULT_PORT);

	/*connect to server*/
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");
 
	/*Log in*/
	Login(username, usernameTmp, password);
	
	/*create messaage "username just logged in" */
	strcpy(message, username);
	message[strlen(message) - 1] = '\0';
	strcat(message, " just logged in");
	strcat(message, "\0");

	/*send messaage "username just logged in" to server*/
	if(send(sock, message, strlen(message), 0) < 0)
	{
		puts("Send failed\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	
	puts("You can start conversation [type >>>> #log out <<<< to log out]");
	
	/*create thread for receiving messages*/
	if(pthread_create(&receiveThread, NULL, ReceiveHandler, (void*)&sock) < 0)
	{
		perror("Couldn't create thread: ");
		close(sock);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		/*clear buffers*/
		memset(&logOut, 0, strlen(logOut));
		memset(&message, 0, DEFAULT_BUFLEN + DEFAULT_USERNAMELEN + 2);
		memset(&messageTmp, 0, DEFAULT_BUFLEN);
		
		/*Enter message*/
		fgets(messageTmp, DEFAULT_BUFLEN, stdin);
		messageTmp[strlen(messageTmp) - 1] = '\0';				//to remove enter ('\n') from the string
		strcpy(logOut, messageTmp);							//to check if message is "#log out"
		
		/*Check if message is "#log out" */
		if(strcmp(logOut, "#log out") == 0)
		{
			/*Create message "username just logged out" */
			strcpy(message, username);
			message[strlen(message) - 1] = '\0';				//to remove enter ('\n') from the string
			strcat(message, " just logged out");
			strcat(message, "\0");

			/*Send message "username just logged out" */
			if(send(sock, message, strlen(message), 0) < 0)
			{
				puts("Send failed\n");
				close(sock);
				exit(EXIT_FAILURE);
			}

			close(sock);
			exit(EXIT_SUCCESS);
		}
		
		/*Create message "username: message" */
		strcpy(message, username);
		message[strlen(message) - 1] = '\0';					//to remove enter ('\n') from the string
		strcat(message, ": ");
		strcat(message, messageTmp);
		strcat(message, "\0");

		/*Send message "username: message" */
		if(send(sock, message, strlen(message), 0) < 0)
		{
			puts("Send failed\n");
			close(sock);
			exit(EXIT_FAILURE);
		}

	}

	/*Close socket*/
    close(sock);

    return 0;
}

void *ReceiveHandler(void *pParam)
{
	char message[DEFAULT_BUFLEN];
	int sock = *(int*)pParam;
	int readSize;

	while(1)
	{
		/*Receive message */
		if((readSize = recv(sock, message, DEFAULT_BUFLEN, 0)) > 0)
		{
			message[readSize] = '\0';
			PrintMessage(message);
		}else
		{
			perror("Recv failed");
			exit(EXIT_FAILURE);
		}
	}

}

void Login(char username[], char usernameTmp[], char password[])
{
	puts("Type your username: ");
	fgets(username, DEFAULT_USERNAMELEN, stdin);
	strcpy(usernameTmp, username);
	usernameTmp[strlen(usernameTmp) - 1] = '\0';				//to remove enter ('\n') from the string
	strcat(usernameTmp, "123");
	puts("If you typed wrong username, type >>>> #back <<<< to enter it again\n");

	while(1)
	{
		puts("Type your password: ");
		fgets(password, DEFAULT_USERNAMELEN + 3, stdin);
		password[strlen(password) - 1] = '\0';
		/*If you typed wrong username*/
		if(strcmp(password, "#back") == 0)
		{
			username[0] = '\0';									//clear buffer
			usernameTmp[0] = '\0';								//clear buffer
			puts("Type your username: ");
			fgets(username, DEFAULT_USERNAMELEN, stdin);
			strcpy(usernameTmp, username);						//to check if password is username123
			usernameTmp[strlen(usernameTmp) - 1] = '\0';		//to remove enter ('\n') from the string
			strcat(usernameTmp, "123");
		}else
		{
			if(strcmp(password, usernameTmp) == 0)
			{
				puts("Login successful\n");
				break;
			}else
			{
				puts("Wrong password! Try again!");
			}
		}
	}
}

void PrintMessage(char message[])
{
	char str[w.ws_col - (strlen(message) % w.ws_col) - 1];					//for printing spaces (' ')
	memset(str, ' ', w.ws_col - (strlen(message) % w.ws_col) - 1);			//fill buffer with spaces (' ')
	str[w.ws_col - strlen(message) - 1] = '\0';
	printf("%s", str);														//print spaces (' ')
	printf("%s\n", message);												//print message which will end at next-to-last position of terminal
}