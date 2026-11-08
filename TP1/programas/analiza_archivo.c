#include <stdio.h>
#include <string.h>   // Para strlen()
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h> // Libreria necesaria para EEXIST

#define BUFFER_SIZE 4096

int main (int argc, char *argv[]) {

	// Verificacion de parametros de entrada
	if (argc != 2){
		const char *msg = "Uso incorrecto, se espera: ./analiza_archivo <archivo>\n";
		write(STDERR_FILENO, msg, strlen(msg));
		return 1;
	}

	// Obtiene metadatos del archivo usando stat()
	struct stat s;
	if(stat(argv[1], &s) == -1){
		perror("Error en la ejecucion de stat()");
		return 1;
	}

	// Verifica que la ruta pasada sea de un archivo real y no de una carpeta
	if (!S_ISREG(s.st_mode)) {
		const char *msg = "ERROR: el parametro no es un archivo regular\n";
		write(STDERR_FILENO, msg, strlen(msg));
		return 1;
	}

	long tamano_bytes = s.st_size;

	int fd = open(argv[1], O_RDONLY);
	if(fd < 0) {
		perror("Error en la apertura del archivo");
		return 1;
	}

	// Lectura y conteo
	long registros=0, pts_y_cms=0, cant=0;
	char buffer[BUFFER_SIZE];
	ssize_t bytes_leidos;

	while ((bytes_leidos = read(fd, buffer, sizeof(buffer))) > 0){
		for (ssize_t i=0; i<bytes_leidos; i++){
			if(buffer[i] == '\n') registros++;
			if(buffer[i] == ';') pts_y_cms++;
			if(buffer[i] == 'A') cant++;
		}
	}

	if (bytes_leidos == -1) {
		perror("Error al leer el archivo");
		close(fd);
		return 1;
	}

	// Prepara un string con los resultados
	char res_str[256];
	int len = snprintf(res_str, sizeof(res_str),
					   "TAMANO_BYTES=%ld\nREGISTROS=%ld\nPUNTOS_Y_COMAS=%ld\nCANTIDAD_A=%ld\n",
					   tamano_bytes, registros, pts_y_cms, cant);

	// Escribe en pantalla
	if (write(STDOUT_FILENO, res_str, len) == -1) {
		perror("Error al escribir por pantalla");
	}

	// Crea la carpeta resultados si no existe. Y evita que open() falle por falta del directorio.
	if (mkdir("../resultados", 0755) == -1 && errno != EEXIST) {
		perror("Advertencia: problema al crear el directorio resultados");
	}

	int fd_out = open("../resultados/analisis_c.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd_out < 0){
		perror("Error en la creacion de ../resultados/analisis_c.txt");
		close(fd);
		return 1;
	}
	if (write(fd_out, res_str, len) != len) {
		perror("Error al escribir ../resultados/analisis_c.txt");
		close(fd_out);
		close(fd);
		return 1;
	}
	close(fd_out);

	// Leer y escribir los primeros 20 bytes con el lseek()
	if (lseek(fd, 0, SEEK_SET) == -1) {
		perror("Error con el lseek()");
		close(fd);
		return 1;
	}

	char p20[20];
	ssize_t bytes_p20 = read(fd, p20, 20);
	if (bytes_p20 == -1) {
		perror("Error al leer los primeros 20 bytes");
		close(fd);
		return 1;
	}
	if (bytes_p20 > 0){
		int fd_p20 = open("../resultados/primeros20.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(fd_p20 >= 0) {
			if (write(fd_p20, p20, bytes_p20) != bytes_p20) {
				perror("Error al escribir primeros20.txt");
			}
			close(fd_p20);
		} else {
			perror("Error al crear primeros20.txt");
		}
	}

	close(fd);
	return 0;
}
