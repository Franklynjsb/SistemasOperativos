#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define RUTA_CLAVE "."
#define PROJ_ID 'C'

#define TAM_BUFFER 5
#define TOTAL_ELEMENTOS 10

/* Indices del conjunto de 3 semaforos */
#define SEM_MUTEX 0
#define SEM_LLENO 1
#define SEM_VACIO 2

/* Buffer circular en memoria compartida */
struct buffer_circular {
    int datos[TAM_BUFFER];
    int in;
    int out;
    int count;
};

/* Union requerida por semctl() en Linux */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void inicializar_sem(int semid, int num_sem, int valor) {
    union semun arg;
    arg.val = valor;
    if (semctl(semid, num_sem, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        exit(1);
    }
}

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
    struct buffer_circular *buf;
    pid_t pid;

    /* Evita que se mezclen lineas de stdout entre padre e hijo */
    setvbuf(stdout, NULL, _IOLBF, 0);

    /*Generar clave IPC */
    clave = ftok(RUTA_CLAVE, PROJ_ID);
    if (clave == -1) {
        perror("ftok");
        exit(1);
    }

    /* Crear y adjuntar segmento de memoria compartida */
    shmid = shmget(clave, sizeof(struct buffer_circular), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    buf = (struct buffer_circular *)shmat(shmid, NULL, 0);
    if (buf == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }

    /* Inicializar indices del buffer circular */
    buf->in = 0;
    buf->out = 0;
    buf->count = 0;

    /* Crear e inicializar el conjunto de 3 semaforos */
    semid = semget(clave, 3, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        shmdt(buf);
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }

    inicializar_sem(semid, SEM_MUTEX, 1);          /* Exclusion mutua */
    inicializar_sem(semid, SEM_LLENO, 0);          /* 0 elementos disponibles */
    inicializar_sem(semid, SEM_VACIO, TAM_BUFFER); /* 5 lugares libres */

    /* Crear procesos: hijo = Productor, padre = Consumidor */
    pid = fork();
    if (pid == -1) {
        perror("fork");
        shmdt(buf);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        exit(1);
    }

    if (pid == 0) {
        /* Proceso hijo: PRODUCTOR (genera 10, 20, ..., 100) */
        for (int i = 1; i <= TOTAL_ELEMENTOS; i++) {
            int valor = i * 10;

            sem_down(semid, SEM_VACIO); /* Espera lugar libre */
            sem_down(semid, SEM_MUTEX); /* Entra a region critica */

            buf->datos[buf->in] = valor;
            buf->in = (buf->in + 1) % TAM_BUFFER;
            buf->count++;

            sem_up(semid, SEM_MUTEX);   /* Sale de region critica */
            sem_up(semid, SEM_LLENO);   /* Avisa que hay un elemento mas */
        }

        if (shmdt(buf) == -1) perror("shmdt productor");
        exit(0);
    } else {
        /* Proceso padre: CONSUMIDOR */
        int consumidos = 0;
        int suma = 0;

        for (int i = 0; i < TOTAL_ELEMENTOS; i++) {
            int valor;

            sem_down(semid, SEM_LLENO); /* Espera elemento disponible */
            sem_down(semid, SEM_MUTEX); /* Entra a region critica */

            valor = buf->datos[buf->out];
            buf->out = (buf->out + 1) % TAM_BUFFER;
            buf->count--;

            sem_up(semid, SEM_MUTEX);   /* Sale de region critica */
            sem_up(semid, SEM_VACIO);   /* Avisa que hay un lugar libre mas */

            printf("Consumido: %d\n", valor);
            consumidos++;
            suma += valor;
        }

        printf("Elementos consumidos: %d\n", consumidos);
        printf("Suma: %d\n", suma);

        /* Esperar al productor y liberar recursos IPC */
        if (waitpid(pid, NULL, 0) == -1) perror("waitpid");
        if (shmdt(buf) == -1) perror("shmdt consumidor");
        if (shmctl(shmid, IPC_RMID, NULL) == -1) perror("shmctl IPC_RMID");
        if (semctl(semid, 0, IPC_RMID) == -1) perror("semctl IPC_RMID");
    }

    return 0;
}
