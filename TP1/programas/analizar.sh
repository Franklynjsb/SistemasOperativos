#!/bin/bash

if [ $# != 3 ]
then
	echo "Se esperaban 3 parametros y se recibieron $#" >&2
	exit 1
else
	if [ ! -f "$1" ]
	then
		echo "El '$1' no existe o no es un archivo regular" >&2
		exit 1
	else
		if [[ "$2" = "AREA0" || "$2" = "AREA1" || "$2" = "AREA2" ]]
		then
			if [[ "$3" = "ESTADO0" || "$3" = "ESTADO1" ]]
			then
				mkdir -p ../resultados ../respaldo
				{
					echo "TOTAL=$(wc -l < "$1")"
					echo "AREA=$2"
					echo "CANTIDAD_AREA=$(grep "$2" "$1" | wc -l)"
					echo "ESTADO=$3"
					echo "CANTIDAD_ESTADO=$(grep "$3" "$1" | wc -l)"
					echo "CANTIDAD_AREA_ESTADO=$(grep "$2" "$1" | grep "$3" | wc -l)"
					sumatoria=0
					for i in $(grep "$2" "$1" | grep "$3" | cut -d ";" -f 4)
					do
						sumatoria=$((sumatoria + i))
					done
					echo "SUMA_AREA_ESTADO=$sumatoria"
				} > ../resultados/resultado.txt
				cp ../resultados/resultado.txt ../respaldo/resultado.bak
				cat ../resultados/resultado.txt
			else
				echo "El estado ingresado no existe (ESTADO0 o ESTADO1)" >&2
				exit 1
			fi
		else
			echo "El area ingresada no existe (AREA0, AREA1 o AREA2)" >&2
			exit 1
		fi
	fi
fi

# Problema 7: integracion con los programas en C
./analiza_archivo "$1"
if [ $? -eq 0 ]; then echo "analiza_archivo finalizo correctamente"; else echo "analiza_archivo finalizo con ERROR"; fi

./procesa_datos "$1" "$2"
if [ $? -eq 0 ]; then echo "procesa_datos finalizo correctamente"; else echo "procesa_datos finalizo con ERROR"; fi

exit 0
