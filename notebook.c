#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "readLine.c"

#define BUF_SIZE 1024

void sighandler() {

	execlp("rm","rm","-rf","Temp/",NULL);
	exit(EXIT_SUCCESS);
}

int parseCommand(char* command, char* args[]) {
    int a = 1;
    char* token; 
    token = strtok(command, " ");
    args[0] = (char*) malloc(sizeof(token));
	strcpy(args[0], token);
	token = strtok(NULL, " ");
    while(token != NULL) {
    	args[a] = (char*) malloc(sizeof(token));
		strcpy(args[a], token);	
	    a++;
     	token = strtok(NULL, " ");
    }
    return a;
}

void executeComando(char* comando, int i) {
	pid_t pid;
	int inputfd, outputfd;
	ssize_t retsize;
	char buffer[BUF_SIZE];
	char *args[50];
	char* token;
	int x;

	for(x = 0; x<BUF_SIZE; x++) { buffer[x] = '\0'; }
	strcpy(buffer, comando);


	/* abrir output fifo */
	char* outputActual = malloc(sizeof("Temp/Out") + sizeof(int));
	snprintf(outputActual, sizeof("Temp/Out") + sizeof(int), "Temp/Out%i", i);
	outputfd = open (outputActual, O_WRONLY | O_CLOEXEC);
	free(outputActual);

	if(buffer[1] == ' ') {
		token = strtok(buffer, " ");
		token = strtok(NULL, "");
		token = strtok(token, "\n");
		x = parseCommand(token, args);
    	args[x] = '\0';

		if((pid = fork()) < 0) {
			perror("Erro fork");
			exit(EXIT_FAILURE);
		} else if (pid == 0) {
			dup2(outputfd, 1);
			if (execvp(args[0], &args[0]) < 0) {
				perror("Erro ao executar comando\n");
				exit(EXIT_FAILURE);
			}
		} else {
			wait(NULL);
			close(outputfd);
			exit(EXIT_SUCCESS);
		}
	} else  {
		if (buffer[2] == ' ') { 
			/* abrir input fifo */
			char* inputActual = malloc(sizeof("Temp/Out") + sizeof(int));
			snprintf(inputActual, sizeof("Temp/Out") + sizeof(int), "Temp/Out%i", i-1);
			if ((inputfd = open (inputActual, O_RDONLY | O_CLOEXEC) ) < 0) {
				perror("Erro open inputfd\n");
				exit(EXIT_FAILURE);
			}	
		} else {

			/* abrir input fifo */
			char* inputActual = malloc(sizeof("Temp/Out") + sizeof(buffer[2]));
			snprintf(inputActual, sizeof("Temp/Out") + sizeof(int), "Temp/Out%i", i-(atoi(&buffer[1])));
			if ((inputfd = open (inputActual, O_RDONLY | O_CLOEXEC) ) < 0) {
				perror("Erro open inputfd\n");
				exit(EXIT_FAILURE);
			}	
		}
		token = strtok(buffer, " ");
		token = strtok(NULL, "");
		token = strtok(token, "\n");

		x = parseCommand(token, args);
		args[x] = '\0';
		if((pid = fork()) < 0) {
			perror("Erro fork");
			exit(EXIT_FAILURE);
		} else if (pid == 0) {
			dup2(outputfd, 1);
			dup2(inputfd, 0);
			if (execvp(args[0], &args[0]) < 0) {
				perror("Erro ao executar comando2\n");
				exit(EXIT_FAILURE);
			}
		} else {
			wait(NULL);
			close(inputfd);
			close(outputfd);
			exit(EXIT_SUCCESS);
		}
	}
}


int main(int argc, char* argv[]) {
	int notebookfd, temp_notebookfd, outfd;
	ssize_t ret_in, ret_out;
	char buffer[BUF_SIZE];
	int i = 1, x, flag = 1;                                           /* i -indice de comando actual | x para ciclos for */
	pid_t pid;
	char* outputActual = malloc(sizeof("Temp/Out") + sizeof(int)); 

    signal(SIGINT, sighandler);

    mkdir("Temp", 0777);
	/* Verificar argumentos */
	if(argc != 2) {
		perror ("1 ficheiro como argumento!\n");
		return (EXIT_FAILURE);
	}

	/* Criar o file descriptor do notebook*/
	notebookfd = open (argv [1], O_RDONLY);
	if (notebookfd == -1) {
		perror ("open notebook\n");
		return (EXIT_FAILURE);
	}

	/* Criar o file descriptor do notebook temporario*/
	temp_notebookfd = open ("Temp/novoTempNotebook", O_CREAT | O_WRONLY, 0666);
	if (temp_notebookfd == -1) {
		perror ("open notebook temporario\n");
		return (EXIT_FAILURE);
	}

	for(x=0;x<BUF_SIZE;x++) { buffer[x] = '\0'; }	
	/* Ler primeira linha do notebook */
	ret_in = readLine (notebookfd, &buffer, BUF_SIZE);
	while(ret_in > 0) {
		if(flag = 1) {
			snprintf(outputActual, sizeof("Temp/Out") + sizeof(int) , "Temp/Out%i", i);    
			outfd = open(outputActual, O_CREAT | O_RDONLY | O_NONBLOCK, 0666);
			if (outfd == -1){
				perror ("open output fifo\n");
				return (EXIT_FAILURE);
			}
		flag = 0;
		}

		ret_out = write(temp_notebookfd, &buffer, ret_in);
		ret_out = write(temp_notebookfd, "\n", 1);

		if (buffer[0] == '$') {


			if ((pid = fork()) < 0) {
				perror("fork fail\n");
				exit(EXIT_FAILURE);
			} else if (pid==0) {
				executeComando(&buffer, i);
			} else {
				wait(NULL);
				ret_out = write(temp_notebookfd, ">>>\n", 4);
				for(x=0;x<=ret_in;x++) {buffer[x] = '\0'; }
				ret_in = read(outfd, &buffer, BUF_SIZE);
				ret_out = write(temp_notebookfd, &buffer, ret_in);
				for(x=0;x<=ret_in;x++) {buffer[x] = '\0'; }
				ret_out = write(temp_notebookfd, "<<<\n", 4);
				close(outfd);
				flag = 1;
				i++;
			}
		}
		for(x=0;x<BUF_SIZE;x++) { buffer[x] = '\0'; }	
		ret_in = readLine(notebookfd, &buffer, BUF_SIZE);
			if (buffer[0] == '>') {
			while (buffer[0] !='<') {
				ret_in = readLine (notebookfd, &buffer, BUF_SIZE);
			}
			ret_in = readLine (notebookfd, &buffer, BUF_SIZE);
		}
	}

	remove(argv[1]);
	rename("Temp/novoTempNotebook", argv[1]);
	execlp("rm","rm","-rf","Temp/",NULL);
	free(outputActual);
	close(temp_notebookfd);
	close(notebookfd);
	return (EXIT_SUCCESS);
}
