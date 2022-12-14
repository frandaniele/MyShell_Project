#include "include/utils.h"

void append(Node** head_ref, pid_t pid){
    Node* new_node = (Node*)malloc(sizeof(Node));
 
    Node *last = *head_ref;  
    
    new_node->pid = pid;
    new_node->next = NULL;
 
    /* Lista vacia, new node = head */
    if(*head_ref == NULL){
        new_node->n_job = 1;
        *head_ref = new_node;
        return;
    } 
      
    /* Buscamos el ultimo nodo */
    while(last->next != NULL){
        last = last->next;
    }
  
    last->next = new_node;
    new_node->n_job = last->n_job + 1;
    
    return;   
}

int eliminar_nodo(Node** head_ref, pid_t pid){
    // guardamos head para iterar
    Node *temp = *head_ref;
    Node *prev;
 
    //si head contiene el pid del proceso a eliminar
    if(temp != NULL && temp->pid == pid){
        *head_ref = temp->next; //cambiamos head
        int job = temp->n_job;
        free(temp);
        return job;
    }
 
    // buscamos el pid del proceso a eliminar
    while(temp != NULL && temp->pid != pid){
        prev = temp;
        temp = temp->next;
    }
 
    //si el pid no estaba en la lista
    if(temp == NULL)    return 1;
 
    //deslinkeamos el nodo
    prev->next = temp->next;
    
    int job = temp->n_job;

    free(temp);

    return job;
}

int last_job(Node** head_ref){
    Node *last = *head_ref;

    while(last->next != NULL){
        last = last->next;
    }

    return last->n_job;
} 

int read_text_file(char *directory, int size, char *buffer){
    FILE *fptr;
    
    if((fptr = fopen(directory, "rb")) == NULL){
        fprintf(stderr, "Error! opening file\n");
        // Program exits if the file pointer returns NULL.
        exit(-1);
    }

    int leer = 0;
    while((leer = fread(buffer, size, 1, fptr)) > 0);

    fclose(fptr); 
    
    return 0;
}

void reemplazar_char(char* string, char ch){
    char *reemplazado;

    if((reemplazado=strchr(string, ch)) != NULL){
        *reemplazado = '\0';
    }
    return;
}

void help_menu(FILE* stream, int exit_code){
    fprintf(stream, 
            "-h --help          Despliega el men?? de ayuda.\n"
            "FILENAME           Ejecuta comandos desde batch file.\n"
            "no args            Espera por inputs del usuario.\n");
    exit(exit_code);
}

void get_env_var(char* dst, char* var){
    char *ptr = getenv(var);
    if(ptr == NULL){
        fprintf(stderr, "ERROR: %s no encontrado.\n", var);
        help_menu(stderr, 1);
    }
    strcpy(dst, ptr);
    return;
}

void get_hostname(char* dst){
    read_text_file("/proc/sys/kernel/hostname", 32, dst);

    dst = strtok(dst, "\n");
    if(dst == NULL){
        fprintf(stderr, "Error al buscar el hostname.\n");
        help_menu(stderr, 1);
    }
    return;
}

int spawn(char* program, char** arg_list, int segundo_plano, int cant_args){
	pid_t child_pid;
    int child_status;
    static Node *head = NULL;

	/* Duplicate this process. */
	child_pid = fork();

    switch(child_pid){
        case -1:
            fprintf(stderr, "ERROR: fork");
            exit(1);
        case 0: ;
            /* pruebo path absoluto */
            ejecutar(arg_list[0], arg_list, cant_args, program);

            /* pruebo los paths en env var $PATH    */
            char *paths = getenv("PATH");
            char *ptr = strtok(paths, ":");
            char aux[256];

            strcpy(aux,ptr);
            strcat(aux,program);
            ejecutar(program, arg_list, cant_args, aux);

            while(ptr != NULL){
                ptr = strtok(NULL, ":");
                if(ptr != NULL){
                    strcpy(aux,ptr);
                    strcat(aux,program);
                    ejecutar(program, arg_list, cant_args, aux);
                }
            }

            /* busco en el path que me encuentro   */
            char path_actual[128];
            get_env_var(path_actual, "PWD");
            strcat(path_actual,program);
            ejecutar(program, arg_list, cant_args, path_actual);
            
            /* returns only if an error occurs. */
            fprintf(stderr, "El programa %s no fue encontrado\n", program);
            exit(1);
        default:    ;
            pid_t zombie_pid;             
            while((zombie_pid = waitpid(-1, &child_status, WNOHANG))>0){                
                printf("[%i]\t%i\t", eliminar_nodo(&head, zombie_pid), zombie_pid);
                if(WIFEXITED(child_status)){
                    if(WEXITSTATUS(child_status) == 0)      printf("Done\n");
                    else    printf("terminated with error code\t%i\n", WEXITSTATUS(child_status));
                }
                if(WIFSIGNALED(child_status)){
                    printf("exited via signal\t%i\n", WTERMSIG(child_status));
                }
            }

            if(segundo_plano){
                //Ejecuto en 2do plano
                append(&head,child_pid);
                printf("[%i] %i\n", last_job(&head), child_pid);  
            }
            else{
                //Ejecuto en 1er plano
                waitpid(child_pid, NULL, 0);
            }
    }
    return 0;
}

void ejecutar(char* program, char** arg_list, int cant_args, char* path){
    if(cant_args == 1){
        execl(path, program, (char*) NULL);
    }
    if(cant_args == 2){
        execl(path, program, arg_list[1], (char*) NULL);
    }
    if(cant_args == 3){
        execl(path, program, arg_list[1], arg_list[2], (char*) NULL);
    }	
    if(cant_args == 4){
        execl(path, program, arg_list[1], arg_list[2], arg_list[3], (char*) NULL);
    }	
    return;
}

/*  Esta funcion devuelve 1 si encuentra '&' y lo reemplaza por '\0'. 
    Caso contrario devuelve 0.  */
int identificar_seg_plano(char* str){
    if(strchr(str, '&') != NULL){
        reemplazar_char(str, '&');
        return 1;
    }
    return 0;
}