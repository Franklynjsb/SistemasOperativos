/*
 * analiza_archivo.c - TP1 Sistemas Operativos UTN FRSF 2026
 * Uso: ./analiza_archivo <archivo>
 *
 * Analiza un archivo usando exclusivamente llamadas al sistema para
 * manipularlo: open(), read(), write(), close(), stat(), lseek().
 * (No se usan fopen/fread/fwrite/fseek/fprintf/fclose.)
 */
#include <stdio.h>      /* snprintf, perror */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFSZ 4096
#define PRIMEROS 20

/* close() verificado: informa con perror() si falla. */
static void cerrar(int fd, const char *que)
{
    if (close(fd) == -1)
        perror(que);
}

/* Escribe exactamente len bytes en fd (maneja escrituras parciales). */
static int escribir_todo(int fd, const char *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t n = write(fd, buf + total, len - total);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        const char *uso = "Uso: ./analiza_archivo <archivo>\n";
        if (escribir_todo(STDERR_FILENO, uso, strlen(uso)) == -1)
            perror("write (stderr)");
        return 1;
    }

    const char *ruta = argv[1];
    struct stat st;

    /* 1) Tamano del archivo mediante stat() */
    if (stat(ruta, &st) == -1) {
        perror("stat");
        return 1;
    }
    if (!S_ISREG(st.st_mode)) {
        const char *msg = "ERROR: el parametro no es un archivo regular\n";
        if (escribir_todo(STDERR_FILENO, msg, strlen(msg)) == -1)
            perror("write (stderr)");
        return 1;
    }

    int fd = open(ruta, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* 2) Recorrido completo del archivo con read() */
    char buf[BUFSZ];
    long registros = 0, puntoycoma = 0, letraA = 0;
    ssize_t n, i;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < n; i++) {
            if (buf[i] == '\n')
                registros++;
            else if (buf[i] == ';')
                puntoycoma++;
            else if (buf[i] == 'A')
                letraA++;
        }
    }
    if (n == -1) {
        perror("read");
        cerrar(fd, "close");
        return 1;
    }

    /* 3) Armado del reporte */
    char reporte[512];
    int len = snprintf(reporte, sizeof(reporte),
                       "TAMANO_BYTES=%lld\n"
                       "REGISTROS=%ld\n"
                       "PUNTO_Y_COMA=%ld\n"
                       "LETRA_A=%ld\n",
                       (long long)st.st_size, registros, puntoycoma, letraA);

    /* Pantalla */
    if (escribir_todo(STDOUT_FILENO, reporte, (size_t)len) == -1) {
        perror("write (stdout)");
        cerrar(fd, "close");
        return 1;
    }

    /* Archivo resultados/analisis_c.txt (se crea el directorio si falta) */
    if (mkdir("resultados", 0755) == -1 && errno != EEXIST) {
        perror("mkdir resultados");
        cerrar(fd, "close");
        return 1;
    }
    int fout = open("resultados/analisis_c.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fout == -1) {
        perror("open resultados/analisis_c.txt");
        cerrar(fd, "close");
        return 1;
    }
    if (escribir_todo(fout, reporte, (size_t)len) == -1) {
        perror("write (analisis_c.txt)");
        cerrar(fout, "close");
        cerrar(fd, "close");
        return 1;
    }
    if (close(fout) == -1) {
        perror("close analisis_c.txt");
        cerrar(fd, "close");
        return 1;
    }

    /* 4) lseek(): volver al comienzo y leer los primeros 20 bytes */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        cerrar(fd, "close");
        return 1;
    }

    char primeros[PRIMEROS];
    size_t leidos = 0;
    while (leidos < PRIMEROS) {
        n = read(fd, primeros + leidos, PRIMEROS - leidos);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("read (primeros 20 bytes)");
            cerrar(fd, "close");
            return 1;
        }
        if (n == 0)
            break; /* EOF: archivo mas corto que 20 bytes */
        leidos += (size_t)n;
    }

    int f20 = open("resultados/primeros20.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f20 == -1) {
        perror("open resultados/primeros20.txt");
        cerrar(fd, "close");
        return 1;
    }
    if (escribir_todo(f20, primeros, leidos) == -1) {
        perror("write (primeros20.txt)");
        cerrar(f20, "close");
        cerrar(fd, "close");
        return 1;
    }
    if (close(f20) == -1) {
        perror("close primeros20.txt");
        cerrar(fd, "close");
        return 1;
    }

    if (close(fd) == -1) {
        perror("close");
        return 1;
    }
    return 0;
}
