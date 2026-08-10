#!/usr/bin/env bash
#
# Universidad del Valle de Guatemala
# Departamento de Computación
# Computación Paralela - CC3069
#
# Autores:
#   - Angie Vela, 23764
#   - Javier Linares, 231135
#   - Roberto Camposeco, 23968
#
# Que hace:
#   1. Compila contador_secuencial.c y contador_paralelo.c
#   2. Corre cada configuracion RUNS veces (5 por defecto) sobre el mismo
#      texto.txt y extrae la linea "Tiempo de conteo" que imprime cada
#      programa (ya instrumentada para medir solo la fase de conteo,
#      igual en las dos versiones).
#   3. Calcula el promedio, speedup (t_secuencial_prom / t_paralelo_prom)
#      y efficiency (speedup / num_hilos).
#
# Salida:
#   benchmark_tiempos.csv  -> tiempos crudos de cada corrida
#   benchmark_resumen.csv  -> promedios, speedup y efficiency por version
#
# Uso: ./benchmark.sh [RUNS]

set -e

RUNS="${1:-5}"
RAW_CSV="benchmark_tiempos.csv"
SUMMARY_CSV="benchmark_resumen.csv"

echo "Compilando..."
gcc -O2 -Wall -o contador_secuencial.exe contador_secuencial.c
gcc -O2 -Wall -o contador_paralelo.exe contador_paralelo.c -lpthread

extraer_tiempo() {
    # Toma la linea "Tiempo de conteo (...): X.XXXXXX segundos" y devuelve X.XXXXXX
    grep "Tiempo de conteo" | grep -oE '[0-9]+\.[0-9]+'
}

echo "Corriendo $RUNS repeticiones por configuracion (secuencial, paralelo 2 hilos, paralelo 4 hilos)..."
echo "run,secuencial,paralelo_2hilos,paralelo_4hilos" > "$RAW_CSV"

for i in $(seq 1 "$RUNS"); do
    t_seq=$(./contador_secuencial.exe | extraer_tiempo)
    t_p2=$(./contador_paralelo.exe 2 | extraer_tiempo)
    t_p4=$(./contador_paralelo.exe 4 | extraer_tiempo)
    echo "$i,$t_seq,$t_p2,$t_p4" >> "$RAW_CSV"
    echo "  corrida $i -> secuencial=${t_seq}s  paralelo2=${t_p2}s  paralelo4=${t_p4}s"
done

echo "Tiempos crudos guardados en $RAW_CSV"

awk -F',' -v runs="$RUNS" -v summary="$SUMMARY_CSV" '
NR > 1 {
    seq_sum += $2; p2_sum += $3; p4_sum += $4
}
END {
    seq_avg = seq_sum / runs
    p2_avg  = p2_sum  / runs
    p4_avg  = p4_sum  / runs

    sp2 = seq_avg / p2_avg
    sp4 = seq_avg / p4_avg
    ef2 = sp2 / 2
    ef4 = sp4 / 4

    printf "\n===== RESUMEN (promedio de %d corridas) =====\n", runs
    printf "%-22s %12s %12s %12s\n", "", "Secuencial", "Paralelo 2h", "Paralelo 4h"
    printf "%-22s %12.6f %12.6f %12.6f\n", "Tiempo (s)", seq_avg, p2_avg, p4_avg
    printf "%-22s %12s %12.4f %12.4f\n", "Speedup", "1.0000", sp2, sp4
    printf "%-22s %12s %12.4f %12.4f\n", "Efficiency", "1.0000", ef2, ef4

    print "version,tiempo_prom,speedup,efficiency" > summary
    printf "secuencial,%.6f,1.0000,1.0000\n", seq_avg >> summary
    printf "paralelo_2hilos,%.6f,%.4f,%.4f\n", p2_avg, sp2, ef2 >> summary
    printf "paralelo_4hilos,%.6f,%.4f,%.4f\n", p4_avg, sp4, ef4 >> summary
}
' "$RAW_CSV"

echo "Resumen guardado en $SUMMARY_CSV"
