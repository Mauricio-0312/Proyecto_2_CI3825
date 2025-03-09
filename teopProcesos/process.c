#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>

// Estructura que representa una celda en la cuadrícula
// tipo: 0 = Tierra Baldía (TB), 1 = Objetivo Militar (OM), 2 = Infraestructura Civil (IC)
// resistencia: nivel de resistencia del objeto en la celda
typedef struct {
    int tipo;
    int resistencia;
    int resistencia_inicial;
} Celda;

// Estructura que representa un dron
// x, y: coordenadas donde explota el dron
// rd: radio de destrucción
// pe: poder explosivo
typedef struct {
    int x, y, rd, pe;
} Dron;


// Mutex global para proteger la modificación de las celdas
pthread_mutex_t mutex;

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

                    pthread_mutex_lock(&mutex);
                    // Modifica la resistencia dependiendo del tipo de objeto
                    if (celda->tipo == 1) { // Objetivo militar (OM)
                        celda->resistencia += dron.pe;
                    } else if (celda->tipo == 2) { // Infraestructura civil (IC)
                        celda->resistencia -= dron.pe;
                    }

                    pthread_mutex_unlock(&mutex);
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

    pthread_mutex_init(&mutex, NULL);

    Celda **teatro = (Celda **)mmap(NULL, n * sizeof(Celda *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (teatro == MAP_FAILED) {
        perror("Error mapeando la memoria");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        teatro[i] = (Celda *)mmap(NULL, m * sizeof(Celda), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (teatro[i] == MAP_FAILED) {
            perror("Error mapeando la memoria");
            exit(1);
        }
    }

    // Lee los objetos (OM e IC) y los coloca en la cuadrícula
    fscanf(archivo, "%d", &k);
    for (int i = 0; i < k; i++) {
        int x, y, resistencia;
        fscanf(archivo, "%d %d %d", &x, &y, &resistencia);
        teatro[x][y].resistencia = resistencia;
        teatro[x][y].resistencia_inicial = resistencia;
        teatro[x][y].tipo = (resistencia < 0) ? 1 : 2;
    }

    // Lee la información de los drones
    fscanf(archivo, "%d", &l);
    Dron drones[l];
    for (int i = 0; i < l; i++) {
        fscanf(archivo, "%d %d %d %d", &drones[i].x, &drones[i].y, &drones[i].rd, &drones[i].pe);
    }
    fclose(archivo);
    // Calculamos el minimo entre el numero de drones y la cantidad de celdas
    int minimo = n*m < l ? n*m : l;
    
    // Si el numero de procesos ingresados por el usuario es mayor que "minimo", entonces el numero
    // de procesos pasa a ser este minimo.
    if (num_procesos > minimo){
        num_procesos = minimo;
    }
    

    // Colocar clock inicial
    // Crea procesos para procesar los drones en paralelo
    for (int i = 0; i < num_procesos; i++) {
        int inicio = i * (l / num_procesos);
        int fin = (i == num_procesos - 1) ? l : inicio + (l / num_procesos);

        pid_t pid = fork();
        if (pid == 0) { // Proceso hijo
            procesar_drones(inicio, fin, n, m, teatro, drones);
             // Liberar memoria asignada para la cuadricula
            for (int i = 0; i < n; i++) {
                munmap(teatro[i], m * sizeof(Celda));
            }
            
            munmap(teatro, n * sizeof(Celda *));
            
            // Destruye el mutex global
            pthread_mutex_destroy(&mutex);
            exit(0);
        } else if (pid < 0) { // Error al crear proceso
            perror("Error creando proceso");
             // Liberar memoria asignada para la cuadricula
            for (int i = 0; i < n; i++) {
                munmap(teatro[i], m * sizeof(Celda));
            }
            
            munmap(teatro, n * sizeof(Celda *));
            
            // Destruye el mutex global
            pthread_mutex_destroy(&mutex);
            exit(1);
        }
    }

    // Espera a que todos los procesos hijos terminen
    for (int i = 0; i < num_procesos; i++) {
        wait(NULL);
    }
    // Colocar clock final

    // Cuenta el estado final de los objetos
    int om_intactos = 0, om_parciales = 0, om_destruidos = 0;
    int ic_intactos = 0, ic_parciales = 0, ic_destruidos = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {

            if (teatro[i][j].tipo == 1) {
                if (teatro[i][j].resistencia == teatro[i][j].resistencia_inicial) om_intactos++;
                if (teatro[i][j].resistencia >= 0) om_destruidos++;
                if (teatro[i][j].resistencia < 0 && teatro[i][j].resistencia > teatro[i][j].resistencia_inicial) om_parciales++;
            } else if (teatro[i][j].tipo == 2) {
                if (teatro[i][j].resistencia == teatro[i][j].resistencia_inicial) ic_intactos++;
                if (teatro[i][j].resistencia <= 0) ic_destruidos++;
                if (teatro[i][j].resistencia > 0 && teatro[i][j].resistencia < teatro[i][j].resistencia_inicial) ic_parciales++;
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
    
    // Liberar memoria asignada para la cuadricula
    for (int i = 0; i < n; i++) {
        munmap(teatro[i], m * sizeof(Celda));
    }
    
    munmap(teatro, n * sizeof(Celda *));
    
    // Destruye el mutex global
    pthread_mutex_destroy(&mutex);

    return 0;
}
