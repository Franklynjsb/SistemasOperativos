#ifndef COMUN_H
#define COMUN_H

#include <sys/types.h>

#define RUTA_CLAVE "."
#define PROJ_ID 'A'
#define TIPO_PETICION 1

struct mensaje {
    long mtype;           /* Obligatorio en System V: 1 para servidor, PID para cliente */
    pid_t pid_cliente;    /* PID del cliente que consulta */
    char operacion[10];   /* "SUMA", "RESTA" o "MULT" */
    int op1;
    int op2;
    int resultado;
    int error;            /* 0 = OK, 1 = operacion invalida */
};

#endif