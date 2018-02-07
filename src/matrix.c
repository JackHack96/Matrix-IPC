/**
 * @file
 * Contiene le implementazioni delle funzioni dichiarate in matrix.h *
 */
#include "../inc/matrix.h"
#include "../inc/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 512

int *load_matrix(char *path, int dim) {
    int file_id;
    char line[MAX_LINE_LENGTH];
    int row = 0;
    int *buf = malloc(sizeof(int) * dim * dim);

    if ((file_id = open(path, O_RDONLY)) < 0) {
        free(buf);
        return (int *) 1;
    }

    while (row < dim) {
        // legge una riga dal file
        int index = 0;
        char ch[1];
        while (read(file_id, ch, 1) > 0) {
            if (ch[0] != '\n')
                line[index++] = ch[0];
            else
                break;
        }
        // parserizza la riga
        char *str = strtok(line, ",");
        for (int col = 0; col < dim; col++) {
            buf[(row * dim) + col] = (int) strtol(str, NULL, 10);
            str = strtok(NULL, ",");
        }
        row++;
        memset(line, 0, sizeof(line)); // resetta il buffer
    }
    if(close(file_id)==-1){
        print_error(FILE_IO, __func__);
        exit(1);
    }
    return buf;
}

int write_matrix(char *path, int *buf, int dim) {
    int file_id;
    char line[MAX_LINE_LENGTH], num[10];

    // crea il file (eventualmente lo sovrascrive)
    if ((file_id = open(path, O_WRONLY | O_CREAT,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0)
        return 1;
    if(memset(line, 0, sizeof(line))==(void*)-1){ // resetta il buffer
        print_error(FILE_IO, __func__);
        exit(1);
    }

    for (int row = 0; row < dim; row++) {
        for (int col = 0; col < dim; col++) {
            sprintf(num, "%d,", buf[(row * dim) + col]);
            if (strlen(line) + strlen(num) > MAX_LINE_LENGTH)
                return 1;
            else
                strncat(line, num, strlen(num));
        }
        line[strlen(line) - 1] = '\n'; // crea la riga da scrivere e mette il newline
        if(write(file_id, line, strlen(line))==-1){
            print_error(FILE_IO, __func__);
            exit(1);
        }
        if(memset(line, 0, sizeof(line))==(void*)-1) { // resetta il buffer
            print_error(FILE_IO, __func__);
            exit(1);
        }
    }
    if(close(file_id)==-1){
        print_error(FILE_IO, __func__);
        exit(1);
    }
    return 0;
}

void print_matrix(int *buf, int dim) {
    char buffer[MAX_LINE_LENGTH] = "", num[10]; // buffer per la riga e per il singolo numero
    for (int row = 0; row < dim; row++) {
        for (int col = 0; col < dim; col++) {
            sprintf(num, "%d\t", buf[row * dim + col]);
            strncat(buffer, num, strlen(num));
        }
        println(buffer);
        buffer[0] = '\0'; // resetto la stringa
    }
}

void copy_matrix_on_shm(int *buf, int dim, int *mat) {
    for (int row = 0; row < dim; row++) {
        for (int col = 0; col < dim; col++) {
            mat[row * dim + col] =
                    buf[row * dim + col]; // buf[row + col * dim]; se si vuole scriverla
            // speculata rispetto alla diagonale
        }
    }
}

char *msg_to_buf(message_t *mes) {
    char *buf = malloc(sizeof(char) * 40);
    sprintf(buf, "%d,%d,%d", mes->operation, mes->row, mes->column);
    return buf;
}

message_t *buf_to_msg(char *buf) {
    message_t *mes = (message_t *) malloc(sizeof(message_t));
    char *str = strtok(buf, ",");
    mes->operation = atoi(str);
    str = strtok(NULL, ",");
    mes->row = atoi(str);
    str = strtok(NULL, ",");
    mes->column = atoi(str);
    return mes;
}
