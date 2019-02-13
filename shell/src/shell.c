#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include "readcmd.h"
#include <signal.h>

static int cpt = 0;
static int s = 0;

void HandlerCHILD(int sig)  {
	pid_t fils;
	while((fils = waitpid(-1, NULL, WNOHANG)) > 0) {
            printf("[%d] : terminated in Background\n", fils);
    }
   
}

void HandlerSIGINT(int sig)
{   
    printf("\nSIGINT recus ^^ \n");
    fflush(stdout);
    return;
    
}
void HandlerSIGTSTP(int sig){
    s = 1;
    cpt++;
    printf("\nSIGTSTP recus ^^\n");
    fflush(stdout);
    return;    
}


struct job{
    pid_t pid;
    int numero;
    char *nom;
    struct job *suivant;
};

struct job *tete;

int SeqPipes (char*** seq){
	int i;
	for(i = 0;seq[i]!=NULL;i++){
		if(i >= 20){
			return -1;
		}
	}
	return i;
}

//ajouter un jobs 

void AddJob(pid_t pid, const char *nom){
    static int job_cpt = 0;
	struct job *job=malloc(sizeof(struct job));
	job->pid = pid;
    job->numero = job_cpt;
    job->nom = malloc(sizeof(char)*(strlen(nom)+1));
    strcpy(job->nom, nom);
	job->suivant = tete->suivant;
	tete->suivant = job;
    job_cpt++;
}

// liste des jobs en cours d'execution avec leur nom
void ListerJobs(){
	struct job *j=tete;
    int status;
	while(j->suivant!=NULL){
        j=j->suivant;
        int etat = waitpid(j->pid, &status, WNOHANG);
        if(etat==0){
            printf("[%i]+ Running : %s \n", j->numero, j->nom);
        }else if(etat!=-1){
            printf("[%i]+ Done    : %s \n", j->numero, j->nom);
        }else{
            printf("[%i]+ Old    : %s \n", j->numero, j->nom);
        }
	}
}

void FreeJobs(){
	struct job *j1, *j2;
	j1=tete;
	while(j1->suivant!=NULL){
		j2 = j1;
		j1 = j1->suivant;
        free(j1->nom);
		free(j2);
	}
	free(j1);
}

int main()
{
	pid_t child;
	int in = -1;
	int out = -1;
	int PipeFd[20][2];

	int avant[2];
	int apres[2];

	pid_t *PidListe ;
	
	/* initialisation de la sentinelle des Jobs */
	tete = malloc(sizeof(struct job));
	tete->suivant = NULL;

	while (1) {
 		
		struct cmdline *l;
		int i;
		
		printf("shell> ");
		fflush(stdin);
		l = readcmd();
		/* If 1 stream closed, normal termination */
		if(!l){

			printf("exit\n");
			exit(0);
		}
		
		if(l->seq[0] ==  NULL){
			continue;
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");

		for (int i = 0; l->seq[i]!=0; ++i){

			PidListe= malloc(sizeof(pid_t)*i);
			if(PidListe==NULL){
				printf("ERROR!!!!\n");
			}
		}

		if(l->seq[0]){

			// si on veut afficher les jobs
			if(strcmp(l->seq[0][0], "jobs")==0){
            	ListerJobs();
        	}
			
			if (!strcmp(l->seq[0][0],"quit") || !strcmp(l->seq[0][0],"q")){
				printf("GOOD BYE \n");
				exit(0);
			}
		
			else{
				for(i = 0; i < SeqPipes(l->seq)-1; i++){
					if(pipe(PipeFd[i]) < -1){
						fprintf(stderr, "IMPOSSIBLE DOUVRIRE LE PIPE NUMERO: %d\n",i);
						exit(2);
					}	
				} 	
				pipe(apres);
				pipe(avant);
   				signal(SIGCHLD, HandlerCHILD); 
   				signal(SIGINT,SIG_IGN);   //Ignorer le signal SIGINT
   				signal(SIGTSTP,SIG_IGN);  //Ignorer le signal SIGTSTP
				//execution des commande shell 
				for (i = 0; i < SeqPipes(l->seq); i++){
					child = fork();
					signal(SIGINT,HandlerSIGINT);
   					signal(SIGTSTP,HandlerSIGTSTP);

					if (child==0){ // Child
						
						if (l->in){ 
							in=open(l->in,O_RDONLY);
							if(in == -1){
								perror(l->seq[i][0]);
								exit(2);
							}
							dup2(in, STDIN_FILENO);
						}

						if (l->out){ 
							out= open(l->out, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							if(out == -1){
								perror(l->seq[i][0]);
								exit(2);
							}
							dup2(out, STDOUT_FILENO);
						}	
						
						// Gestion des pipes 
						if (i != SeqPipes(l->seq)-1){ 
							dup2(apres[1], STDOUT_FILENO);
                        	close(apres[0]);
                        	close(apres[1]);
						}

						if (i){ 
							dup2(avant[0], STDIN_FILENO);
							close(avant[0]);
                        	close(avant[1]);
						}
						//signal(SIGTSTP,handler);
						execvp(l->seq[i][0], l->seq[i]);
						if(strcmp(l->seq[0][0], "jobs")){
							printf("%s :commande not found\n",l->seq[i][0]);
						}
						exit(0);
					}else { //father
						  // si ce n'est pas le premier, il existe un pipe obsolète à fermer
	                    if(i>0){
	                        close(avant[0]);
	                        close(avant[1]);
	                    }
	                    // on décale les pipes
	                    avant[0]=apres[0];
	                    avant[1]=apres[1];

	                    // si ce n'est pas le dernier, il faut un pipe pour après
	                    if(l->seq[i+1]!=0){
	                        pipe(apres);
	                    }

	                    if(l->bg){
							// si on est dans un pipe mais pas en background
                        	// on liste pour les attendre tous à la fin
                        	PidListe[i]=child;
                    	}else{
                        	// sinon, le processus est un job
                        	AddJob(child,l->seq[i][0]);
						}

						while(waitpid(child, NULL, 0|WUNTRACED) == -1){} //waiting for the child 
    						if (s!=1){
    							printf("[%d]: Success\n", child);
    						}else{
    		 					printf("[%d]\n", child);
              					printf("[%d] :terminated abnormally\n",cpt);
              					s=0;
    						}
					}
					FreeJobs();
				}	
			}
		}
	}
}