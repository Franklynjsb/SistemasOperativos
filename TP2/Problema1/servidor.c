#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "comun.h"

/* Bandera para salir del bucle al presionar Ctrl+C */
volatile sig_atomic_t seguir = 1;

void manejar_senial(int sig) {
    (void)sig;
    seguir = 0;
}

int main(void) {
    key_t clave;
    int msqid;
    struct mensaje msg;
    size_t tam_datos = sizeof(struct mensaje) - sizeof(long);

    /* Configurar Ctrl+C (SIGINT) para que interrumpa msgrcv() y limpie la cola */
    struct sigaction sa;
    sa.sa_handler = manejar_senial;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    /* Generar la clave a partir de la carpeta actual y la letra 'A' */
    clave = ftok(RUTA_CLAVE, PROJ_ID);
    if (clave == -1) {
        perror("ftok");
        exit(1);
    }

    /* Crear la cola de mensajes (o abrirla si ya existe) */
    msqid = msgget(clave, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Servidor iniciado (msqid = %d). Esperando peticiones... (Ctrl+C para salir)\n", msqid);

    /* Bucle principal: recibir peticiones, calcular y responder */
    while (seguir) {
        if (msgrcv(msqid, &msg, tam_datos, TIPO_PETICION, 0) == -1) {
            if (errno == EINTR) {
                break; /* Interrumpido por Ctrl+C: salimos ordenadamente */
            }
            perror("msgrcv");
            break;
        }

        msg.error = 0;
        if (strcmp(msg.operacion, "SUMA") == 0) {
            msg.resultado = msg.op1 + msg.op2;
        } else if (strcmp(msg.operacion, "RESTA") == 0) {
            msg.resultado = msg.op1 - msg.op2;
        } else if (strcmp(msg.operacion, "MULT") == 0) {
            msg.resultado = msg.op1 * msg.op2;
        } else {
            msg.error = 1;
            msg.resultado = 0;
        }

        /* Responder usando el PID del cliente como tipo de mensaje */
        msg.mtype = (long)msg.pid_cliente;

        if (msgsnd(msqid, &msg, tam_datos, 0) == -1) {
            perror("msgsnd");
        }
    }

    /* Eliminar la cola de mensajes antes de terminar */
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    printf("\nServidor finalizado y cola eliminada correctamente.\n");
    return 0;
}
