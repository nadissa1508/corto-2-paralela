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
 * Descripción: Implementa un contador paralelo de frecuencia de palabras
 *              utilizando hilos POSIX (pthreads). Lee las palabras de
 *              texto.txt, las divide entre tres hilos y utiliza un
 *              diccionario global implementado como tabla hash para
 *              almacenar la frecuencia de cada palabra.
 *
 *              El acceso al diccionario y al contador global de palabras
 *              es protegido mediante un mutex para evitar condiciones
 *              de carrera entre los hilos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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

Nodo *tabla_hash[TABLE_SIZE];   /* diccionario global */
int total_palabras = 0;         /* total global */

/* Mutex que protege diccionario y total_palabras (recursos compartidos) */
pthread_mutex_t mutex_diccionario = PTHREAD_MUTEX_INITIALIZER;

/* Lista de palabras leidas del archivo (se llena antes de crear hilos) */
char **palabras = NULL;
int cantidad_palabras = 0;

/* Argumentos que recibe cada hilo: el bloque [inicio, fin) que le toca */
typedef struct {
    int id_hilo;
    int inicio;
    int fin;
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
 * Busca la palabra en el diccionario:
 *  - si existe, incrementa su frecuencia
 *  - si no existe, la inserta con frecuencia 1
 * NOTA: esta funcion NO es thread-safe por si sola; siempre se llama
 * dentro de la seccion protegida por mutex_diccionario.
 */
void insertar_o_incrementar(const char *palabra) {
    unsigned int indice = funcion_hash(palabra);
    Nodo *actual = tabla_hash[indice];

    while (actual != NULL) {
        if (strcmp(actual->palabra, palabra) == 0) {
            actual->frecuencia += 1;
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
    nuevo->frecuencia = 1;
    nuevo->siguiente = tabla_hash[indice];
    tabla_hash[indice] = nuevo;
}

/*
 * Funcion que ejecuta cada hilo.
 * Recorre el bloque de palabras [inicio, fin) que le fue asignado y por
 * cada una: verifica si esta en el diccionario, incrementa o inserta, y
 * actualiza el total. Toda esta seccion es critica porque el diccionario
 * y el total son compartidos entre los 3 hilos.
 */
void *contar_palabras(void *args) {
    ArgsHilo *a = (ArgsHilo *)args;
    int i = a->inicio;

    while (i < a->fin) {              /* ¿hay mas palabras por leer en mi bloque? */
        char *p = palabras[i];         /* leer la palabra */

        pthread_mutex_lock(&mutex_diccionario);   /* --- inicio seccion critica --- */
        insertar_o_incrementar(p);
        total_palabras += 1;
        pthread_mutex_unlock(&mutex_diccionario); /* --- fin seccion critica --- */

        i++;
    }

    printf("Hilo %d finalizo: proceso %d palabras (rango [%d, %d))\n",
           a->id_hilo, a->fin - a->inicio, a->inicio, a->fin);

    return NULL;
}

/* Libera toda la memoria dinamica usada (diccionario y arreglo de palabras) */
void liberar_recursos(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Nodo *actual = tabla_hash[i];
        while (actual != NULL) {
            Nodo *tmp = actual;
            actual = actual->siguiente;
            free(tmp);
        }
    }
    for (int i = 0; i < cantidad_palabras; i++) {
        free(palabras[i]);
    }
    free(palabras);
}

/* Muestra el diccionario completo y el total de palabras */
void mostrar_resultados(void) {
    printf("\n===== RESULTADOS FINALES =====\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        Nodo *actual = tabla_hash[i];
        while (actual != NULL) {
            printf("%-20s -> %d\n", actual->palabra, actual->frecuencia);
            actual = actual->siguiente;
        }
    }
    printf("-------------------------------\n");
    printf("Total de palabras contadas: %d\n", total_palabras);
}

int main(void) {
    /* Abrir el archivo */
    FILE *archivo = fopen(NOMBRE_ARCHIVO, "r");

    /* Verificar apertura correcta */
    if (archivo == NULL) {
        printf("Error: no se pudo abrir el archivo '%s'\n", NOMBRE_ARCHIVO);
        return 1;
    }

    /* Inicializar el diccionario vacio */
    for (int i = 0; i < TABLE_SIZE; i++) {
        tabla_hash[i] = NULL;
    }
    /* total_palabras ya inicia en 0 por ser variable global */

    /* Leer todas las palabras del archivo hacia un arreglo dinamico.
          Esto nos permite conocer la cantidad total de palabras y
          dividirlas en bloques iguales para cada hilo. */
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
                liberar_recursos();
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

    /* Validacion: la cantidad de palabras debe ser > 0 */
    if (cantidad_palabras == 0) {
        printf("Error: el archivo no contiene palabras.\n");
        fclose(archivo);
        free(palabras);
        return 1;
    }

    printf("Se leyeron %d palabras del archivo.\n", cantidad_palabras);

    /* Dividir en partes iguales y asignar un bloque a cada hilo */
    pthread_t hilos[NUM_HILOS];
    ArgsHilo args[NUM_HILOS];

    int base = cantidad_palabras / NUM_HILOS;
    int resto = cantidad_palabras % NUM_HILOS;
    int inicio = 0;

    for (int i = 0; i < NUM_HILOS; i++) {
        /* Los primeros "resto" hilos reciben una palabra extra para que
           la division quede lo mas pareja posible */
        int tam_bloque = base + (i < resto ? 1 : 0);
        args[i].id_hilo = i + 1;
        args[i].inicio = inicio;
        args[i].fin = inicio + tam_bloque;
        inicio += tam_bloque;
    }

    /* Crear e iniciar los hilos */
    for (int i = 0; i < NUM_HILOS; i++) {
        if (pthread_create(&hilos[i], NULL, contar_palabras, &args[i]) != 0) {
            fprintf(stderr, "Error: no se pudo crear el hilo %d\n", i + 1);
            return 1;
        }
    }

    /* Esperar a que todos los hilos terminen (join) */
    for (int i = 0; i < NUM_HILOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    /* Cerrar el archivo */
    fclose(archivo);

    /* mostrar resultados finales */
    mostrar_resultados();

    /* Liberar memoria y destruir el mutex */
    liberar_recursos();
    pthread_mutex_destroy(&mutex_diccionario);

    return 0;
}
