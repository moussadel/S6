

struct job{
    pid_t pid;
    int numero;
    char *nom;
    struct job *suivant;
};

struct job *tete;


//ajouter un jobs 

void AddJob(pid_t pid, const char *nom){
    static int job_count = 0;
	struct job *job=malloc(sizeof(struct job));
	job->pid = pid;
    job->numero = job_count;
    job->nom = malloc(sizeof(char)*(strlen(nom)+1));
    strcpy(job->nom, nom);
	job->suivant = tete->suivant;
	tete->suivant = job;
    job_count++;
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

//dans le pere
if(l->bg){
    // si on est dans un pipe mais pas en background
    // on liste pour les attendre tous à la fin
    PidListe[i]=child;
}else{
    // sinon, le processus est un job
    AddJob(child,l->seq[i][0]);
}