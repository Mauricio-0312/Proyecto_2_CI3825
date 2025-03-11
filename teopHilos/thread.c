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
    int tipo;
    long long resistencia, resistencia_inicial;
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
    long long x, y, rd, pe;
} Dron;

// Mutex para proteger la modificación de las celdas
pthread_mutex_t mutex;

/**
 * Estructura que contiene los argumentos para los hilos.
 * 
 * @field inicio Rango inicial de drones a procesar.
 * @field fin Rango final de drones a procesar.
 * @field n Número de filas de la cuadrícula.
 * @field m Número de columnas de la cuadrícula.
 * @field num_drones Número total de drones.
 * @field teatro Puntero a la cuadrícula de celdas.
 * @field objetivos Arreglo de objetivos.
 * @field drones Lista de drones.
 */
typedef struct {
    long long inicio, fin, n, m, num_drones;         
    Celda ***teatro, *objetivos; 
    Dron *drones;           
} HiloArgs;

/**
 * @brief Procesa la destruccion causada por los drones en el teatro.
 *
 * @param arg 
 * 
 * @return void.
 *
 * La funcion recorre el area de destrucción de cada dron en el rango especificado
 * y modifica la resistencia de las celdas en funcion del tipo de objeto presente.
 * Si la celda contiene un objetivo militar (OM), se incrementa su resistencia.
 * Si la celda contiene infraestructura civil (IC), se decrementa su resistencia.
 * La modificacion de la resistencia se realiza dentro de una seccion critica protegida
 * por un mutex para asegurar la consistencia de los datos y evitar 
 * condiciones de carrera.
 */
void *procesar_drones(void *arg) {
    HiloArgs *args = (HiloArgs *)arg;

    for (long long d = args->inicio; d < args->fin; d++) {
        Dron dron = args->drones[d];

         // Recorremos el area de destruccion del dron
        for (long long i = dron.x - dron.rd; i <= dron.x + dron.rd; i++) {

            for (long long j = dron.y - dron.rd; j <= dron.y + dron.rd; j++) {
               // Verificamos que la celda esté dentro de los limites del teatro
                if (i >= 0 && i < args->n && j >= 0 && j < args->m) {
                    Celda *celda = args->teatro[i][j];
                    
                    // Verificamos si la celda tiene un objeto
                    if (celda != NULL){
                        
                        // Bloqueamos seccion critica
                        pthread_mutex_lock(&mutex);

                         // Modifica la resistencia dependiendo del tipo de objeto
                        if (celda->tipo == 1) { // Objetivo militar (OM)
                            celda->resistencia += dron.pe; 
                            
                        } else if (celda->tipo == 2) { // Infraestructura civil (IC)
                            celda->resistencia -= dron.pe;
                        }

                        // Desbloqueamos seccion critica
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

// Funcion main donde se procesa el input
int main(int argc, char *argv[]) {
    
    // Verificamos el número correcto de argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s [n_hilos] [archivo_instancia]\n", argv[0]);
        return 1;
    }

    long long num_hilos = atoi(argv[1]);

    // Lectura del archivo
    FILE *archivo = fopen(argv[2], "r");
    if (!archivo) {
        perror("Error abriendo el archivo");
        return 1;
    }

    // Variables para guardar datos de la lectura
    long long n, m, k, l;

    // Leemos las dimensiones del teatro
    fscanf(archivo, "%lld %lld", &n, &m);

    // Inicializamos el mutex
    pthread_mutex_init(&mutex, NULL);

    //printf("Hola2\n");
    // Creamos el teatro (cuadricula) de punteros y la inicializamos en NULL
    Celda ***teatro = (Celda ***)malloc(n * sizeof(Celda **));
    for (long long i = 0; i < n; i++) {
        teatro[i] = (Celda **)calloc(m, sizeof(Celda *));
        for (long long j = 0; j < m; j++){
            teatro[i][j] = NULL;
        }
        
    }

   
    // Leemos la cantidad de objetos
    fscanf(archivo, "%lld", &k);

    
    // Creamos el arreglo que almacena los objetivos en el teatro
    Celda *objetivos = (Celda *)calloc(k, sizeof(Celda));
    
    
    // Leemos cada uno de los objetos y se asignan en el teatro
    for (long long i = 0; i < k; i++) {
        long long x, y, resistencia;
        
        // Leemos coordenadas y resistencia para cada objeto
        fscanf(archivo, "%lld %lld %lld", &x, &y, &resistencia);

        // Si es una tierra baldia, entonces pasamos al siguiente objeto
        if (resistencia == 0){
            continue;
        }
        // Se guarda el objeto en el arreglo
        objetivos[i].resistencia = resistencia;
        objetivos[i].resistencia_inicial = resistencia;
        objetivos[i].tipo = (resistencia < 0) ? 1 : 2;

        // Se guarda el apuntador al objeto en el teatro
        teatro[x][y] = &objetivos[i];
    }
    
    // Leemos la cantidad drones
    fscanf(archivo, "%lld", &l);
    
    // Calculamos el minimo entre el numero de drones y la cantidad de celdas
    long long minimo = n*m < l ? n*m : l;

    // Si el numero de procesos ingresados por el usuario es mayor que "minimo", entonces el numero
    // de procesos pasa a ser este minimo.
    if (num_hilos > minimo){
        num_hilos = minimo;
    }

    // Declaramos el arreglo de drones
    Dron *drones = (Dron *)malloc(l * sizeof(Dron));
    
    for (long long i = 0; i < l; i++) {
        fscanf(archivo, "%lld %lld %lld %lld", &drones[i].x, &drones[i].y, 
            &drones[i].rd, &drones[i].pe);
    }
    
    // Creamos el arreglo de hilos
   
    pthread_t *hilos = (pthread_t *)malloc(num_hilos * sizeof(pthread_t));

    // Creamos el arreglo de los argumentos para cada hilo
    
    HiloArgs *args = (HiloArgs *)malloc(num_hilos * sizeof(HiloArgs));
    
    // Se asigna los argumentos a cada hilo
    for (long long i = 0; i < num_hilos; i++) {
        args[i].inicio = i * (l / num_hilos);
        args[i].fin = (i == num_hilos - 1) ? l : args[i].inicio + (l / num_hilos);
        args[i].n = n;
        args[i].m = m;
        args[i].num_drones = l / num_hilos;
        args[i].objetivos = objetivos;
        args[i].teatro = teatro;
        args[i].drones = drones;

        // Se crea el hilo para que ejecute la funcion procesar_drones con sus 
        //respectivos argumentos
        if (pthread_create(&hilos[i], NULL, procesar_drones, &args[i]) != 0) {
            perror("Error creando hilo");
            exit(1);
        }
    }

    // Esperamos a que todos los hilos terminen
    for (long long i = 0; i < num_hilos; i++) {
        if (pthread_join(hilos[i], NULL) != 0) {
            perror("Error al esperar hilo");
            exit(1);
        }
    }

    // Variables para guardar los resultados
    long long om_intactos = 0, om_parciales = 0, om_destruidos = 0, 
              ic_intactos = 0, ic_parciales = 0, ic_destruidos = 0;

    // Recorremos el arreglo de objetivos para analizar los resultados luego del ataque
    for (long long i = 0; i < k; i++) {
        if (objetivos[i].tipo == 1) {
            if (objetivos[i].resistencia == objetivos[i].resistencia_inicial) {
                om_intactos++;
            }
            else if (objetivos[i].resistencia >= 0) {
                om_destruidos++;
            }
            else if (objetivos[i].resistencia < 0 && objetivos[i].resistencia > objetivos[i].resistencia_inicial) {
                om_parciales++;
            }
        } else if (objetivos[i].tipo == 2) {
            if (objetivos[i].resistencia == objetivos[i].resistencia_inicial) {
                ic_intactos++;
            }
            else if (objetivos[i].resistencia <= 0) {
                ic_destruidos++;
            }
            else if (objetivos[i].resistencia > 0 && objetivos[i].resistencia < objetivos[i].resistencia_inicial) {
                ic_parciales++;
            }
        }     
    }

    // Imprimimos los resultados
    printf("OM intactos: %lld\n", om_intactos);
    printf("OM parcialmente destruidos: %lld\n", om_parciales);
    printf("OM totalmente destruidos: %lld\n", om_destruidos);
    printf("IC intactos: %lld\n", ic_intactos);
    printf("IC parcialmente destruidos: %lld\n", ic_parciales);
    printf("IC totalmente destruidos: %lld\n", ic_destruidos);

    // Libera la memoria dinámica
    for (long long i = 0; i < n; i++) {
        free(teatro[i]);
    }

    // Liberamos memoria
    free(teatro);
    free(objetivos);
    free(drones);
    free(hilos);
    free(args);

    // Destruye el mutex
    pthread_mutex_destroy(&mutex);

    fclose(archivo);
    return 0;
}
