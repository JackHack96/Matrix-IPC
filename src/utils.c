/**
 * @file
 * Libreria contenente le implementazioni delle funzioni dichiarate in utils.h
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sem.h>
#include "../inc/utils.h"

void print_error(error_type er, const char function_name[]) {
    char error_msg[200];
    switch (er) {
        case SHARED_MEMORY:
            sprintf(error_msg, "Errore durante la gestione della memoria condivisa nella funzione %s",
                    function_name);
            break;
        case FILE_IO:
            sprintf(error_msg, "Errore durante un'operazione di I/O nella funzione %s", function_name);
            break;
        case MESSAGE_QUEUE:
            sprintf(error_msg, "Errore durante la gestione della coda di messaggi nella funzione %s", function_name);
            break;
        case SEMAPHORE:
            sprintf(error_msg, "Errore durante la gestione dei semafori nella funzione %s", function_name);
            break;
        default:
            sprintf(error_msg, "Errore non classificato nella funzione %s", function_name);
            break;
    }
    println(error_msg);
}

int sem_wait(int semid, int sem_number) {
    struct sembuf wait_b = {
            .sem_num=(unsigned short) sem_number,
            .sem_op=-1,
            .sem_flg=0
    };

    if (semop(semid, &wait_b, 1) == -1)
        return 1;
    return 0;
}

int sem_signal(int semid, int sem_number) {
    struct sembuf signal_b = {
            .sem_num=(unsigned short) sem_number,
            .sem_op=1,
            .sem_flg=0
    };

    if (semop(semid, &signal_b, 1) == -1)
        return 1;
    return 0;
}

int is_maxcalc_goal(int *c, int goal, int op) {
    (*c) = (*c) + op;
    if ((*c) == goal)
        return 1;
    else
        return 0;
}

void println(char *str) {
    if(write(1, str, strlen(str))==-1){
        print_error(FILE_IO, __func__);
        exit(1);
    }
    if(write(1, "\n", 1)==-1){
        print_error(FILE_IO, __func__);
        exit(1);
    }
}

void print(char *str) {
    if(write(1, str, strlen(str))==-1){
        print_error(FILE_IO, __func__);
        exit(1);
    }
}

void print_usage() {
    println("Uso del programma:");
    println("./ElaboratoC [MATRICE A] [MATRICE B] [MATRICE C] [ORDINE MATRICE] [NUMERO PROCESSI]\n");
    println("Il parametro [MATRICE A] contiene la prima matrice, che deve essere moltiplicata");
    println("per il parametro [MATRICE B] e il risultato deve venire salvato nel file [MATRICE C]\n");
    println("Il parametro [ORDINE MATRICE] deve indicare l'ordine (dimensione) della matrice\n");
    println("Il parametro [NUMERO PROCESSI] deve indicare in quanti processi dividere il calcolo (escluso il padre)\n");
}

int check_args(int argc, char *argv[]) {
    if (argc != 6) {
        println("Numero di argomenti errato!");
        print_usage();
        return 1;
    }
    if (access(argv[1], R_OK | W_OK) != 0) {
        println("Errore durante l'apertura del file [MATRICE A]");
        return 1;
    }
    if (access(argv[2], R_OK | W_OK) != 0) {
        println("Errore durante l'apertura del file [MATRICE B]");
        return 1;
    }
    if (atoi(argv[4]) <= 0) {
        println("La dimensione della matrice non puo' essere <= 0");
        return 1;
    }
    if (atoi(argv[5]) <= 0) {
        println("Il numero di processi non puo' essere <= 0");
        return 1;
    }
    return 0;
}
