#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define RUTA_CLAVE "."
#define PROJ_ID 'B'
#define TAM_SECUENCIA 10

/* Indices dentro del conjunto de 2 semaforos */
#define SEM_HIJO1 0
#define SEM_HIJO2 1

/* Estructura alojada en memoria compartida */
struct datos_compartidos {
    int secuencia[TAM_SECUENCIA];
    int indice;
};

/* Union requerida por semctl() en Linux */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

/* Operacion down (-1): espera si el semaforo vale 0 */
void sem_down(int semid, int num_sem) {
    struct sembuf op;
    op.sem_num = num_sem;
    op.sem_op = -1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("semop down");
        exit(1);
    }
}

/* Operacion up (+1): habilita al otro proceso */
void sem_up(int semid, int num_sem) {
    struct sembuf op;
    op.sem_num = num_sem;
    op.sem_op = 1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("semop up");
        exit(1);
    }
}

int main(void) {
    key_t clave;
    int shmid, semid;
    struct datos_compartidos *shm;
    union semun arg;
    pid_t pid1, pid2;

    /* 1. Generar clave IPC */
    clave = ftok(RUTA_CLAVE, PROJ_ID);
    if (clave == -1) {
        perror("ftok");
        exit(1);
    }

    /* 2. Crear segmento de memoria compartida */
    shmid = shmget(clave, sizeof(struct datos_compartidos), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    /* 3. Adjuntar memoria compartida al proceso padre */
    shm = (struct datos_compartidos *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }
    shm->indice = 0;

    /* 4. Crear conjunto de 2 semaforos */
    semid = semget(clave, 2, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }

    /* 5. Inicializar semaforos: Hijo 1 arranca en 1, Hijo 2 en 0 */
    arg.val = 1;
    if (semctl(semid, SEM_HIJO1, SETVAL, arg) == -1) {
        perror("semctl SEM_HIJO1");
        exit(1);
    }
    arg.val = 0;
    if (semctl(semid, SEM_HIJO2, SETVAL, arg) == -1) {
        perror("semctl SEM_HIJO2");
        exit(1);
    }

    /* 6. Crear Hijo 1 (impares: 1, 3, 5, 7, 9) */
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork hijo 1");
        exit(1);
    }
    if (pid1 == 0) {
        for (int val = 1; val <= 9; val += 2) {
            sem_down(semid, SEM_HIJO1);
            shm->secuencia[shm->indice] = val;
            shm->indice++;
            sem_up(semid, SEM_HIJO2);
        }
        shmdt(shm);
        exit(0);
    }

    /* 7. Crear Hijo 2 (pares: 2, 4, 6, 8, 10) */
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork hijo 2");
        exit(1);
    }
    if (pid2 == 0) {
        for (int val = 2; val <= 10; val += 2) {
            sem_down(semid, SEM_HIJO2);
            shm->secuencia[shm->indice] = val;
            shm->indice++;
            sem_up(semid, SEM_HIJO1);
        }
        shmdt(shm);
        exit(0);
    }

    /* 8. El padre espera a que terminen ambos hijos */
    if (waitpid(pid1, NULL, 0) == -1) perror("waitpid 1");
    if (waitpid(pid2, NULL, 0) == -1) perror("waitpid 2");

    /* 9. Mostrar la secuencia final */
    printf("Secuencia final:");
    for (int i = 0; i < TAM_SECUENCIA; i++) {
        printf(" %d", shm->secuencia[i]);
    }
    printf("\n");

    /* 10. Desadjuntar y liberar recursos IPC */
    if (shmdt(shm) == -1) perror("shmdt");
    if (shmctl(shmid, IPC_RMID, NULL) == -1) perror("shmctl IPC_RMID");
    if (semctl(semid, 0, IPC_RMID) == -1) perror("semctl IPC_RMID");

    return 0;
}