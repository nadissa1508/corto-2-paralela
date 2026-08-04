/*
 * Universidad del Valle de Guatemala
 * Departamento de Computación
 * Computación Paralela - CC3069
 *
 * Autores:
 *   - Angie Vela, 23764
 *   - Javier Linares, 231135
 *   - Roberto Camposeco, 23968
 *
 * Archivo: contador_paralelo.c
 * Descripción: Implementa un contador paralelo de frecuencia de
 *              palabras utilizando hilos POSIX (pthreads). Lee las
 *              palabras de texto.txt, calcula el total y las divide
 *              en NUM_HILOS bloques de tamano similar. Cada hilo
 *              recibe un bloque y cuenta sus palabras en un
 *              diccionario local propio (tabla hash independiente),
 *              sin tocar memoria compartida durante el conteo, por
 *              lo que ya no se necesita usar mutex.
 *
 *              La sincronizacion entre hilos se logra con
 *              pthread_join: el hilo principal espera a que todos
 *              terminen antes de continuar. Una vez sincronizados,
 *              el hilo principal combina (merge) los NUM_HILOS
 *              diccionarios parciales en un unico diccionario
 *              global, de forma secuencial.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define NOMBRE_ARCHIVO "texto.txt"
#define NUM_HILOS 3
#define TABLE_SIZE 211
#define MAX_PALABRA 100

/* ---------- Diccionario (tabla hash con encadenamiento) ---------- */
typedef struct Nodo {
    char palabra[MAX_PALABRA];
    int frecuencia;
    struct Nodo *siguiente;
} Nodo;

/* Lista de palabras leidas del archivo (solo lectura durante el
   conteo, todos los hilos leen del mismo arreglo, cada uno en su
   propio rango */
char **palabras = NULL;
int cantidad_palabras = 0;

/* Argumentos que recibe cada hilo */
typedef struct {
    int id_hilo;
    int inicio;
    int fin;
    Nodo **tabla_local;   /* diccionario propio del hilo (conteo parcial) */
    int total_local;      /* contador propio del hilo */
} ArgsHilo;

/* ---------- Funcion hash simple (djb2) ---------- */
unsigned int funcion_hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + (unsigned int)c;
    }
    return hash % TABLE_SIZE;
}

/*
 * Busca la palabra en una tabla y le suma cantidad a su frecuencia
 * (la inserta con esa frecuencia si no existia todavia).
 * Se usa tanto para el conteo local de cada hilo (cantidad = 1, una
 * palabra a la vez) como para la combinacion final en el hilo
 * principal (cantidad = frecuencia parcial de cada hilo).
 */
void insertar_o_sumar(Nodo *tabla[], const char *palabra, int cantidad) {
    unsigned int indice = funcion_hash(palabra);
    Nodo *actual = tabla[indice];

    while (actual != NULL) {
        if (strcmp(actual->palabra, palabra) == 0) {
            actual->frecuencia += cantidad;
            return;
        }
        actual = actual->siguiente;
    }

    Nodo *nuevo = (Nodo *)malloc(sizeof(Nodo));
    if (nuevo == NULL) {
        fprintf(stderr, "Error: no hay memoria disponible.\n");
        exit(1);
    }
    strncpy(nuevo->palabra, palabra, MAX_PALABRA - 1);
    nuevo->palabra[MAX_PALABRA - 1] = '\0';
    nuevo->frecuencia = cantidad;
    nuevo->siguiente = tabla[indice];
    tabla[indice] = nuevo;
}

/*
 * Funcion que ejecuta cada hilo.
 * Recorre el bloque de palabras [inicio, fin) que le fue asignado y
 * construye su propio diccionario local. 
 */
void *contar_palabras(void *args) {
    ArgsHilo *a = (ArgsHilo *)args;
    int i = a->inicio;

    while (i < a->fin) {                  // hay mas palabras por leer en mi bloque? 
        char *p = palabras[i];            // leer palabra
        insertar_o_sumar(a->tabla_local, p, 1);
        a->total_local++;
        i++;
    }

    printf("Hilo %d finalizo: proceso %d palabras (rango [%d, %d)) en su diccionario local\n",
           a->id_hilo, a->fin - a->inicio, a->inicio, a->fin);

    return NULL;
}

/* Libera toda la memoria dinamica de una tabla */
void liberar_tabla(Nodo *tabla[]) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Nodo *actual = tabla[i];
        while (actual != NULL) {
            Nodo *tmp = actual;
            actual = actual->siguiente;
            free(tmp);
        }
    }
}

/* Muestra el diccionario completo y el total de palabras */
void mostrar_resultados(Nodo *tabla[], int total) {
    printf("\n===== RESULTADOS FINALES (PARALELO) =====\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        Nodo *actual = tabla[i];
        while (actual != NULL) {
            printf("%-20s -> %d\n", actual->palabra, actual->frecuencia);
            actual = actual->siguiente;
        }
    }
    printf("-------------------------------\n");
    printf("Total de palabras contadas: %d\n", total);
}

int main(void) {
    /* Abrir el archivo */
    FILE *archivo = fopen(NOMBRE_ARCHIVO, "r");

    /* Verificar apertura correcta */
    if (archivo == NULL) {
        printf("Error: no se pudo abrir el archivo '%s'\n", NOMBRE_ARCHIVO);
        return 1;
    }

    /* Inicializar el diccionario global vacio y el total en 0 */
    Nodo *tabla_global[TABLE_SIZE] = { NULL };
    int total_palabras = 0;

    /* Leer todas las palabras del archivo hacia un arreglo dinamico */
    int capacidad = 100;
    palabras = (char **)malloc((size_t)capacidad * sizeof(char *));
    if (palabras == NULL) {
        fprintf(stderr, "Error: no hay memoria disponible.\n");
        fclose(archivo);
        return 1;
    }

    char buffer[MAX_PALABRA];
    while (fscanf(archivo, "%99s", buffer) == 1) {
        if (cantidad_palabras >= capacidad) {
            capacidad *= 2;
            char **tmp = (char **)realloc(palabras, (size_t)capacidad * sizeof(char *));
            if (tmp == NULL) {
                fprintf(stderr, "Error: no hay memoria disponible.\n");
                fclose(archivo);
                return 1;
            }
            palabras = tmp;
        }
        palabras[cantidad_palabras] = (char *)malloc(strlen(buffer) + 1);
        if (palabras[cantidad_palabras] == NULL) {
            fprintf(stderr, "Error: no hay memoria disponible.\n");
            fclose(archivo);
            return 1;
        }
        strcpy(palabras[cantidad_palabras], buffer);
        cantidad_palabras++;
    }
    fclose(archivo);

    /* Validacion: la cantidad de palabras debe ser > 0 */
    if (cantidad_palabras == 0) {
        printf("Error: el archivo no contiene palabras.\n");
        free(palabras);
        return 1;
    }

    printf("Se leyeron %d palabras del archivo.\n", cantidad_palabras);

    clock_t inicio = clock();

    /* Dividir en partes iguales y asignar un bloque a cada hilo */
    pthread_t hilos[NUM_HILOS];
    ArgsHilo args[NUM_HILOS];

    int base = cantidad_palabras / NUM_HILOS;
    int resto = cantidad_palabras % NUM_HILOS;
    int inicio_bloque = 0;

    for (int i = 0; i < NUM_HILOS; i++) {
        int tam_bloque = base + (i < resto ? 1 : 0);
        args[i].id_hilo = i + 1;
        args[i].inicio = inicio_bloque;
        args[i].fin = inicio_bloque + tam_bloque;
        args[i].total_local = 0;

        /* Cada hilo recibe su propia tabla hash local, ya inicializada en NULL */
        args[i].tabla_local = (Nodo **)calloc(TABLE_SIZE, sizeof(Nodo *));
        if (args[i].tabla_local == NULL) {
            fprintf(stderr, "Error: no hay memoria disponible.\n");
            return 1;
        }

        inicio_bloque += tam_bloque;
    }

    /* Crear e iniciar los hilos */
    for (int i = 0; i < NUM_HILOS; i++) {
        if (pthread_create(&hilos[i], NULL, contar_palabras, &args[i]) != 0) {
            fprintf(stderr, "Error: no se pudo crear el hilo %d\n", i + 1);
            return 1;
        }
    }

    /* Esperar a que todos los hilos terminen (join)
          Este es el punto de sincronizacion, nadie puede combinar
          resultados hasta que todos los hilos hayan terminado */
    for (int i = 0; i < NUM_HILOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    /* Combinar los diccionarios locales en el diccionario global */
    for (int i = 0; i < NUM_HILOS; i++) {
        for (int b = 0; b < TABLE_SIZE; b++) {
            Nodo *actual = args[i].tabla_local[b];
            while (actual != NULL) {
                insertar_o_sumar(tabla_global, actual->palabra, actual->frecuencia);
                actual = actual->siguiente;
            }
        }
        total_palabras += args[i].total_local;
        liberar_tabla(args[i].tabla_local);
        free(args[i].tabla_local);
    }

    clock_t fin = clock();
    double segundos = (double)(fin - inicio) / CLOCKS_PER_SEC;

    /* mostrar resultados finales */
    mostrar_resultados(tabla_global, total_palabras);
    printf("Tiempo de ejecucion (paralelo, %d hilos): %.6f segundos\n", NUM_HILOS, segundos);

    /* Liberar memoria */
    liberar_tabla(tabla_global);
    for (int i = 0; i < cantidad_palabras; i++) {
        free(palabras[i]);
    }
    free(palabras);

    /* Finalizar el programa */
    return 0;
}
