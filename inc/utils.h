/**
 * @file
 * Libreria contenente le dichiarazioni di varie funzioni
 */
#ifndef ELABORATOC_UTILS_H_H
#define ELABORATOC_UTILS_H_H

/* definizioni numeri dei semafori */
#define VAR_PF 0
#define VAR_C 1
#define VAR_SOMMA 2

/// Enumerazione per la funzione print_error()
typedef enum {
    SHARED_MEMORY, FILE_IO, MESSAGE_QUEUE, SEMAPHORE
} error_type;

/// Union per la gestione del controllo sui semafori
union semun {
    int val;
    struct semid_ds *buf;
    short *array;
} st_sem;

/**
 * Stampa la stringa data e aggiunge un newline
 * @param str Stringa da stampare
 */
void println(char *str);

/**
 * Stampa la stringa data senza aggiungere newline
 * @param str Stringa da stampare
 */
void print(char *str);

/**
 * Compone automaticamente un messaggio di errore e lo stampa
 */
void print_error(error_type er, const char function_name[]);

/**
 * Stampa l'uso del programma
 */
void print_usage();

/**
 * Decrementa il valore del semaforo (e blocca, come P())
 * @param semid ID dell'array semafori
 * @param sem_number Numero del semaforo
 * @return 0 in caso di successo, 1 altrimenti
 */
int sem_wait(int semid, int sem_number);

/**
 * Incrementa il valore del semaforo (come V())
 * @param semid ID dell'array semafori
 * @param sem_number Numero del semaforo
 * @return 0 in caso di successo, 1 altrimenti
 */
int sem_signal(int semid, int sem_number);

int is_maxcalc_goal(int *c, int dim, int op);

/**
 * Controlla che gli argomenti siano corretti
 * @param argv Array contenente gli argomenti
 * @return 0 se tutto è a posto, 1 altrimenti
 */
int check_args(int argc, char *argv[]);

#endif //ELABORATOC_UTILS_H_H
