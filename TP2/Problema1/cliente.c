#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "comun.h"

int main(int argc, char *argv[]) {
    key_t clave;
    int msqid;
    struct mensaje msg;
    size_t tam_datos = sizeof(struct mensaje) - sizeof(long);
    pid_t mi_pid = getpid();

    /* 1. Validar cantidad de argumentos: ./cliente OPERACION OP1 OP2 */
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <SUMA|RESTA|MULT> <op1> <op2>\n", argv[0]);
        exit(1);
    }

    /* 2. Generar la misma clave que el servidor */
    clave = ftok(RUTA_CLAVE, PROJ_ID);
    if (clave == -1) {
        perror("ftok");
        exit(1);
    }

    /* 3. Conectarse a la cola ya creada por el servidor (sin IPC_CREAT) */
    msqid = msgget(clave, 0666);
    if (msqid == -1) {
        perror("msgget (asegurate de que ./servidor este corriendo)");
        exit(1);
    }

    /* 4. Armar la peticion */
    msg.mtype = TIPO_PETICION;
    msg.pid_cliente = mi_pid;
    strncpy(msg.operacion, argv[1], sizeof(msg.operacion) - 1);
    msg.operacion[sizeof(msg.operacion) - 1] = '\0';
    msg.op1 = atoi(argv[2]);
    msg.op2 = atoi(argv[3]);
    msg.resultado = 0;
    msg.error = 0;

    /* 5. Enviar peticion al servidor */
    if (msgsnd(msqid, &msg, tam_datos, 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    /* 6. Esperar UNICAMENTE la respuesta dirigida a nuestro PID */
    if (msgrcv(msqid, &msg, tam_datos, (long)mi_pid, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }

    /* 7. Mostrar el resultado */
    if (msg.error) {
        fprintf(stderr, "Error: operacion invalida '%s'. Use SUMA, RESTA o MULT.\n", argv[1]);
        exit(1);
    }

    printf("Resultado: %d\n", msg.resultado);
    return 0;
}