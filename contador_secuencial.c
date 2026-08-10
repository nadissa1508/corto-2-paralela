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
 * Descripción: Implementa un contador secuencial de frecuencia de
 *              palabras. Lee texto.txt con un unico flujo de
 *              ejecucion, procesa las palabras una por una y las
 *              almacena en un diccionario implementado como tabla
 *              hash. 
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
 * Busca la palabra en la tabla:
 *  - si existe, incrementa su frecuencia
 *  - si no existe, la inserta con frecuencia 1
 */
void insertar_o_incrementar(Nodo *tabla[], const char *palabra) {
    unsigned int indice = funcion_hash(palabra);
    Nodo *actual = tabla[indice];

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
    nuevo->siguiente = tabla[indice];
    tabla[indice] = nuevo;
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
    printf("\n===== RESULTADOS FINALES (SECUENCIAL) =====\n");
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
    /* Abrir el archivo (hardcodeado texto.txt) */
    FILE *archivo = fopen(NOMBRE_ARCHIVO, "r");

    /* Verificar apertura correcta */
    if (archivo == NULL) {
        printf("Error: no se pudo abrir el archivo '%s'\n", NOMBRE_ARCHIVO);
        return 1;
    }

    /* Inicializar el diccionario vacio y el total en 0 */
    Nodo *tabla_hash[TABLE_SIZE] = { NULL };
    int total_palabras = 0;

    /* El cronometro total arranca antes de leer, igual que en la
       version paralela, para que el tiempo total sea comparable. */
    clock_t inicio_total = clock();

    /* ---- Fase 1: lectura del archivo hacia un arreglo en memoria ---- */
    int capacidad = 100;
    char **palabras = (char **)malloc((size_t)capacidad * sizeof(char *));
    if (palabras == NULL) {
        fprintf(stderr, "Error: no hay memoria disponible.\n");
        fclose(archivo);
        return 1;
    }

    int cantidad_palabras = 0;
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

    /* Cerrar el archivo */
    fclose(archivo);

    /* validar cantidad de palabras > 0 */
    if (cantidad_palabras == 0) {
        printf("Error: el archivo no contiene palabras.\n");
        free(palabras);
        return 1;
    }

    clock_t fin_lectura = clock();

    /* ---- Fase 2: conteo secuencial sobre el arreglo ya leido ---- */
    for (int i = 0; i < cantidad_palabras; i++) {
        insertar_o_incrementar(tabla_hash, palabras[i]);
        total_palabras++;
    }

    clock_t fin_conteo = clock();

    double t_lectura = (double)(fin_lectura - inicio_total) / CLOCKS_PER_SEC;
    double t_conteo  = (double)(fin_conteo - fin_lectura) / CLOCKS_PER_SEC;
    double t_total   = (double)(fin_conteo - inicio_total) / CLOCKS_PER_SEC;

    /* mostrar resultados finales */
    mostrar_resultados(tabla_hash, total_palabras);
    printf("Tiempo de lectura del archivo:        %.6f segundos\n", t_lectura);
    printf("Tiempo de conteo (secuencial):         %.6f segundos\n", t_conteo);
    printf("Tiempo de ejecucion total:              %.6f segundos\n", t_total);

    liberar_tabla(tabla_hash);
    for (int i = 0; i < cantidad_palabras; i++) {
        free(palabras[i]);
    }
    free(palabras);

    /* finalizar el programa */
    return 0;
}
