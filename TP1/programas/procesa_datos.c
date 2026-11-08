#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // Para _exit(), pipe(), fork(), etc.
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h> // Para stat() y S_ISREG
#include <errno.h>    // Para EEXIST

int main (int argc, char *argv[]){
	if (argc != 3){
		fprintf(stderr, "Uso incorrecto del ejecutable, se espera archivo seguido de area\n");
		return 1;
	}

	if(strcmp(argv[2], "AREA0") != 0 && strcmp(argv[2], "AREA1")!=0 && strcmp(argv[2], "AREA2")!= 0){
		fprintf(stderr, "Error, area incorrecta\n");
		return 1;
	}

	// Validar si es archivo regular
	struct stat st;
	if (stat(argv[1], &st) == -1) {
		perror("Error en stat()");
		return 1;
	}
	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "ERROR: el parámetro no es un archivo regular\n");
		return 1;
	}

	// Crear directorio resultados si no existe
	if (mkdir("../resultados", 0755) == -1 && errno != EEXIST) {
		perror("Advertencia: problema al crear el directorio resultados");
	}

	//Armar patrón exacto para evitar contar falsos positivos
	char patron[32];
	snprintf(patron, sizeof(patron), ";%s;", argv[2]);

	int pipefd[2];
	if(pipe(pipefd) == -1){
		perror("Error al crear la tuberia con pipe()");
		return 1;
	}

	pid_t pid1=fork();
	if(pid1 == -1) {
		perror("Error al realizar el fork() del hijo 1");
		return 1;
	}

	// HIJO 1
	if (pid1 == 0){
		close(pipefd[0]);
		printf("HIJO1 PID: %d, PPID: %d\n", getpid(), getppid());
		fflush(stdout);

		int fd = open(argv[1], O_RDONLY);
		if (fd == -1){
			perror("Error al abrir el archivo en hijo 1");
			_exit(1); // Usar _exit() para no usar return
		}

		int cantidad=0;
		char buffer[1024];
		char linea[256];
		int idx = 0;
		ssize_t bytes;

		while ((bytes = read(fd, buffer, sizeof(buffer))) > 0){
			for(ssize_t i=0; i<bytes; i++){
				if(buffer[i] == '\n'){
					linea[idx] = '\0';
					// Buscar el patrón delimitado (ej. ";AREA1;")
					if (strstr(linea, patron) != NULL){
						cantidad++;
					}
					idx=0;
				}else {
					if (idx < (int)sizeof(linea) -1) {
						linea[idx++] = buffer[i];
					}
				}
			}
		}
		if (bytes == -1) {
			perror("Error al leer el archivo en hijo 1");
			close(fd);
			_exit(1);
		}
		close(fd);

		if (write(pipefd[1], &cantidad, sizeof(cantidad)) != sizeof(cantidad)) {
			perror("Error al escribir en la tuberia");
			_exit(1);
		}
		close(pipefd[1]);
		_exit(0); // Fin correcto del hijo
	}

	pid_t pid2 = fork();
	if (pid2 == -1) {
		perror("Error al ejecutar fork() para el hijo 2");
		return 1;
	}

	// HIJO 2
	if (pid2 == 0) {
		close(pipefd[0]);
		close(pipefd[1]);

		int fd_out = open("../resultados/exec_resultado.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd_out == -1){
			perror("Error al abrir ../resultados/exec_resultado.txt");
			_exit(1);
		}

		if (dup2(fd_out, STDOUT_FILENO) == -1){
			perror("Error al ejecutar dup2()");
			close(fd_out);
			_exit(1);
		}
		close(fd_out);

		// Se usa execlp buscando el patrón exacto (-F) en base al snprintf de arriba
		execlp("grep", "grep", "-F", patron, argv[1], (char *)NULL);

		// Si execlp falla, llega hasta acá
		perror("Error en la ejecucion de execlp()");
		_exit(1);
	}

	// PADRE
	close(pipefd[1]);

	int cant_recibida=0;
	ssize_t n = read(pipefd[0], &cant_recibida, sizeof(cant_recibida));
	if (n == -1){
		perror("Error al leer de la tuberia en el padre");
	}
	close(pipefd[0]);

	int status1, status2;
	waitpid(pid1, &status1, 0);
	waitpid(pid2, &status2, 0);

	// Si el hijo 1 fallo, no llego a enviar el dato: no mostrar un 0 engañoso
	if (n != sizeof(cant_recibida) || !WIFEXITED(status1) || WEXITSTATUS(status1) != 0) {
		fprintf(stderr, "Error: el hijo 1 no pudo calcular la cantidad\n");
		return 1;
	}

	if (!WIFEXITED(status2) || WEXITSTATUS(status2) != 0) {
		fprintf(stderr, "Error: el hijo 2 (exec) finalizo con error\n");
		return 1;
	}

	printf("Cantidad recibida del hijo: %d.\n", cant_recibida);
	return 0; // Fin padre
}
