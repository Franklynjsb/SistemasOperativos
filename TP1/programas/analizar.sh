#!/bin/bash
# analizar.sh - TP1 Sistemas Operativos UTN FRSF 2026
# Uso: ./analizar.sh <archivo> <area> <estado>

# ---------- Validaciones ----------
if [ $# -ne 3 ]; then
    echo "ERROR: se requieren exactamente 3 parametros (recibidos: $#)." >&2
    echo "Uso: $0 <archivo> <area> <estado>" >&2
    exit 1
fi

ARCHIVO="$1"
AREA="$2"
ESTADO="$3"

if [ ! -e "$ARCHIVO" ]; then
    echo "ERROR: el archivo '$ARCHIVO' no existe." >&2
    exit 2
fi
if [ ! -f "$ARCHIVO" ]; then
    echo "ERROR: '$ARCHIVO' no es un archivo regular." >&2
    exit 3
fi

# Validacion de AREA (estructura repetitiva)
AREA_OK=0
for a in AREA0 AREA1 AREA2; do
    if [ "$AREA" = "$a" ]; then
        AREA_OK=1
    fi
done
if [ $AREA_OK -eq 0 ]; then
    echo "ERROR: AREA invalida '$AREA'. Valores permitidos: AREA0, AREA1, AREA2." >&2
    exit 4
fi

# Validacion de ESTADO
ESTADO_OK=0
for e in ESTADO0 ESTADO1; do
    if [ "$ESTADO" = "$e" ]; then
        ESTADO_OK=1
    fi
done
if [ $ESTADO_OK -eq 0 ]; then
    echo "ERROR: ESTADO invalido '$ESTADO'. Valores permitidos: ESTADO0, ESTADO1." >&2
    exit 5
fi

# ---------- Directorios de salida ----------
for d in resultados respaldo; do
    if [ ! -d "$d" ]; then
        mkdir -p "$d"
    fi
done

# ---------- Procesamiento ----------
TOTAL=$(wc -l < "$ARCHIVO")
CANTIDAD_AREA=$(grep -c ";$AREA;" "$ARCHIVO")
CANTIDAD_ESTADO=$(grep -c ";$ESTADO;" "$ARCHIVO")
CANTIDAD_AREA_ESTADO=$(grep ";$AREA;" "$ARCHIVO" | grep -c ";$ESTADO;")
SUMA_AREA_ESTADO=$(grep ";$AREA;" "$ARCHIVO" | grep ";$ESTADO;" \
    | cut -d';' -f4 | awk '{ s += $1 } END { print s + 0 }')

{
    echo "TOTAL=$TOTAL"
    echo "AREA=$AREA"
    echo "CANTIDAD_AREA=$CANTIDAD_AREA"
    echo "ESTADO=$ESTADO"
    echo "CANTIDAD_ESTADO=$CANTIDAD_ESTADO"
    echo "CANTIDAD_AREA_ESTADO=$CANTIDAD_AREA_ESTADO"
    echo "SUMA_AREA_ESTADO=$SUMA_AREA_ESTADO"
} > resultados/resultado.txt

cat resultados/resultado.txt
cp resultados/resultado.txt respaldo/resultado.bak

# ---------- Integracion (Problema 7) ----------
# Ejecuta los programas en C y reporta si cada uno finalizo correctamente.
# Todos los archivos producidos quedan en resultados/.
RC_FINAL=0
INTEG=resultados/integracion.txt
: > "$INTEG"

ejecutar_programa() {
    # $1 = nombre del programa, resto = argumentos
    local prog="$1"
    shift
    local bin="" d
    # Busca el ejecutable en el directorio actual, en programas/ y junto al script
    for d in . programas "$(dirname "$0")" "$(dirname "$0")/programas"; do
        if [ -x "$d/$prog" ] && [ -f "$d/$prog" ]; then
            bin="$d/$prog"
            break
        fi
    done
    # Si no existe, intenta compilarlo desde el fuente
    if [ -z "$bin" ]; then
        for d in . programas "$(dirname "$0")" "$(dirname "$0")/programas"; do
            if [ -f "$d/$prog.c" ]; then
                gcc -o "$prog" "$d/$prog.c" 2> "resultados/compilacion_$prog.txt" \
                    && bin="./$prog"
                break
            fi
        done
    fi
    if [ -z "$bin" ]; then
        echo "PROGRAMA $prog: NO DISPONIBLE (sin ejecutable y no pudo compilarse)" \
            | tee -a "$INTEG"
        echo "No se pudo ejecutar $prog" > "resultados/salida_$prog.txt"
        return 1
    fi
    "$bin" "$@" > "resultados/salida_$prog.txt" 2>&1
    local rc=$?
    if [ $rc -eq 0 ]; then
        echo "PROGRAMA $prog: finalizo correctamente (codigo 0)" | tee -a "$INTEG"
    else
        echo "PROGRAMA $prog: finalizo con ERROR (codigo $rc)" | tee -a "$INTEG"
    fi
    return $rc
}

echo "--- Ejecucion de programas C ---"
ejecutar_programa analiza_archivo "$ARCHIVO" || RC_FINAL=6
ejecutar_programa procesa_datos "$ARCHIVO" "$AREA" || RC_FINAL=7

echo "--- Salida de analiza_archivo ---"
cat resultados/salida_analiza_archivo.txt
echo "--- Salida de procesa_datos ---"
cat resultados/salida_procesa_datos.txt

echo "--- Archivos en resultados/ ---"
ls -1 resultados

exit $RC_FINAL
