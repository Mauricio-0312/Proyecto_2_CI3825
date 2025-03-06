#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

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

// Argumentos que se pasan a cada hilo
typedef struct {
    int inicio, fin;        // Rango de drones a procesar
    int n, m;               // Dimensiones de la cuadrícula
    int num_drones;         // Número total de drones
    Celda **teatro;         // Puntero a la cuadrícula
    Dron *drones;           // Lista de drones
} HiloArgs;

// Función que procesa la explosión de drones en un rango específico
void *procesar_drones(void *arg) {
    HiloArgs *args = (HiloArgs *)arg;
    for (int d = args->inicio; d < args->fin; d++) {
        Dron dron = args->drones[d];
        // Recorre el área afectada por la explosión del dron
        for (int i = dron.x - dron.rd; i <= dron.x + dron.rd; i++) {
            for (int j = dron.y - dron.rd; j <= dron.y + dron.rd; j++) {
                // Verifica si la celda está dentro de los límites de la cuadrícula
                if (i >= 0 && i < args->n && j >= 0 && j < args->m) {
                    Celda *celda = &args->teatro[i][j];
                    // Aplica el efecto del poder explosivo según el tipo de objeto
                    if (celda->tipo == 1) {
                        celda->resistencia -= dron.pe; // Para OM, se resta el poder explosivo
                    } else if (celda->tipo == 2) {
                        celda->resistencia += dron.pe; // Para IC, se suma el poder explosivo
                    }
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s [n_hilos] [archivo_instancia]\n", argv[0]);
        return 1;
    }

    int num_hilos = atoi(argv[1]);
    FILE *archivo = fopen(argv[2], "r");
    if (!archivo) {
        perror("Error abriendo el archivo");
        return 1;
    }

    int n, m, k, l;
    fscanf(archivo, "%d %d", &n, &m);

    // Asigna memoria dinámica para la cuadrícula
    Celda **teatro = (Celda **)malloc(n * sizeof(Celda *));
    for (int i = 0; i < n; i++) {
        teatro[i] = (Celda *)calloc(m, sizeof(Celda));
    }

    // Lee los objetos (OM, IC) y los coloca en la cuadrícula
    fscanf(archivo, "%d", &k);
    for (int i = 0; i < k; i++) {
        int x, y, resistencia;
        fscanf(archivo, "%d %d %d", &x, &y, &resistencia);
        teatro[x][y].resistencia = resistencia;
        teatro[x][y].tipo = (resistencia < 0) ? 1 : 2;
    }

    // Lee los drones
    fscanf(archivo, "%d", &l);
    Dron drones[l];
    for (int i = 0; i < l; i++) {
        fscanf(archivo, "%d %d %d %d", &drones[i].x, &drones[i].y, &drones[i].rd, &drones[i].pe);
    }

    // Calculamos el minimo entre el numero de drones y la cantidad de celdas
    int minimo = n*m < l ? n*m : l;
    
    // Si el numero de procesos ingresados por el usuario es mayor que "minimo", entonces el numero
    // de procesos pasa a ser este minimo.
    if (num_hilos > minimo){
        num_hilos = minimo;
    }

    
    // Crea los hilos y distribuye el trabajo entre ellos
    pthread_t hilos[num_hilos];
    HiloArgs args[num_hilos];

    // Colocar clock inicial
    for (int i = 0; i < num_hilos; i++) {
        args[i].inicio = i * (l / num_hilos);
        args[i].fin = (i == num_hilos - 1) ? l : args[i].inicio + (l / num_hilos);
        args[i].n = n;
        args[i].m = m;
        args[i].num_drones = l / num_hilos;
        args[i].teatro = teatro;
        args[i].drones = drones;
        if (pthread_create(&hilos[i], NULL, procesar_drones, &args[i]) != 0) {
            perror("Error creando hilo");
            exit(1);
        }
    }

    // Espera a que todos los hilos terminen
    for (int i = 0; i < num_hilos; i++) {
        if (pthread_join(hilos[i], NULL) != 0) {
            perror("Error al esperar hilo");
            exit(1);
        }
    }
    // Colocar clock final

    // Cuenta el estado final de los objetos
    int om_intactos = 0, om_parciales = 0, om_destruidos = 0;
    int ic_intactos = 0, ic_parciales = 0, ic_destruidos = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (teatro[i][j].tipo == 1) {
                if (teatro[i][j].resistencia <= 0) om_destruidos++;
                else if (teatro[i][j].resistencia < -1) om_parciales++;
                else om_intactos++;
            } else if (teatro[i][j].tipo == 2) {
                if (teatro[i][j].resistencia <= 0) ic_destruidos++;
                else if (teatro[i][j].resistencia > 1) ic_parciales++;
                else ic_intactos++;
            }
        }
    }

    // Imprime los resultados
    printf("OM sin destruir: %d\n", om_intactos);
    printf("OM parcialmente destruidos: %d\n", om_parciales);
    printf("OM totalmente destruidos: %d\n", om_destruidos);
    printf("IC sin destruir: %d\n", ic_intactos);
    printf("IC parcialmente destruidos: %d\n", ic_parciales);
    printf("IC totalmente destruidos: %d\n", ic_destruidos);

    // Libera la memoria dinámica
    for (int i = 0; i < n; i++) {
        free(teatro[i]);
    }
    free(teatro);

    fclose(archivo);
    return 0;
}
