#include "../inc/matrix.h"
#include "../inc/utils.h"
#include <stdlib.h>
#include <sys/shm.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>

/// variabili globali
int dim; // ordine della matrice
int n_proc; // numero dei processi da utilizzare
int shm_ids[5]; // id dei segmenti di memoria condivisa

int id_child; // numero del figlio
int msgid; // id della coda di messaggi
int semid; // id del semaforo
char *pathC;

/// matrici in memoria condivisa
int *matA;
int *matB;
int *matC;

/// somma e c in memoria condivisa
int *somma;
int *c;
/// pipes da padre a figlio
int (*pipes)[2]; // due array di puntatori
/// pids dei figli
pid_t *pids;

// funzioni varie
void create_ipc();

void free_ipc(int signid);

/// funzioni per i semafori
void create_sem();

void create_children();

void padre();

void figlio();

int main(int argc, char *argv[]) {
    if (check_args(argc, argv) == 1) // controllo gli argomenti
        exit(1);

    signal(SIGTERM, free_ipc); // registro la funzione in caso di uscita
    signal(SIGSEGV, free_ipc);
    signal(SIGINT, free_ipc);

    dim = atoi(argv[4]);
    n_proc = atoi(argv[5]);
    pathC = argv[3];
    pipes = malloc(sizeof(int[2]) * n_proc);
    pids = malloc(sizeof(int) * n_proc);
    for (int i = 0; i < n_proc; i++)
        //setto a zero tutto l'array di pids, così in free_ipc posso ciclare e terminare figli finché essi sono stati generati
        pids[i] = 0;

    create_ipc();
    create_sem();

    copy_matrix_on_shm(load_matrix(argv[1], dim), dim, matA);
    copy_matrix_on_shm(load_matrix(argv[2], dim), dim, matB);

    println("Matrice A:");
    print_matrix(matA, dim);
    println("Matrice B:");
    print_matrix(matB, dim);

    create_children();
}

void create_ipc() {
    for (int i = 0; i < 3; i++) { // creo i segmenti di memoria condivisa per le matrici
        shm_ids[i] = shmget(IPC_PRIVATE, sizeof(int) * dim * dim, 0666 | IPC_CREAT);
        if (shm_ids[i] == -1) {
            print_error(SHARED_MEMORY, __func__);
            exit(1);
        }
    }
    // attacco i segmenti al processo corrente
    if ((matA = shmat(shm_ids[0], NULL, 0)) == (void *) -1) {
        print_error(SHARED_MEMORY, __func__);
        exit(1);
    }
    if ((matB = shmat(shm_ids[1], NULL, 0)) == (void *) -1) {
        print_error(SHARED_MEMORY, __func__);
        exit(1);
    }
    if ((matC = shmat(shm_ids[2], NULL, 0)) == (void *) -1) {
        print_error(SHARED_MEMORY, __func__);
        exit(1);
    }
    // creo il segmento per la somma
    if ((shm_ids[3] = shmget(IPC_PRIVATE, sizeof(int), 0666 | IPC_CREAT)) == -1) {
        print_error(SHARED_MEMORY, __func__);
        exit(1);
    }
    // attacco il segmento per la somma
    if ((somma = shmat(shm_ids[3], NULL, 0)) == (void *) -1) {
        print_error(SHARED_MEMORY, __func__);
        exit(1);
    }
    // inizializzo a 0 la somma
    (*somma) = 0;

    // creo il segmento per la variabile c -> contatore per sapere se sono stati fatti tutti i calcoli della moltiplicazione e somma
    if ((shm_ids[4] = shmget(IPC_PRIVATE, sizeof(int), 0666 | IPC_CREAT)) == -1) {
        print_error(SHARED_MEMORY, __func__);
        exit(1);
    }
    // attacco il segmento per la variabile c
    if ((c = shmat(shm_ids[4], NULL, 0)) == (void *) -1) {
        print_error(SHARED_MEMORY, __func__);
        exit(1);
    }
    // inizializzo a 0 la variabile c
    (*c) = 0;
    println("Memoria condivisa allocata");

    if ((msgid = msgget(IPC_PRIVATE, (0666 | IPC_CREAT | IPC_EXCL))) == -1) {
        print_error(MESSAGE_QUEUE, __func__);
        exit(1);
    }
}

void create_sem() {
    // creazione di un vettore di 1 semafori tra PADRE e FIGLIO
    if ((semid = semget(IPC_PRIVATE, 3, 0666 | IPC_EXCL | IPC_CREAT)) == -1) {
        print_error(SEMAPHORE, __func__);
        exit(1);
    }
    // inizializzazione semafori
    st_sem.val = 0;
    if (semctl(semid, VAR_PF, SETVAL, st_sem) == -1) {
        print_error(SEMAPHORE, __func__);
        exit(1);
    }
    st_sem.val = 1;
    if (semctl(semid, VAR_C, SETVAL, st_sem) == -1) {
        print_error(SEMAPHORE, __func__);
        exit(1);
    }
    st_sem.val = 1;
    if (semctl(semid, VAR_SOMMA, SETVAL, st_sem) == -1) {
        print_error(SEMAPHORE, __func__);
        exit(1);
    }
    println("Semafori creati e inizializzati");
}

void free_ipc(int signid) {
    // termino tutti i figli
    int x = 0;
    while (x < n_proc && pids[x] != 0) {
        //verifico che abbia ancora figli (già creati, vedi seconda condizione) da termianare
        kill(pids[x], SIGQUIT);
        //per vedere differenze SIGQUIT, SIGKILL, SIGINT, SIGTERM, vedere http://programmergamer.blogspot.it/2013/05/clarification-on-sigint-sigterm-sigkill.html
        x++;
    }

    // stacco i segmenti dal processo corrente
    shmdt(matA);
    shmdt(matB);
    shmdt(matC);
    shmdt(somma);
    shmdt(c);

    // libero i segmenti di memoria condivisa
    for (int i = 0; i < sizeof(shm_ids); i++)
        shmctl(shm_ids[i], IPC_RMID, NULL);
    switch (signid) {
        case SIGINT:
            print("SIGINT ricevuto - ");
            break;
        case SIGTERM:
            print("SIGTERM ricevuto - ");
            break;
        case SIGSEGV:
            print("SIGTERM ricevuto - ");
            break;
        default:
            print("Tutto ok - ");
    }
    // libero la coda di messaggi
    if(msgctl(msgid, IPC_RMID, NULL) == -1){
        print_error(MESSAGE_QUEUE, __func__);
        exit(1);
    }
    // libero i semafori
    if(semctl(semid, 0, IPC_RMID, 0)==-1){
        print_error(SEMAPHORE, __func__);
        exit(1);
    }
    free(pipes);
    free(pids);
    println("Memoria liberata e figli terminati");
}

void create_children() {
    for (int i = 0; i < n_proc; i++) {
        pipe(pipes[i]);
        pids[i] = fork();
        switch (pids[i]) {
            case -1:
                print_error(FILE_IO, __func__);
                exit(1);
                break;
            case 0: // codice figlio
                if(close(pipes[i][1])==-1){
                    print_error(FILE_IO, __func__);
                    exit(1);
                }
                id_child = i;
                figlio();
                exit(1);
            default: // codice padre
                if(close(pipes[i][0])==-1){
                    print_error(FILE_IO, __func__);
                    exit(1);
                }
                break;
        }
    }
    padre();
}

void padre() {
    message_t *msg_to_receive = (message_t *) malloc(sizeof(message_t));
    message_t *msg_to_send = (message_t *) malloc(sizeof(message_t));

    // procedo con la moltiplicazione
    for (int i = 0; i < dim; i++) {
        for (int k = 0; k < dim; k++) {
            if (msgrcv(msgid, msg_to_receive, sizeof(message_t) - sizeof(long), MSGTYPE, 0) == -1) {
                print_error(MESSAGE_QUEUE, __func__);
                free(msg_to_receive);
                exit(1);
            }
            msg_to_send->operation = 1;
            msg_to_send->row = i;
            msg_to_send->column = k;
            char *buf_msg_pipe = msg_to_buf(msg_to_send);
            write(pipes[msg_to_receive->id][1], buf_msg_pipe, strlen(buf_msg_pipe));
            free(buf_msg_pipe);
        }
    }

    if(sem_wait(semid, VAR_PF)==-1) { // aspetto che tutti i figli abbiano fatto la moltiplicazione
        print_error(SEMAPHORE, __func__);
        exit(1);
    }
    (*c) = 0; // imposto c a 0 per resettare il contatore operazioni
    // non usiamo il semaforo VAL_C perché sono sicuro che tutti i figli abbiano finito di fare la moltiplicazione e quindi di utilizzare c
    println("Moltiplicazione finita");

    // stampo matrice C
    println("Matrice C:");
    if (write_matrix(pathC, matC, dim) == 1) {
        print_error(FILE_IO, __func__);
        exit(1);
    }
    print_matrix(matC, dim);

    // procedo con la somma
    for (int i = 0; i < dim; i++) {
        if (msgrcv(msgid, msg_to_receive, sizeof(message_t) - sizeof(long), MSGTYPE, 0) == -1) {
            print_error(MESSAGE_QUEUE, __func__);
            free(msg_to_receive);
            exit(1);
        }
        msg_to_send->operation = 2;
        msg_to_send->row = i;
        msg_to_send->column = i; // lascio comunque un valore alla colonna, cosi non riscrivo le funzioni che vanno a creare str_pipe
        char *buf_msg_pipe = msg_to_buf(msg_to_send);
        if(write(pipes[msg_to_receive->id][1], buf_msg_pipe, strlen(buf_msg_pipe))==-1){
            print_error(FILE_IO, __func__);
            exit(1);
        }
        free(buf_msg_pipe);
    }
    if(sem_wait(semid, VAR_PF)==-1) { // aspetto che tutti i figli abbiano fatto la somma
        print_error(SEMAPHORE, __func__);
        exit(1);
    }
    // stampo la somma
    char buf_somma[20];
    sprintf(buf_somma, "%d", (*somma));
    print("La somma della matrice C e' ");
    println(buf_somma);

    // procediamo con la terminazione dei figli
    for (int i = 0; i < n_proc; i++) {
        if (msgrcv(msgid, msg_to_receive, sizeof(message_t) - sizeof(long), MSGTYPE, 0) == -1) {
            print_error(MESSAGE_QUEUE, __func__);
            free(msg_to_receive);
            exit(1);
        }
        msg_to_send->operation = -1;
        msg_to_send->row = i;
        msg_to_send->column = i; // lascio comunque un valore alla colonna, cosi non riscrivo le funzioni che vanno a creare str_pipe
        char *buf_msg_pipe = msg_to_buf(msg_to_send);
        if(write(pipes[msg_to_receive->id][1], buf_msg_pipe, strlen(buf_msg_pipe))==-1){
            print_error(FILE_IO, __func__);
            exit(1);
        }
        free(buf_msg_pipe);
    }
    println("Padre: Aspetto la morte dei miei figli");
    wait(NULL); // aspetta la terminazione di tutti i figli

    free(msg_to_send);
    free(msg_to_receive);
    free_ipc(0);
}

void figlio() {
    /*signal(SIGTERM, SIG_DFL); // tolgo le registrazioni delle funzioni, altrimenti ho solo doppioni
    signal(SIGSEGV, SIG_DFL);
    signal(SIGINT, SIG_DFL);*/

    message_t *msg_to_send = (message_t *) malloc(sizeof(message_t));
    message_t *msg_to_receive = NULL;
    char buf_msg_pipe[40];
    int have_finished = 0; /// have_finished = 1 allora esce dal ciclo infinito (se operation = -1)

    while (have_finished == 0) {
        msg_to_send->mtype = MSGTYPE;
        msg_to_send->id = id_child;
        msg_to_send->operation = 0; // 0 sta per sono libero, 1 moltiplicazione, 2 somma

        if (msgsnd(msgid, msg_to_send, sizeof(message_t) - sizeof(long), 0) == -1) {
            print_error(MESSAGE_QUEUE, __func__);
            free(msg_to_send);
            exit(1);
        }

        if(read(pipes[id_child][0], buf_msg_pipe, sizeof(buf_msg_pipe))==-1){
            print_error(FILE_IO, __func__);
            exit(1);
        }

        msg_to_receive = buf_to_msg(buf_msg_pipe);

        /// procedo con le operazioni
        if (msg_to_receive->operation == 1) { /// moltiplicazione
            int result = 0;
            for (int i = 0; i < dim; i++) {
                result = result +
                         (matA[(msg_to_receive->row) * dim + i] * matB[(msg_to_receive->column) + dim * i]);
            }
            // salvo il valore result nella matrice C
            matC[msg_to_receive->row * dim + msg_to_receive->column] = result;

            // vado a verificare se ho finito le operazioni
            if(sem_wait(semid, VAR_C)==-1){
                print_error(SEMAPHORE, __func__);
                exit(1);
            }
            if (is_maxcalc_goal(c, dim * dim, 1)) { // incremento il contatore di 1
                if(sem_signal(semid, VAR_PF)==-1) { // se il goal è raggiunto, si fa sbloccare il padre
                    print_error(SEMAPHORE, __func__);
                    exit(1);
                }
            }
            if(sem_signal(semid, VAR_C)==-1){
                print_error(SEMAPHORE, __func__);
                exit(1);
            }
        } else if (msg_to_receive->operation == 2) { /// somma
            int result = 0;
            for (int i = 0; i < dim; i++) {
                result = result + matC[(msg_to_receive->row) * dim + i];
            }
            // vado ad aggiungere la mia somma parziale 'result' con la somma vera e propria nella shm
            if(sem_wait(semid, VAR_SOMMA)==-1){
                print_error(SEMAPHORE, __func__);
                exit(1);
            }
            (*somma) = (*somma) + result;
            if(sem_signal(semid, VAR_SOMMA)==-1){
                print_error(SEMAPHORE, __func__);
                exit(1);
            }
            if(sem_wait(semid, VAR_C)==-1){
                print_error(SEMAPHORE, __func__);
                exit(1);
            }
            if (is_maxcalc_goal(c, dim, 1)) { // incremento il contatore di 1
                if(sem_signal(semid, VAR_PF)==-1){ // se il goal è raggiunto, si fa sbloccare il padre
                    print_error(SEMAPHORE, __func__);
                    exit(1);
                }
            }
            if(sem_signal(semid, VAR_C)==-1){
                print_error(SEMAPHORE, __func__);
                exit(1);
            }
        } else if (msg_to_receive->operation == -1) {
            // terminati ma prima libera le varie cose (lo fa dopo questi if)
            have_finished = 1;
        }
        /* ora se mi hanno detto di terminare esco,
         * altrimenti ricomincio il ciclo, mando messaggio con operation = 0 per dire che sono pronto,
         * mi metto a leggere sulla pipe cosi mi fermo
         */
        free(msg_to_receive);
    }
    free(msg_to_send);
}
