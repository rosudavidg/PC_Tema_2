#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define BUFLEN 256

void close_log_file();
void open_log_file();

int PID;
FILE* log_fd;
int logged = 0; // Clientul este logat (exista o sesiune deschisa)
int last_command_was_transfer = 0;
char my_card_no[BUFLEN];

// Functia de iesire in caz de eroare
void error(char *msg, int error_no)
{
	close_log_file();

    perror(msg);    // Afisez un mesaj
    exit(error_no); // Trimit un cod de eroare
}

// Verificare numar argumente primite
void check_no_args(int argc) {
	if (argc < 2) {
		error("Not enough arguments!\n", -10);
	}
}

void open_log_file() {
	char file_name[BUFLEN];

	PID = getpid();
	sprintf(file_name, "client-%d.log", PID);
	log_fd = fopen(file_name, "wt");
}

void close_log_file() {
	fclose(log_fd);
}

void run(char* argv[]) {

	int sockfd, n, sockfd_udp;
    struct sockaddr_in serv_addr, serv_addr_udp;
    struct hostent *server;

    char buffer[BUFLEN];  
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("Eroare la apel socket", -10);

    sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_udp < 0)
    	error("Eroare la apel socket", -10);
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);

    serv_addr_udp.sin_family = PF_INET;
    serv_addr_udp.sin_port   = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr_udp.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("Eroare la apel connect", -1);    

    if (connect(sockfd_udp, (struct sockaddr*) &serv_addr_udp,sizeof(serv_addr)) < 0) 
        error("Eroare la apel connect", -1);    

    while(1){
  		// Citesc de la tastatura
    	memset(buffer, 0 , BUFLEN);
    	fgets(buffer, BUFLEN-1, stdin);

    	fprintf(log_fd, "%s", buffer);

    	// Daca comanda este login si exista o sesiune deschisa
    	if (strncmp(buffer, "login", 5) == 0 && logged == 1) {
    		fprintf(log_fd, "IBANK> -2 : Sesiune deja deschisa\n\n");
    		printf("IBANK> -2 : Sesiune deja deschisa\n\n");
    		continue;
    	}

    	// Daca comanda este quit
    	if (strncmp(buffer, "quit", 4) == 0) {
    		n = send(sockfd, buffer, strlen(buffer), 0);
			if (n < 0) {
        		error("Eroare la apel recv", -10);
        	}

        	return;    		
    	}

    	// Daca e orice comanda in afara de quit, unlock sau login si nu sunt conectat
    	if (strncmp(buffer, "login", 5) != 0 && logged == 0 && strncmp(buffer, "unlock", 6) != 0) {
    		fprintf(log_fd, "-1 : Clientul nu este autentificat\n\n");
    		printf("-1 : Clientul nu este autentificat\n\n");
    		continue;
    	}

    	if (strncmp(buffer, "logout", 6) == 0) {
    		logged = 0;
    	}

		if (last_command_was_transfer == 1) {
			last_command_was_transfer = 0;

			if (strncmp(buffer, "IBANK> Transfer", 15) == 0) {
        		// Citesc de la tastatura
    			printf("SE ASTEAPTA CEVA\n");
    			memset(buffer, 0 , BUFLEN);
    			fgets(buffer, BUFLEN - 1, stdin);
        	}
    	}

    	if (strncmp(buffer, "transfer", 8) == 0) {
    		last_command_was_transfer = 1;
    	}

    	if (strncmp(buffer, "login", 5) == 0) {

    		char buffer_copy[BUFLEN];
    		strcpy(buffer_copy, buffer);

    		char* token = strtok(buffer_copy, " \n");
    		token = strtok(NULL, " \n");
    		strcpy(my_card_no, token);
    	}

    	if (strncmp(buffer, "unlock", 6) == 0) {

    		printf("%s", buffer);
        	fprintf(log_fd, "%s", buffer);

    		char command[256];
    		sprintf(command, "unlock %s", my_card_no);
    		sendto(sockfd_udp, command, strlen(command), 0, (struct sockaddr *) &serv_addr_udp, sizeof(struct sockaddr_in));

    		memset(buffer, 0, BUFLEN);
        	int size_msg;
			int len = recvfrom(sockfd_udp, buffer, 256, 0, (struct sockaddr*) &serv_addr_udp, &size_msg);

        	if (n < 0) {
        		error("Eroare la apel recv", -10);
        	}

        	printf("%s", buffer);
        	fprintf(log_fd, "%s", buffer);
        	continue;
    	}

    	// Trimit mesaj la server
    	n = send(sockfd, buffer, strlen(buffer), 0);
    	if (n < 0) 
        	 error("Eroare la apel send", -10);

    	// Primesc mesaj de la server
        memset(buffer, 0, BUFLEN);
        n = recv(sockfd, buffer, sizeof(buffer), 0);

        if (n < 0) {
        	error("Eroare la apel recv", -10);
        }

        printf("%s", buffer);
        fprintf(log_fd, "%s", buffer);

        if (strncmp(buffer, "IBANK> Welcome", 14) == 0) {
        	logged = 1;
        }
    }
}

int main(int argc, char *argv[])
{
	// Deschid fisierul pentru log
	open_log_file();

	// Verific numarul de argumente
	check_no_args(argc);

	// Pornesc clientul
	run(argv);

	// Inchid fisierul pentru log
	close_log_file();

	// Iesire cu succes
    return 0;
}
