# Hoja de Trabajo 1 - Computación Paralela

 #### Universidad del Valle de Guatemala
 #### Departamento de Computación
 #### Computación Paralela - CC3069
 
 Autores:
   - Angie Vela, 23764
   - Javier Linares, 231135
   - Roberto Camposeco, 23968

Medición y comparación de rendimiento (tiempo, speedup, efficiency) entre la versión secuencial y la versión paralela del conteo de frecuencia de palabras.

## Programas

- `contador_secuencial.c`: cuenta la frecuencia de palabras de `texto.txt` con un solo hilo.
- `contador_paralelo.c`: hace el mismo conteo, pero repartido entre varios hilos (pthreads).

### Compilar

```
gcc -O2 -o contador_secuencial.exe contador_secuencial.c
gcc -O2 -o contador_paralelo.exe contador_paralelo.c -lpthread
```

### Correr

```
./contador_secuencial.exe
./contador_paralelo.exe [num_hilos]   # si no se indica, usa 3 hilos por defecto
```

Ambos imprimen el tiempo de lectura, el tiempo de conteo y el tiempo total.

## Benchmark (Hoja de Trabajo 1)

```
./benchmark.sh [RUNS]   # 5 corridas por defecto
```

Qué hace:
1. Compila los dos programas.
2. Corre `RUNS` veces cada configuración: secuencial, paralelo con 2 hilos y paralelo con 4 hilos.
3. Guarda los tiempos de cada corrida en `benchmark_tiempos.csv`.
4. Calcula el promedio, el speedup y la efficiency, y los guarda en `benchmark_resumen.csv`.

## Fórmulas

```
Speedup    = Tiempo secuencial promedio / Tiempo paralelo promedio
Efficiency = Speedup / Número de hilos
```

## Archivos generados

| Archivo | Contiene |
|---|---|
| `benchmark_tiempos.csv` | Tiempo de conteo de cada corrida (5 secuencial, 5 con 2 hilos, 5 con 4 hilos) |
| `benchmark_resumen.csv` | Promedio, speedup y efficiency por versión |
