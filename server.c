#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFLEN 256
#define MAXCLIE 100

// Structura unui user din fisier
struct user {
	char   first_name[12 + 1];     // Prenumele persoanei
	char   last_name[12 + 1];      // Numele persoanei
	char   card_no[6 + 1];         // Numarul cardului
	char   pin[4 + 1];             // Pin-ul cardului
	char   secret_password[8 + 1]; // Parola secreta
	double balance;                // Sold-ul, precizie: 2 zecimale
	int    locked;                 // 1 daca cardul este blocat, 0 altfel
	int    logged;                 // 1 daca este logat, 0 altfel
	int    attempt;                // Numarul de incercari esuate de logare
};
typedef struct user User;

int   PORT;          // Portul pentru UDP si TCP
int   users_no;      // Numarul de persoane (useri)
User* users;         // Datele (userii)
int logged[MAXCLIE]; // logged[k] = -1, daca clientul k nu este logat,
        	 //    	  =  x, daca clientul x este conectat la user-ul x
int last_command_was_transfer[MAXCLIE]; // -1 daca ultima comanda NU a fost transfer, x altfel, unde
                    // x este destinatarul fondurilor
float values[MAXCLIE];

// Functia de iesire in caz de eroare
void error(char *msg, int error_no)
{
    perror(msg);    // Afisez un mesaj
    exit(error_no); // Trimit un cod de eroare
}

// Verificare numar argumente primite
void check_no_args(int argc) {
	if (argc < 2) {
    error("Not enough arguments!\n", -10);
	}
}

void set_port(char* port) {
	PORT = atoi(port);
}

// Verific daca fisierul exista
void check_if_users_file_exists(FILE* returned_value) {
	if (returned_value == 0) {
    error("Eroare la deschiderea fisierului!\n", -1);
	}
}

void set_logged() {
	for (int i = 0; i < MAXCLIE; i++) {
    logged[i] = -1;
    last_command_was_transfer[i] = -1;
	}
}

// Citirea din fisier a datelor
void read_users(char* file_name) {
	
	// Deschiderea si verificarea fisierului
	FILE* input_file = fopen(file_name, "rt");
	check_if_users_file_exists(input_file);

	// Citirea numarului de intrari si alocarea memoriei
	fscanf(input_file, "%d", &users_no);
	users = (User*) malloc(sizeof(User) * users_no);

	// Citirea intrarilor
	for (int i = 0; i < users_no; i++) {
    fscanf(input_file,
    	"%s %s %s %s %s %lf",
    	users[i].last_name,
    	users[i].first_name,
    	users[i].card_no,
    	users[i].pin,
    	users[i].secret_password,
    	&users[i].balance);
    users[i].locked  = 0;
    users[i].logged  = 0;
    users[i].attempt = 0;
	}

	// Inchiderea fisierului de citire
	fclose(input_file);
}

// Scrierea in fisier a datelor (Salvarea lor)
void write_users(char* file_name) {
	// Deschiderea si verificarea fisierului
	FILE* output_file = fopen(file_name, "wt");
	check_if_users_file_exists(output_file);

	// Scrierea numarul de persoane
	fprintf(output_file,"%d\n" ,users_no);

	// Scrierea datelor
	for (int i = 0; i < users_no; i++) {
    fprintf(output_file,
    	"%s %s %s %s %s %.2lf\n",
    	users[i].last_name,
    	users[i].first_name,
    	users[i].card_no,
    	users[i].pin,
    	users[i].secret_password,
    	users[i].balance);
	}

	// Inchiderea fisierului
	fclose(output_file);
}

// Eliberarea memoriei alocate pentru date
void free_users() {
	free(users);
	users = NULL;
}

void run() {

    int sockfd, newsockfd, clilen, sockfd_udp, clilen_udp;
    char buffer[BUFFLEN];
    struct sockaddr_in serv_addr, cli_addr, serv_addr_udp, cli_addr_udp;
    int n, i, j;

    fd_set read_fds; // Multimea de citire folosita in select()
    fd_set tmp_fds;  // Multime folosita temporar 
    int fdmax;       // Valoare maxima file descriptor din multimea read_fds

    // Golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
     
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
       error("Eroare la apel socket", -10);
    }

    sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_udp < 0) {
    	error("Eroare la apel socket", -10);
    }
     
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii: 127.0.0.1
    serv_addr.sin_port = htons(PORT);

    memset((char *) &serv_addr_udp, 0, sizeof(serv_addr));
    serv_addr_udp.sin_family = PF_INET;
    serv_addr_udp.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii: 127.0.0.1
    serv_addr_udp.sin_port = htons(PORT);
     
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
    	error("Eroare la apel bind", -10);

    if (bind(sockfd_udp, (struct sockaddr *) &serv_addr_udp, sizeof(struct sockaddr)) < 0)
    	error("Eroare la apel bind", -10);
     
    listen(sockfd, MAXCLIE);
    listen(sockfd_udp, MAXCLIE);

    // Adaug descriptorul pentru citirea de la tastatura (consola, STDIN)
	FD_SET(0, &read_fds);

    // adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;

    FD_SET(sockfd_udp, &read_fds);
    fdmax++;

    while (1) {
    tmp_fds = read_fds;
    if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
    	error("Eroare la apel select", -10);
	
    for(i = 0; i <= fdmax; i++) {
    	if (FD_ISSET(i, &tmp_fds)) {

        if (i == 0) {
        	// Am primit mesaj la consola

        	memset(buffer, 0, BUFFLEN);
        	scanf("%s", buffer);

        	if (strncmp(buffer, "QUIT", 4) == 0) {
            	close(sockfd);

            return;
        	} else {
            error("Eroare: functia nu exista", -10);
        	}

        } else if (i == sockfd) {
        	// Un nou client s-a conectat la server

        	clilen = sizeof(cli_addr);
        	if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1) {
            error("Eroare la apel accept", -10);
        	} 
        	else {
            // Se adauga noul descriptor pentru citire intors de accept()
            FD_SET(newsockfd, &read_fds);
            if (newsockfd > fdmax) { 
            	fdmax = newsockfd;
            }
        	}

        } else if (i == sockfd_udp) {
        	// Se cere un unlock

        	char buf[256];
        	int len = recvfrom(sockfd_udp, buf, 256, 0, (struct sockaddr*) &cli_addr_udp, &clilen_udp);

        	memset(buffer, 0, BUFFLEN);
        	sprintf(buffer, "IBANK> -5 : Trimite parola secreta\n\n");

            sendto(i, buffer, strlen(buffer), 0, (struct sockaddr *) &cli_addr_udp, sizeof(struct sockaddr_in));
        }

        else {
        	// Primesc date de la unul dintre clienti

        	memset(buffer, 0, BUFFLEN);
        	if ((n = recv(i, buffer, sizeof(buffer), 0)) < 0) {
            // Eroare la apel

            error("Eroare la apel recv", -10);
        	} else if (n == 0) {
            // Clientul a inchis conexiunea

            close(i); 
            FD_CLR(i, &read_fds); // Se scoate socketul din multimea
        	} else {
            // Am primit un mesaj

            char tok[2] = " \n";
            char* token = strtok(buffer, tok); // Comanda

            // LOGIN
            if (strncmp(token, "login", 5) == 0) {
            	token = strtok(NULL, tok); // Numar card

            	int gasit  = 0;
            	int id_tmp = -1;

            	for (int count = 0; count < users_no; count++) {
                if (strcmp(users[count].card_no, token) == 0) {
                	if (users[count].logged == 1) {
                    memset(buffer, 0, BUFFLEN);
                    sprintf(buffer, "IBANK> -2 : Sesiune deja deschisa\n\n");
                	} else if (users[count].locked == 1) {
                    memset(buffer, 0, BUFFLEN);
                    sprintf(buffer, "IBANK> -5 : Card blocat\n\n");
                	} else {
                    id_tmp = count;
                	}

                	gasit = 1;
                	count = users_no;
                }
            	}

            	if (gasit == 1 && id_tmp != -1) {
                token = strtok(NULL, tok); // Parola

                if (strcmp(users[id_tmp].pin, token) == 0) {
                	users[id_tmp].logged = 1;
                	logged[i] = id_tmp;
                	users[id_tmp].attempt = 0;
                	memset(buffer, 0, BUFFLEN);
                	sprintf(buffer, "IBANK> Welcome %s %s\n\n", users[id_tmp].last_name, users[id_tmp].first_name);
                } else {
                	memset(buffer, 0, BUFFLEN);
                	sprintf(buffer, "IBANK> -3 : Pin gresit\n\n");

                	users[id_tmp].attempt++;

                	if (users[id_tmp].attempt == 3) {
                    users[id_tmp].attempt = 0;
                    users[id_tmp].locked = 1;
                    memset(buffer, 0, BUFFLEN);
                    sprintf(buffer, "IBANK> -5 : Card blocat\n\n");
                	}
                }
            	}

            	if (gasit == 0) {
                memset(buffer, 0, BUFFLEN);
                sprintf(buffer, "IBANK> -4 : Numar card inexistent\n\n");
            	}
            }

            // LOGOUT
            else if (strncmp(token, "logout", 6) == 0) {
            	users[logged[i]].logged = 0;
            	logged[i] = -1;
            	memset(buffer, 0, BUFFLEN);
            	sprintf(buffer, "IBANK> : Clientul a fost deconectat\n\n");
            }

            // LISTSOLD
            else if (strncmp(token, "listsold", 8) == 0) {
            	memset(buffer, 0, BUFFLEN);
            	sprintf(buffer, "IBANK> : %.2f\n\n", users[logged[i]].balance);
            }

            else if (strncmp(token, "transfer", 8) == 0) {
            	token = strtok(NULL, tok);

            	int gasit  = 0;
            	int id_tmp = -1;

            	for (int count = 0; count < users_no; count++) {
                if (strcmp(users[count].card_no, token) == 0) {
                	id_tmp = count;
                	gasit = 1;
                	count = users_no;
                }
            	}

            	if (gasit == 0) {
                memset(buffer, 0, BUFFLEN);
                sprintf(buffer, "IBANK> -4 : Card inexistent\n\n");
            	} else {
                token = strtok(NULL, tok);
                double value = atof(token);

                if (value > users[logged[i]].balance) {
                	memset(buffer, 0, BUFFLEN);
                    sprintf(buffer, "IBANK> -8 : Fonduri insuficiente\n\n");
                } else {
                	memset(buffer, 0, BUFFLEN);
                	sprintf(buffer, "IBANK> : Transfer %s catre %s %s? [y/n]\n"
                    ,token, users[id_tmp].last_name, users[id_tmp].first_name);

                	last_command_was_transfer[i] = id_tmp;
                	values[i] = value;
                }
            	}
            }

            // QUIT
            else if (strncmp(token, "quit", 4) == 0) {
            	if (logged[i] != -1) {
                users[logged[i]].logged = 0;
                logged[i] = -1;
            	}

            	close(i); 
            	FD_CLR(i, &read_fds); // Se scoate socketul din multimea
            }

            // Daca nu se recunoaste comanda, aceasta poate fi aprobarea pentru transfer
            else if (last_command_was_transfer[i] != -1) {

            	if (strncmp(buffer, "y", 1) == 0) {

                users[logged[i]].balance -= values[i];
                users[last_command_was_transfer[i]].balance += values[i];

                memset(buffer, 0, BUFFLEN);
                sprintf(buffer, "IBANK> Transfer realizat cu succes\n\n");
            	} else {
                memset(buffer, 0, BUFFLEN);
                sprintf(buffer, "IBANK> -9 : Operatie anulata\n\n");	
            	}
            	last_command_was_transfer[i] = -1;
            }

            	n = send(i, buffer,strlen(buffer), 0);
        	}
        } 
    	}
    }
    }

    // Se inchide socketul care asculta pentru noi conexiuni
	close(sockfd);
}

int main(int argc, char* argv[]) {
	
	// Testare numar argumente
	check_no_args(argc);
	
	// Citirea intrarilor
	read_users(argv[2]);

	// Setarea variabilelor logged pe -1
	set_logged();

	// Setare port
	set_port(argv[1]);

	// Serverul ramane deschis pana primeste QUIT
	run();

	// Salvarea in fisier a datelor
	write_users(argv[2]);

	// Eliberarea memoriei
	free_users();

	// Iesire cu succes
	return 0;
}
