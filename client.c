#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>

#define STDIN_FILENO 0
#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int client_conectat = 0;

    int sockfd, n, udp_socket, locked;
    int number_of_tries = 0;
    int fdmax, rc;
    struct sockaddr_in serv_addr;
    struct sockaddr_in si_other;
    struct hostent *server;
    int card_number;
    int slen = sizeof(si_other);

    int pid = getpid();
    char fisier_de_log[50];
    sprintf(fisier_de_log,"client-<%d>.log",pid);
    FILE *log = fopen(fisier_de_log,"w");

    fd_set read_fds;
    fd_set tmp_fds;

    char buffer[BUFLEN];
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_address server_port\n", argv[0]);
       exit(0);
    }  

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sockfd < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);

    //udp
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &si_other.sin_addr);
    
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    
    
    // Adaugam tastatura
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd, &read_fds);

    fdmax = sockfd;

    while(1){
        tmp_fds = read_fds;
        rc = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) { 

                if (i == STDIN_FILENO){
                    char buff[100];
                    scanf("%s",buff);

                    if (strncmp (buff, "login",5) != 0 && strncmp (buff, "logout",6) != 0 && 
                        strncmp (buff, "listsold",8) != 0 && strncmp (buff, "transfer",8) != 0 &&
                        strncmp (buff, "quit",4) != 0 && strncmp (buff, "unblock",8) != 0 ){

                        printf("Comanda inexistenta\n");
                    	fprintf(log,"%s\n", "Comanda inexistenta");

                    }else{

                    	if(strncmp(buff,"quit",4) == 0){
                            n = send(sockfd,buff,sizeof(buff),0);
                            fprintf(log, "%s\n",buff );
                            fclose(log);
                            return 0;
                        }

                        //daca primim altceva in afara de login si nu suntem conectati
                        if(!locked && client_conectat == 0 && strncmp (buff , "login",5) != 0){
                            printf("-1 Clientul nu este autentificat!\n");
                            fprintf(log,"%s\n", "-1 Clientul nu este autentificat");
                        }                        


                    	if(client_conectat == 0 && strncmp (buff , "unblock",7) == 0){
                
                            char mybuff[50];
                            sprintf(mybuff,"%s %d\n",buff,card_number);
                           //printf("TRIMIT %s\n", mybuff);
                            n = sendto(udp_socket, mybuff, sizeof(mybuff), 0, (struct sockaddr*) &si_other, slen);
                            fprintf(log, "%s",mybuff );
                            
                            strcpy(mybuff, "");
                            recvfrom(udp_socket, mybuff, 50, 0, (struct sockaddr *) &si_other, &slen);
                            fprintf(log, "%s",mybuff );
		     				
		     				printf ("%s", mybuff);
		     				
		     				strcpy(mybuff, "");
	                    	scanf("%s",mybuff);
	                    	n = sendto(udp_socket, mybuff, sizeof(mybuff), 0, (struct sockaddr*) &si_other, slen);
                            fprintf(log, "%s",mybuff );
                            
                            strcpy(mybuff, "");
                            recvfrom(udp_socket, mybuff, 50, 0, (struct sockaddr *) &si_other, &slen);
                            fprintf(log, "%s",mybuff );
		     				
		     				printf ("%s", mybuff);
                        }



                        

                        if( client_conectat == 0 && strncmp (buff,"login",5) == 0){

                            char mybuff[100];
                            int nr;
                            int pin;
                            scanf("%d",&card_number);
                            scanf("%d",&pin);
                            sprintf(mybuff,"%s %d %d\n",buff, card_number,pin);
                           
                            n = send(sockfd, mybuff, sizeof(buff), 0);
                            fprintf(log, "%s",mybuff );
                            //strcpy(buff,"");
                            
                        }

                        if(client_conectat == 1 && strncmp (buff,"login",5) == 0){
                            printf("-2 Sesiune deja deschisa\n");
                            fprintf(log,"%s\n", "IBANK>-2 Sesiune deja deschisa");
                        }

                        if(client_conectat == 1 && strncmp (buff, "logout",6) == 0){
                            //client_conectat = 0;-----
                            fprintf(log, "%s\n",buff );
                        	n = send(sockfd,buff,sizeof(buff),0);
                        }

                        if(client_conectat == 1 && strncmp (buff, "listsold",8) == 0){
                            n = send(sockfd,buff,sizeof(buff),0);
                            fprintf(log, "%s",buff );
                            //printf("%s\n", buff);
                        }



                        if(client_conectat == 1 && strncmp(buff,"transfer",8) == 0){
                            int nr_card;
                            double suma;
                            char mybuff[100];
                            scanf("%d",&nr_card);
                            scanf("%lf",&suma);
                            sprintf(mybuff,"%s %d %lf\n",buff,nr_card,suma);
                            n = send(sockfd,mybuff,sizeof(mybuff),0);
                            fprintf(log, "%s",mybuff );

                            char recvbuff[100];
	                    	n = recv(sockfd, recvbuff, sizeof(recvbuff), 0);
	                    	fprintf(log, "%s", recvbuff);
	                    	printf("%s", recvbuff);

	                    	if (recvbuff[6] != '-') {

		                    	char answer[10];
		                    	printf("[Y/n]: ");
		                    	fprintf(log, "%s","[Y/n]: ");
		                    	scanf("%s",answer);
		                    	//printf("Raspunsul trimis este %s\n", answer);
		                    	n = send(sockfd,answer,sizeof(answer),0);
		                    	fprintf(log, "%s\n", answer);
		                        strcpy(buff,"");
		                    }
                        }

                    }

                }

                if (i == sockfd){

                    char buff[100];
                    n = recv(sockfd, buff, sizeof(buff), 0);
                    printf("%s", buff);
                    fprintf(log, "%s",buff);

                    fflush(stdin);

                    if(strncmp(buff,"quit",4) == 0){
                    	fclose(log);
                    	return 0;
                    }


                    if(strncmp(buff,"[Logged]",8) == 0){
                        client_conectat = 1;
                        number_of_tries = 0;

                    }

                    if(client_conectat == 0 && strncmp(buff,"[Logged]",8) != 0) {
                    	number_of_tries++;
                    	if (number_of_tries == 3)
                    		locked = 1;
                    }

                    if(strcmp(buff,"IBANK>Clientul a fost deconectat\n") == 0){
                    	client_conectat = 0;
                    }

                }
            }
        }
    }
    return 0;
}


