/*
 * procesa_datos.c - TP1 Sistemas Operativos UTN FRSF 2026
 * Uso: ./procesa_datos <archivo> <area>
 *
 * Problema 5: hijo 1 cuenta registros del area y los envia al padre por pipe.
 * Problema 6: hijo 2 ejecuta grep (exec) con stdout redirigido por dup2()
 *             hacia resultados/exec_resultado.txt.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFSZ 4096
#define LINEMAX 256

/* close() verificado: informa con perror() si falla. */
static void cerrar(int fd, const char *que)
{
    if (close(fd) == -1)
        perror(que);
}

/* Cuenta registros cuyo campo AREA coincide con ";<area>;" */
static long contar_area(const char *ruta, const char *area)
{
    char patron[32];
    snprintf(patron, sizeof(patron), ";%s;", area);

    int fd = open(ruta, O_RDONLY);
    if (fd == -1) {
        perror("open (hijo 1)");
        return -1;
    }

    char buf[BUFSZ];
    char linea[LINEMAX];
    size_t llen = 0;
    long cuenta = 0;
    ssize_t n, i;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                linea[llen] = '\0';
                if (strstr(linea, patron) != NULL)
                    cuenta++;
                llen = 0;
            } else if (llen < LINEMAX - 1) {
                linea[llen++] = buf[i];
            }
        }
    }
    if (n == -1) {
        perror("read (hijo 1)");
        cerrar(fd, "close");
        return -1;
    }
    cerrar(fd, "close");
    return cuenta;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo> <area>\n", argv[0]);
        return 1;
    }

    const char *ruta = argv[1];
    const char *area = argv[2];

    /* ---- Validacion de parametros ---- */
    if (strcmp(area, "AREA0") != 0 && strcmp(area, "AREA1") != 0 &&
        strcmp(area, "AREA2") != 0) {
        fprintf(stderr, "ERROR: area invalida '%s' (AREA0, AREA1 o AREA2)\n", area);
        return 1;
    }
    struct stat st;
    if (stat(ruta, &st) == -1) {
        perror("stat");
        return 1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "ERROR: '%s' no es un archivo regular\n", ruta);
        return 1;
    }
    if (mkdir("resultados", 0755) == -1 && errno != EEXIST) {
        perror("mkdir resultados");
        return 1;
    }

    /* ================= Problema 5: pipe + fork ================= */
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        cerrar(fd[0], "close");
        cerrar(fd[1], "close");
        return 1;
    }

    if (pid1 == 0) {
        /* ---------- HIJO 1 ---------- */
        cerrar(fd[0], "close");                       /* extremo de lectura: no se usa */
        long cuenta = contar_area(ruta, area);
        if (cuenta < 0) {
            cerrar(fd[1], "close");
            _exit(1);
        }
        if (write(fd[1], &cuenta, sizeof(cuenta)) != (ssize_t)sizeof(cuenta)) {
            perror("write (pipe)");
            cerrar(fd[1], "close");
            _exit(1);
        }
        printf("Hijo 1: PID=%d, PPID=%d\n", (int)getpid(), (int)getppid());
        fflush(stdout);
        cerrar(fd[1], "close");
        _exit(0);
    }

    /* ---------- PADRE ---------- */
    cerrar(fd[1], "close");                           /* extremo de escritura: no se usa */
    long recibido = 0;
    ssize_t r = read(fd[0], &recibido, sizeof(recibido));
    if (r == -1) {
        perror("read (pipe)");
    } else if (r != (ssize_t)sizeof(recibido)) {
        fprintf(stderr, "ERROR: el hijo 1 no envio el resultado completo\n");
    }
    cerrar(fd[0], "close");

    int status1;
    if (waitpid(pid1, &status1, 0) == -1) {
        perror("waitpid (hijo 1)");
        return 1;
    }
    if (r == (ssize_t)sizeof(recibido))
        printf("Cantidad recibida del hijo: %ld\n", recibido);
    if (!WIFEXITED(status1) || WEXITSTATUS(status1) != 0) {
        fprintf(stderr, "ERROR: el hijo 1 finalizo con error\n");
        return 1;
    }

    /* ================= Problema 6: exec + dup2 ================= */
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork (hijo 2)");
        return 1;
    }

    if (pid2 == 0) {
        /* ---------- HIJO 2 ---------- */
        int fout = open("resultados/exec_resultado.txt",
                        O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fout == -1) {
            perror("open exec_resultado.txt");
            _exit(1);
        }
        if (dup2(fout, STDOUT_FILENO) == -1) {   /* stdout -> archivo */
            perror("dup2");
            cerrar(fout, "close");
            _exit(1);
        }
        cerrar(fout, "close");                        /* ya duplicado en fd 1 */

        char patron[32];
        snprintf(patron, sizeof(patron), ";%s;", area);
        execlp("grep", "grep", "-F", patron, ruta, (char *)NULL);

        /* Solo se llega aqui si exec fallo */
        perror("execlp grep");
        _exit(127);
    }

    int status2;
    if (waitpid(pid2, &status2, 0) == -1) {
        perror("waitpid (hijo 2)");
        return 1;
    }
    if (WIFEXITED(status2) && WEXITSTATUS(status2) == 0) {
        printf("Hijo 2 (exec grep) finalizo correctamente. "
               "Salida en resultados/exec_resultado.txt\n");
    } else {
        fprintf(stderr, "ERROR: el hijo 2 (exec) finalizo con error (status=%d)\n",
                WIFEXITED(status2) ? WEXITSTATUS(status2) : -1);
        return 1;
    }

    return 0;
}
