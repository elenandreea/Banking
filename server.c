#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_CLIENTS	5
#define BUFLEN 256
#define PROMPT "IBANK>"

typedef struct Client{

	char nume[13];
	char prenume[13];
	int nr_card;
	int pin;
	char parola[9];
	double sold;
	int blocat;
	int logat;
	int mysocket;

}Client;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){

	int i;

	int sockfd, newsockfd, portno, clilen, sock_udp, recv_len;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    char unblock_answer[50];
    struct sockaddr_in si_me, si_other;
    int slen = sizeof(si_other);
    int n, j;

    int prim;
	int fanta = 0;
  
    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds
 
    if (argc < 3) {
        fprintf(stderr,"Usage : %s port user_data\n", argv[0]);
                exit(1);
    }


    int incercari ;
    int k;
    int position;
	//Citesc clientii si informatiile referitoare la ei

	int nr_clienti;

	char name[13];
	char surname[13];
	int nr_card;
	int pin;
	char parol[9];
	float sold;

	Client vector_clienti[100];

	FILE *f = fopen(argv[2],"r");
	fscanf(f,"%d",&nr_clienti);

	for(i = 0; i < nr_clienti; i++){
		fscanf(f,"%s %s %d %d %s %f",name,surname,&nr_card,&pin,parol,&sold);
		strcpy(vector_clienti[i].nume, name);
		strcpy(vector_clienti[i].prenume, surname);
		vector_clienti[i].nr_card = nr_card;
		vector_clienti[i].pin = pin;
		strcpy(vector_clienti[i].parola, parol);
		vector_clienti[i].sold = sold;
		vector_clienti[i].blocat = 0;
		vector_clienti[i].logat = 0;
		vector_clienti[i].mysocket = -1;
	}

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
     if (sockfd < 0) 
        error("ERROR opening socket");
    if (sock_udp < 0) 
        error("ERROR opening udp socket");
     
     portno = atoi(argv[1]);

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
     serv_addr.sin_port = htons(portno);

     memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portno);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
              error("ERROR on binding");
     if (bind(sock_udp, (struct sockaddr *) &si_me, sizeof(struct sockaddr)) < 0) 
              error("ERROR on binding udp");
     
     listen(sockfd, MAX_CLIENTS);

     //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
     FD_SET(sockfd, &read_fds);
     FD_SET(sock_udp, &read_fds);
     fdmax = sockfd;


	 FD_SET(STDIN_FILENO, &read_fds);
     // main loop
	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if (i == STDIN_FILENO){

					scanf("%s",buffer);

					if(strncmp(buffer,"quit",4) == 0){

						char quit[10];
						strcpy(quit,"quit\n");

						FILE *out = fopen(argv[2],"w");
						fprintf(out, "%d\n",nr_clienti );
						for(int b = 0; b < nr_clienti; b++){
							fprintf(out, "%s %s %d %d %s %.2f\n",vector_clienti[b].nume,vector_clienti[b].prenume,
							vector_clienti[b].nr_card,vector_clienti[b].pin,vector_clienti[b].parola,vector_clienti[b].sold );
						}
						fclose(out);

						int t;
						for(t = 0; t <= fdmax; t++){
							if(FD_ISSET(t,&read_fds) && t != sockfd && t != 0){
								n = send (t,quit,sizeof(quit),0);
							}
						}
						sleep(2);
						exit(1);
					}
				}
			
				if (i == sockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

    			} 
					
				else {
						if (i == sock_udp) {
		    				memset(unblock_answer, 0, 50);
		    				if ((recv_len = recvfrom(sock_udp, unblock_answer, 50, 0, (struct sockaddr *) &si_other, &slen)) < 0 )
		     					error("UDP_RECIVE\n");
		     				char fill[10];
		     				int nr_cd;
		     				printf ("Am primit de la clientul de pe socketul udp %d, mesajul: %s\n", i, unblock_answer);
		     				sscanf(unblock_answer,"%s %d",fill,&nr_cd);
		     				position = -1 ;
							for(k = 0; k < nr_clienti; k++){
								if(vector_clienti[k].nr_card == nr_cd){
									position = k;
								}
							}

		     				char mybuff[50];
		     				strcpy(mybuff, PROMPT);
		     				strcat(mybuff, "Send secret\n");
                            //printf("TRIMIT %s\n", mybuff);
                            n = sendto(sock_udp, mybuff, sizeof(mybuff), 0, (struct sockaddr*) &si_other, slen);

                            if ((recv_len = recvfrom(sock_udp, unblock_answer, 50, 0, (struct sockaddr *) &si_other, &slen)) < 0 )
		     					error("UDP_RECIVE\n");

		     				printf ("Am primit de la clientul de pe socketul udp %d, mesajul: %s\n", i, unblock_answer);
		     				if (strncmp(unblock_answer, vector_clienti[position].parola, 5 ) == 0) {
		     					
		     					vector_clienti[position].blocat = 0;
		     					strcpy(mybuff, PROMPT);
			     				strcat(mybuff, " Card deblocat\n");
	                            //printf("TRIMIT %s\n", mybuff);
	                            n = sendto(sock_udp, mybuff, sizeof(mybuff), 0, (struct sockaddr*) &si_other, slen);
	                            continue;
		     				} else {

			     				strcpy(mybuff, PROMPT);
			     				strcat(mybuff, " -7 : Deblocare esuata\n");
	                            //printf("TRIMIT %s\n", mybuff);
	                            n = sendto(sock_udp, mybuff, sizeof(mybuff), 0, (struct sockaddr*) &si_other, slen);

		     				}

		    			} else {
						// am primit date pe unul din socketii cu care vorbesc cu clientii
						//actiunea serverului: recv()
						memset(buffer, 0, BUFLEN);
						
						if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
							if (n == 0) {
								//conexiunea s-a inchis
								printf("selectserver: socket %d hung up\n", i);
							} else {
								error("ERROR in recv");
							}
							
							close(i); 
							FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
						}

					
					else { //recv intoarce >0
						printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);

						char token[15];
						sscanf(buffer,"%s",token);

						if(strcmp(token,"login") == 0){
							printf("yey\n" );

							int nr_cd,pin;
							strcpy(token,"");
							sscanf(buffer,"%s %d %d",token,&nr_cd,&pin);

							printf("nr_cd:%d\n",nr_cd );
							printf("pin:%d\n",pin );

							position = -1 ;
							for(k = 0; k < nr_clienti; k++){
								if(vector_clienti[k].nr_card == nr_cd){
									position = k;
								}
							}

							if(position == -1){ // card inexistent

								char card_inexistent[40];
								strcpy(card_inexistent,"-4 Card inexistent!\n");
								n = send(i,card_inexistent,sizeof(card_inexistent),0);
								strcpy(card_inexistent,"");

							}else{

								if(vector_clienti[position].blocat == 1){
									char blocare[40];
									strcpy(blocare, PROMPT);
									strcat(blocare,"-5 Card Blocat!\n");
									n = send(i,blocare,sizeof(blocare),0);
									strcpy(blocare,"");

								}else{

									if (vector_clienti[position].logat == 1){
										char logat[40];
										strcpy(logat, PROMPT);
										strcat(logat,"-2 Sesiune deja deschisa!\n");
										n = send(i,logat,sizeof(logat),0);
										strcpy(logat,"");

									}else{

										if(vector_clienti[position].pin == pin){ //logare

											char mesaj_logare[40];
											
											strcpy(mesaj_logare,"[Logged]Welcome,");
											strcat(mesaj_logare,vector_clienti[position].nume);
											strcat(mesaj_logare,"!\n");
											n = send(i,mesaj_logare,sizeof(mesaj_logare), 0);
											vector_clienti[position].logat = 1;
											strcpy(mesaj_logare,"");
											vector_clienti[position].mysocket = i;
											incercari = 0;
											printf("%s\n", mesaj_logare);
											strcpy(token,"");

										}else{//pin gresit
											incercari ++;
											if(incercari < 3){

												char pin_gresit[40];
												strcpy(pin_gresit, PROMPT);
												strcat(pin_gresit, "-3 Pin gresit!\n");
												n = send(i,pin_gresit,sizeof(pin_gresit), 0);
												strcpy(pin_gresit,"");


											}else{

												char blocare[40];
												strcpy(blocare, PROMPT);
												strcat(blocare,"-5 Card Blocat!\n");
												n = send(i,blocare,sizeof(blocare),0);
												strcpy(blocare,"");
												incercari = 0;
												vector_clienti[position].blocat = 1;

											}
										}
									}
								}
							}


						}


						//caut socketul care contine clientul care doreste sa paraseasca conexiunea
						if(strcmp(token,"logout") == 0){

							int my_socket_position = -1;

							for(int a = 0; a < nr_clienti; a++){
								if(vector_clienti[a].mysocket == i){
									my_socket_position = a;
								}
							}

							if(vector_clienti[my_socket_position].logat == 1){ //positon in loc de k

								char esti_logat[40];
								strcpy(esti_logat, PROMPT);
								strcat(esti_logat,"Clientul a fost deconectat\n");
								n = send(i,esti_logat,sizeof(esti_logat), 0);
								vector_clienti[my_socket_position].logat = 0;
								vector_clienti[my_socket_position].mysocket = -1;
								strcpy(esti_logat,"");

							}
						}

						//caut mai intai sa vad clientul de pe care socket doreste sa si vada soldul
						if(strcmp(token,"listsold") == 0){

							int my_socket_position = -1;

							for(int a = 0; a < nr_clienti; a++){
								if(vector_clienti[a].mysocket == i){
									my_socket_position = a;
								}
							}

							char sold[50];
							sprintf(sold, "%s", PROMPT);
							sprintf(sold,"%s%0.2f\n", PROMPT, vector_clienti[my_socket_position].sold);
							//printf("TRIMIT %0.2f\n", vector_clienti[my_socket_position].sold);
							n = send(i,sold,sizeof(sold), 0);
							strcpy(sold,"");
						}

						if(strcmp(token,"transfer") == 0){

							int nr_card;
							double suma;
							//sscanf(buffer,"%s %d %d",token,&nr_card,&suma);

							//printf("%d\n",vector_clienti[k].nr_card );
							strcpy(token,"");
							sscanf(buffer,"%s %d %lf",token,&nr_card,&suma);
							printf("%s\n",token );
							printf("%d\n",nr_card );
							printf("%lf\n",suma );

							int transfer_position = -1;
							for(int j = 0; j < nr_clienti; j++){
								if(vector_clienti[j].nr_card == nr_card){
									transfer_position = j;
								}
							}

							printf("transfer catre pozitia: %d\n", transfer_position);

							if(transfer_position == -1){
								char fara_card[40];
								strcpy(fara_card, PROMPT);
								strcat(fara_card,"-4 Numar card inexistent!\n");
								printf("%s\n",fara_card );
								n = send(i,fara_card,sizeof(fara_card),0);
								strcpy(fara_card,"");

							}else{


							int my_socket_position = -1;

							for(int a = 0; a < nr_clienti; a++){
								if(vector_clienti[a].mysocket == i){
									my_socket_position = a;
								}
							}

								if(suma > vector_clienti[my_socket_position].sold){
									char fara_bani[40];
									strcpy(fara_bani, PROMPT);
									strcat(fara_bani,"-8 Fonduri insuficiente!\n");
									n = send(i,fara_bani,sizeof(fara_bani),0);
									strcpy(fara_bani,"");

								}else{

									char asigurare[100];
									printf("Trimit catre %s %s suma de %lf\n",
									vector_clienti[transfer_position].nume,
									vector_clienti[transfer_position].prenume,
									suma );
									sprintf(asigurare,"%sTransfer %lf catre %s %s ?\n", 
											PROMPT,
											suma,vector_clienti[transfer_position].nume,
											vector_clienti[transfer_position].prenume);
									n = send(i,asigurare,sizeof(asigurare),0);
									strcpy(asigurare,"");

									n = recv(i,buffer,sizeof(buffer),0);


									printf("%s\n",buffer );
									if(strcmp(buffer,"y") == 0){

										char succes[40];
										strcpy(succes, PROMPT);
										strcat(succes,"Transfer realizat cu succes\n");
										vector_clienti[transfer_position].sold += suma;
										vector_clienti[my_socket_position].sold -= suma;
										n = send(i,succes,sizeof(succes),0);
										strcpy(succes,"");
									
									}else{

										char anulare[40];
										strcpy(anulare,PROMPT);
										strcat(anulare,"-9 Operatie anulata!\n");
										n = send(i,anulare,sizeof(anulare),0);
										strcpy(anulare,"");
									}
								}
							}


						}
						if(strcmp(token,"quit") == 0){
							// o sa inchid fisierul
							close(i);
							FD_CLR(i, &read_fds);

						}

					}
				} 
			}

			}
		}
     }


     close(sockfd);
   
     return 0; 
}
