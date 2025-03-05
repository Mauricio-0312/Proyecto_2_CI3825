#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// Estructura que representa una celda en la cuadrícula
// tipo: 0 = Tierra Baldía (TB), 1 = Objetivo Militar (OM), 2 = Infraestructura Civil (IC)
// resistencia: nivel de resistencia del objeto en la celda
typedef struct {
    int tipo;
    int resistencia;
} Celda;

// Estructura que representa un dron
// x, y: coordenadas donde explota el dron
// rd: radio de destrucción
// pe: poder explosivo
typedef struct {
    int x, y, rd, pe;
} Dron;

// Función que procesa las explosiones de un rango de drones
// Modifica la resistencia de los objetos en el área afectada
void procesar_drones(int inicio, int fin, int n, int m, Celda **teatro, Dron *drones) {
    for (int d = inicio; d < fin; d++) {
        Dron dron = drones[d];
        // Recorre el área de destrucción del dron
        for (int i = dron.x - dron.rd; i <= dron.x + dron.rd; i++) {
            for (int j = dron.y - dron.rd; j <= dron.y + dron.rd; j++) {
                // Verifica que la celda esté dentro de los límites de la cuadrícula
                if (i >= 0 && i < n && j >= 0 && j < m) {
                    Celda *celda = &teatro[i][j];
                    // Modifica la resistencia dependiendo del tipo de objeto
                    if (celda->tipo == 1) { // Objetivo militar (OM)
                        celda->resistencia -= dron.pe;
                    } else if (celda->tipo == 2) { // Infraestructura civil (IC)
                        celda->resistencia += dron.pe;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // Verifica el número correcto de argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s [n_procesos] [archivo_instancia]\n", argv[0]);
        return 1;
    }

    int num_procesos = atoi(argv[1]);
    FILE *archivo = fopen(argv[2], "r");
    if (!archivo) {
        perror("Error abriendo el archivo");
        return 1;
    }

    int n, m, k, l;
    fscanf(archivo, "%d %d", &n, &m);

    // Asigna memoria dinámica para la cuadrícula
    Celda **teatro = (Celda **)malloc(n * sizeof(Celda *));
    if (teatro == NULL) {
        perror("Error al asignar memoria para la cuadrícula");
        return 1;
    }

    // Asigna memoria para cada fila de la cuadrícula
    for (int i = 0; i < n; i++) {
        teatro[i] = (Celda *)calloc(m, sizeof(Celda));
        if (teatro[i] == NULL) {
            perror("Error al asignar memoria para una fila de la cuadrícula");
            return 1;
        }
    }

    // Lee los objetos (OM e IC) y los coloca en la cuadrícula
    fscanf(archivo, "%d", &k);
    for (int i = 0; i < k; i++) {
        int x, y, resistencia;
        fscanf(archivo, "%d %d %d", &x, &y, &resistencia);
        teatro[x][y].resistencia = resistencia;
        teatro[x][y].tipo = (resistencia < 0) ? 1 : 2;
    }

    // Lee la información de los drones
    fscanf(archivo, "%d", &l);
    Dron drones[l];
    for (int i = 0; i < l; i++) {
        fscanf(archivo, "%d %d %d %d", &drones[i].x, &drones[i].y, &drones[i].rd, &drones[i].pe);
    }

    // Crea procesos para procesar los drones en paralelo
    for (int i = 0; i < num_procesos; i++) {
        int inicio = i * (l / num_procesos);
        int fin = (i == num_procesos - 1) ? l : inicio + (l / num_procesos);

        pid_t pid = fork();
        if (pid == 0) { // Proceso hijo
            procesar_drones(inicio, fin, n, m, teatro, drones);
            exit(0);
        } else if (pid < 0) { // Error al crear proceso
            perror("Error creando proceso");
            exit(1);
        }
    }

    // Espera a que todos los procesos hijos terminen
    for (int i = 0; i < num_procesos; i++) {
        wait(NULL);
    }

    fclose(archivo);
    // Libera la memoria asignada para la cuadrícula
    for (int i = 0; i < n; i++) {
        free(teatro[i]);
        printf("Memoria liberada correctamente para la fila %d\n", i);
    }
    free(teatro);
    printf("Memoria liberada correctamente para la cuadrícula\n");

    return 0;
}
