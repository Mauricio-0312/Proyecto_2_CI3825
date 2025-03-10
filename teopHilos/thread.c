#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


/**
 * Estructura que representa una celda en el teatro (cuadricula).
 * 
 * @typedef Celda
 * 
 * @field tipo
 *        Tipo de celda:
 *        - 0: Tierra Baldía (TB)
 *        - 1: Objetivo Militar (OM)
 *        - 2: Infraestructura Civil (IC)
 * 
 * @field resistencia
 *        Nivel de resistencia del objeto en la celda.
 * 
 * @field resistencia_inicial
 *        Nivel de resistencia inicial del objeto en la celda.
 */
typedef struct {
    int tipo, resistencia, resistencia_inicial;
} Celda;

/**
 * Estructura que representa un dron.
 * 
 * @typedef Dron
 * 
 * @field x Coordenada X donde explota el dron.
 * @field y Coordenada Y donde explota el dron.
 * @field rd Radio de destrucción del dron.
 * @field pe Poder explosivo del dron.
 */
typedef struct {
    int x, y, rd, pe;
} Dron;

// Mutex para proteger la modificación de las celdas
pthread_mutex_t mutex;

// Argumentos que se pasan a cada hilo
typedef struct {
    // Rango de drones a procesar
    // Dimensiones de la cuadrícula
    // Número total de drones
    int inicio, fin, n, m, num_drones;         
    Celda ***teatro;         // Puntero a la cuadrícula
    Celda *objetivos;       // Arreglo de  objetivos
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
                    Celda *celda = args->teatro[i][j];
                    // Aplica el efecto del poder explosivo según el tipo de objeto
                    if (celda != NULL){
        
                        pthread_mutex_lock(&mutex);
                        if (celda->tipo == 1) {
                            celda->resistencia += dron.pe; // Para OM, se resta el poder explosivo
                        } else if (celda->tipo == 2) {
                            celda->resistencia -= dron.pe; // Para IC, se suma el poder explosivo
                        }

                        pthread_mutex_unlock(&mutex);
                    }
                    else{
                        continue;
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

    // Inicializamos el mutex
    pthread_mutex_init(&mutex, NULL);

    // Creamos el teatro (cuadricula) de punteros y la inicializamos en NULL
    Celda ***teatro = (Celda ***)malloc(n * sizeof(Celda **));
    for (int i = 0; i < n; i++) {
        teatro[i] = (Celda **)calloc(m, sizeof(Celda *));
        for (int j = 0; j < m; j++){
            teatro[i][j] = NULL;
        }
        
    }

    fscanf(archivo, "%d", &k);

    Celda *objetivos = (Celda *)malloc(k * sizeof(Celda));


    for (int i = 0; i < k; i++) {
        int x, y, resistencia;
        fscanf(archivo, "%d %d %d", &x, &y, &resistencia);
        // teatro[x][y].resistencia = resistencia;
        // teatro[x][y].resistencia_inicial = resistencia;
        // teatro[x][y].tipo = (resistencia < 0) ? 1 : 2;
        // teatro[x][y].x = x;
        // teatro[x][y].y = y;
        objetivos[i].resistencia = resistencia;
        objetivos[i].resistencia_inicial = resistencia;
        objetivos[i].tipo = (resistencia < 0) ? 1 : 2;
        objetivos[i].x = x;
        objetivos[i].y = y;
        teatro[x][y] = &objetivos[i];
    }

    // Lee los drones
    fscanf(archivo, "%d", &l);
    Dron drones[l];
    for (int i = 0; i < l; i++) {
        fscanf(archivo, "%d %d %d %d", &drones[i].x, &drones[i].y, &drones[i].rd, &drones[i].pe);
    }

    // Crea los hilos y distribuye el trabajo entre ellos
    pthread_t hilos[num_hilos];
    HiloArgs args[num_hilos];

    for (int i = 0; i < num_hilos; i++) {
        args[i].inicio = i * (l / num_hilos);
        args[i].fin = (i == num_hilos - 1) ? l : args[i].inicio + (l / num_hilos);
        args[i].n = n;
        args[i].m = m;
        args[i].num_drones = l / num_hilos;
        args[i].objetivos = objetivos;
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

    // Cuenta el estado final de los objetos
    int om_intactos = 0, om_parciales = 0, om_destruidos = 0;
    int ic_intactos = 0, ic_parciales = 0, ic_destruidos = 0;

    // Iteramos sobre el arreglo de objetivos
    for (int i = 0; i < k; i++) {
        if (objetivos[i].tipo == 1) {
            if (objetivos[i].resistencia == objetivos[i].resistencia_inicial) om_intactos++;
            if (objetivos[i].resistencia >= 0) om_destruidos++;
            if (objetivos[i].resistencia < 0 && objetivos[i].resistencia > objetivos[i].resistencia_inicial) om_parciales++;
        } else if (objetivos[i].tipo == 2) {
            if (objetivos[i].resistencia == objetivos[i].resistencia_inicial) ic_intactos++;
            if (objetivos[i].resistencia <= 0) ic_destruidos++;
            if (objetivos[i].resistencia > 0 && objetivos[i].resistencia < objetivos[i].resistencia_inicial) ic_parciales++;
        }     
    }

    // Imprime los resultados
    printf("OM intactos: %d\n", om_intactos);
    printf("OM parcialmente destruidos: %d\n", om_parciales);
    printf("OM totalmente destruidos: %d\n", om_destruidos);
    printf("IC intactos: %d\n", ic_intactos);
    printf("IC parcialmente destruidos: %d\n", ic_parciales);
    printf("IC totalmente destruidos: %d\n", ic_destruidos);

    // Libera la memoria dinámica
    for (int i = 0; i < n; i++) {
        free(teatro[i]);
    }
    free(teatro);
    free(objetivos);
    pthread_mutex_destroy(&mutex);

    fclose(archivo);
    return 0;
}
