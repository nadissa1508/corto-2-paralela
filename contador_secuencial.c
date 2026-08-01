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
 * Archivo: contador_secuencial.c
 * Descripción: Implementa un contador secuencial de frecuencia de palabras.
 *              Lee las palabras del archivo texto.txt y las procesa una
 *              por una utilizando un único flujo de ejecución, sin hilos
 *              ni mecanismos de sincronización.
 *
 *              Utiliza un diccionario implementado mediante una tabla hash
 *              con encadenamiento para almacenar la frecuencia de cada
 *              palabra. El tiempo de ejecución del conteo es medido para
 *              compararlo con la versión paralela basada en pthreads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NOMBRE_ARCHIVO "texto.txt"
#define TABLE_SIZE 211
#define MAX_PALABRA 100

/* ---------- Diccionario (tabla hash con encadenamiento) ---------- */
typedef struct Nodo {
    char palabra[MAX_PALABRA];
    int frecuencia;
    struct Nodo *siguiente;
} Nodo;

Nodo *tabla_hash[TABLE_SIZE];
int total_palabras = 0;

char **palabras = NULL;
int cantidad_palabras = 0;

/* ---------- Funcion hash simple (djb2), identica a la version paralela ---------- */
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
 * Recorre TODAS las palabras leidas del archivo, una por una,
 * y actualiza el diccionario y el total. Equivale a lo que hacia
 * cada hilo en la version paralela, pero aqui sobre el arreglo completo.
 */
void contar_palabras_secuencial(void) {
    for (int i = 0; i < cantidad_palabras; i++) {
        char *p = palabras[i];
        insertar_o_incrementar(p);
        total_palabras += 1;
    }
}

/* Libera toda la memoria dinamica usada */
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

    /* Contar frecuencias de forma secuencial */
    clock_t inicio = clock();
    contar_palabras_secuencial();
    clock_t fin = clock();
    double segundos = (double)(fin - inicio) / CLOCKS_PER_SEC;

    /* Cerrar el archivo */
    fclose(archivo);

    /* Mostrar resultados finales */
    mostrar_resultados();
    printf("Tiempo de conteo (secuencial): %.6f segundos\n", segundos);

    /* Liberar memoria */
    liberar_recursos();

    return 0;
}
