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

    clock_t inicio = clock();

    /* Mientras haya mas palabras leer, verificar si esta en el
       diccionario, incrementar o insertar, y actualizar el total. */
    char buffer[MAX_PALABRA];
    while (fscanf(archivo, "%99s", buffer) == 1) {
        insertar_o_incrementar(tabla_hash, buffer);
        total_palabras++;
    }

    /* validar cantidad de palabras > 0 */
    if (total_palabras == 0) {
        printf("Error: el archivo no contiene palabras.\n");
        fclose(archivo);
        return 1;
    }

    /* Cerrar el archivo */
    fclose(archivo);

    clock_t fin = clock();
    double segundos = (double)(fin - inicio) / CLOCKS_PER_SEC;

    /* mostrar resultados finales */
    mostrar_resultados(tabla_hash, total_palabras);
    printf("Tiempo de ejecucion (secuencial): %.6f segundos\n", segundos);

    liberar_tabla(tabla_hash);

    /* finalizar el programa */
    return 0;
}
