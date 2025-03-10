#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

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
    int tipo;
    int resistencia;
    int resistencia_inicial;
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

/**
 * @brief Procesa la destruccion causada por los drones en el teatro.
 *
 * @param inicio Indice inicial de los drones a procesar.
 * @param fin Indice final de los drones a procesar.
 * @param n Numero de filas en la cuadricula del teatro.
 * @param m Numero de columnas en la cuadricula del teatro.
 * @param teatro Matriz de celdas que representa el teatro de operaciones.
 * @param drones Arreglo de drones.
 * 
 * @return void.
 *
 * La funcion recorre el area de destrucción de cada dron en el rango especificado
 * y modifica la resistencia de las celdas en funcion del tipo de objeto presente.
 * Si la celda contiene un objetivo militar (OM), se incrementa su resistencia.
 * Si la celda contiene infraestructura civil (IC), se decrementa su resistencia.
 * La modificacion de la resistencia se realiza dentro de una seccion critica protegida
 * por un mutex para asegurar la consistencia de los datos.
 */
void procesar_drones(int inicio, int fin, int n, int m, Celda **teatro, Dron *drones) {

    for (int d = inicio; d < fin; d++) {
        Dron dron = drones[d];

        // Recorremos el area de destruccion del dron
        for (int i = dron.x - dron.rd; i <= dron.x + dron.rd; i++) {

            for (int j = dron.y - dron.rd; j <= dron.y + dron.rd; j++) {
                // Verificamos que la celda esté dentro de los limites del teatro
                if (i >= 0 && i < n && j >= 0 && j < m) {
                    Celda *celda = &teatro[i][j];

                    // Bloqueamos seccion crítica
                    pthread_mutex_lock(&mutex);

                    // Modifica la resistencia dependiendo del tipo de objeto
                    if (celda->tipo == 1) { // Objetivo militar (OM)
                        celda->resistencia += dron.pe;

                    } else if (celda->tipo == 2) { // Infraestructura civil (IC)
                        celda->resistencia -= dron.pe;
                    }

                    // Desbloqueamos seccion crítica
                    pthread_mutex_unlock(&mutex);
                }
            }
        }
    }
}

// Funcion donde se procesa el input
int main(int argc, char *argv[]) {

    // Verificamos el número correcto de argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s [n_procesos] [archivo_instancia]\n", argv[0]);
        return 1;
    }

    int num_procesos = atoi(argv[1]);

    // Lectura del archivo
    FILE *archivo = fopen(argv[2], "r");
    if (!archivo) {
        perror("Error abriendo el archivo");
        return 1;
    }

    int n, m, k, l;

    // Leemos las dimensiones del teatro
    fscanf(archivo, "%d %d", &n, &m);

    // Inicializamos el mutex
    pthread_mutex_init(&mutex, NULL);

    // Declaramos el teatro y asignamos memoria dinamica y compartida para los procesos
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

    // Leemos la cantidad de objetos
    fscanf(archivo, "%d", &k);

    // Leemos cada uno de los objetos y se asignan en el teatro
    for (int i = 0; i < k; i++) {
        int x, y, resistencia;

        // Leeomos coordenadas y resistencia para cada objeto
        fscanf(archivo, "%d %d %d", &x, &y, &resistencia);
        teatro[x][y].resistencia = resistencia;
        teatro[x][y].resistencia_inicial = resistencia;
        teatro[x][y].tipo = (resistencia < 0) ? 1 : 2;
    }

    // Leemos la cantidad de drones
    fscanf(archivo, "%d", &l);

    // Declaramos el arreglo de drones
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
    
    // Se crean los procesos para realizar el ataque de los drones en paralelo
    for (int i = 0; i < num_procesos; i++) {
        int inicio = i * (l / num_procesos);
        int fin = (i == num_procesos - 1) ? l : inicio + (l / num_procesos);

        // Creamos proceso
        pid_t pid = fork();
        if (pid == 0) { // Proceso hijo

            // Procesamos el ataque de drones en el teatro
            procesar_drones(inicio, fin, n, m, teatro, drones);

             // Liberamos memoria luego de que cada proceso hijo termine
            for (int i = 0; i < n; i++) {
                munmap(teatro[i], m * sizeof(Celda));
            }
            
            munmap(teatro, n * sizeof(Celda *));
            
            // Destruimos el mutex
            pthread_mutex_destroy(&mutex);

            // Terminamos el proceso
            exit(0);

        } else if (pid < 0) { // Error al crear proceso
            perror("Error creando proceso");

             // Liberamos memoria asignada
            for (int i = 0; i < n; i++) {
                munmap(teatro[i], m * sizeof(Celda));
            }
            
            munmap(teatro, n * sizeof(Celda *));
            
            // Destruimos el mutex
            pthread_mutex_destroy(&mutex);

            // Terminamos el proceso
            exit(1);
        }
    }

    // Esperamos a que todos los procesos hijos terminen
    for (int i = 0; i < num_procesos; i++) {
        wait(NULL);
    }
    
    
    int om_intactos = 0, om_parciales = 0, om_destruidos = 0;
    int ic_intactos = 0, ic_parciales = 0, ic_destruidos = 0;

    // Recorremos el teatro para analizar los resultados luego del ataque
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

    // Imprimimos los resultados
    printf("OM sin destruir: %d\n", om_intactos);
    printf("OM parcialmente destruidos: %d\n", om_parciales);
    printf("OM totalmente destruidos: %d\n", om_destruidos);
    printf("IC sin destruir: %d\n", ic_intactos);
    printf("IC parcialmente destruidos: %d\n", ic_parciales);
    printf("IC totalmente destruidos: %d\n", ic_destruidos);
    
    // Liberamos memoria asignada para la cuadricula
    for (int i = 0; i < n; i++) {
        munmap(teatro[i], m * sizeof(Celda));
    }
    
    munmap(teatro, n * sizeof(Celda *));
    
    // Destruimos el mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}
