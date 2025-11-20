#include <unistd.h>     // write(), read(), exec(), fork(), chdir(), dup2()
#include <sys/types.h>  
#include <sys/wait.h>    // wait(), waitpid()
#include <sys/stat.h>    // mkdir()
#include <fcntl.h>       // open()
#include <string.h>      // strtok(), strcmp()
#include <stdlib.h>      // exit(), malloc()
#include <stdio.h>       // perror(), printf()

#define MAX 1024         // Tamaño máximo del buffer de entrada
#define TOKENS 64        // Máximo número de argumentos permitidos

// FUNCION parse()
// Se encarga de dividir la línea ingresada por el usuario
// en una lista de tokens (argumentos).
// Utiliza strtok() para separar por espacio, tab o salto.
void parse(char *line, char **args) {
    int i = 0;

    // Primer token
    args[i] = strtok(line, " \t\n");

    // Leer todos los demás tokens
    while (args[i] != NULL && i < TOKENS - 1) {
        args[++i] = strtok(NULL, " \t\n");
    }

    // Última posición debe ser NULL para execvp()
    args[i] = NULL;
}

// Manejo de redirecciones (< y >)
// Revisa la lista de argumentos y si encuentra un símbolo
// de redirección, redirige stdin o stdout utilizando dup2().
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {

        // Redirección de salida: comando > archivo
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);  // stdout -> archivo
            close(fd);
            args[i] = NULL;           // eliminamos '>'
        }

        // Redirección de entrada: comando < archivo
        else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i+1], O_RDONLY);
            dup2(fd, STDIN_FILENO);   // stdin <- archivo
            close(fd);
            args[i] = NULL;
        }
    }
}

// Implementación de comandos internos (cd, ls, pwd, etc.)
// Devuelve 1 si el comando es interno y ya fue ejecutado.
// Devuelve 0 si NO es interno (debe ejecutarse externamente).
int comandos_internos(char **args) {

    // salir / exit
    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "salir") == 0) {
        exit(0);
    }

    // cd -> cambiar directorio
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) write(1, "Uso: cd <directorio>\n", 22);
        else chdir(args[1]);
        return 1;
    }

    // pwd -> imprimir directorio actual
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[256];
        getcwd(cwd, sizeof(cwd));
        write(1, cwd, strlen(cwd));
        write(1, "\n", 1);
        return 1;
    }

    // ls -> implementado con exec interno
    if (strcmp(args[0], "ls") == 0) {
        pid_t pid = fork();
        if (pid == 0) execl("/bin/ls", "ls", NULL);
        else wait(NULL);
        return 1;
    }

    // mkdir -> crear directorio
    if (strcmp(args[0], "mkdir") == 0) {
        if (args[1] == NULL) write(1, "Uso: mkdir <nombre>\n", 21);
        else mkdir(args[1], 0755);
        return 1;
    }

    // rm -> borrar archivo
    if (strcmp(args[0], "rm") == 0) {
        if (args[1] == NULL) write(1, "Uso: rm <archivo>\n", 19);
        else unlink(args[1]);
        return 1;
    }

    // cp -> copiar archivo (implementación manual)
    if (strcmp(args[0], "cp") == 0) {
        int fd1 = open(args[1], O_RDONLY);
        int fd2 = open(args[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);

        char buffer[1024];
        int n;

        while ((n = read(fd1, buffer, 1024)) > 0)
            write(fd2, buffer, n);

        close(fd1);
        close(fd2);
        return 1;
    }

    // mv -> renombrar archivo
    if (strcmp(args[0], "mv") == 0) {
        rename(args[1], args[2]);
        return 1;
    }

    // cat -> mostrar archivo por pantalla
    if (strcmp(args[0], "cat") == 0) {
        int fd = open(args[1], O_RDONLY);

        char buffer[1024];
        int n;

        while ((n = read(fd, buffer, 1024)) > 0)
            write(1, buffer, n);

        close(fd);
        return 1;
    }

    // Si llega aquí, no es un comando interno
    return 0;
}

// ejecutar() → Ejecuta comandos externos usando fork() + execvp()
// background = 1 si el usuario puso & al final.
void ejecutar(char **args, int background) {
    pid_t pid = fork();

    if (pid == 0) {
        // Hijo: verifica redirecciones antes de ejecutar
        handle_redirection(args);

        execvp(args[0], args);

        // Si execvp falla:
        write(1, "Comando no encontrado\n", 22);
        exit(1);
    }

    // Proceso padre: espera solo si NO es background
    if (!background) {
        waitpid(pid, NULL, 0);
    }
}

// BUCLE PRINCIPAL DE BASH
int main() {
    char line[MAX];
    char *args[TOKENS];

    while (1) {
        // Mostrar prompt (myshell>)
        write(1, "Terminal> ", 9);

        // Leer entrada del usuario
        int n = read(0, line, MAX);
        if (n <= 0) break;

        // Separar en tokens
        parse(line, args);
        if (args[0] == NULL) continue;

        // Verificar si corre en background (comando &)
        int background = 0;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "&") == 0) {
                background = 1;
                args[i] = NULL; // eliminamos "&"
            }
        }

        // Revisar si es comando interno
        if (comandos_internos(args)) continue;

        // Ejecutar comando externo
        ejecutar(args, background);
    }

    return 0;
}
