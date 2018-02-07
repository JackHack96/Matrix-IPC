/** @file
 * Libreria contenente le dichiarazioni delle funzioni per la gestione delle
 * matrici
 */

#ifndef MATRIX_H
#define MATRIX_H

#define MSGTYPE 1

/**
 * Legge la matrice dal file e la carica sul buffer
 * @param path Percorso del file
 * @param dim Ordine della matrice
 * @return Il puntatore alla matrice, 1 altrimenti
 */
int *load_matrix(char *path, int dim);

/**
 * Scrive la matrice nel file
 * @param path Percorso del file
 * @param buf Matrice da salvare
 * @param dim Ordine della matrice
 * @return 0 in caso di successo, 1 altrimenti
 */
int write_matrix(char *path, int *buf, int dim);

/**
 * Stampa a video la matrice
 * @param buf Matrice da stampare
 * @param dim Ordine della matrice
 */
void print_matrix(int *buf, int dim);

/**
 * Copia la matrice dal buffer alla memoria condivisa
 * @param buf Matrice in memoria locale di dimensione [ordine][ordine]
 * @param dim L'ordine della matrice quadrata
 * @param mat Matrice nella memoria condivisa
 */
void copy_matrix_on_shm(int *buf, int dim, int *mat);

/**
 * Struttura del messaggio per le operazioni
 */
typedef struct {
    long mtype;
    int row;
    int column;
    int operation;
    int id;
} message_t;

/**
 * Converte la stringa data in una struttura
 * @param buf Stringa da convertire
 * @return La struttura message_t convertita
 */
message_t *buf_to_msg(char *buf);

/**
 * Converte la struttura data in un messaggio per la pipe
 * @param mes Struttura message_t da convertire
 * @return La string convertita
 */
char *msg_to_buf(message_t *mes);

#endif // MATRIX_H