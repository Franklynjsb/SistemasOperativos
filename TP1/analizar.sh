#!/bin/bash

if [ $# != 3 ]
then
	exit 1
else
	if [ ! -f $1 ]
then
		exit 1
	else
		if [[ $2 = "AREA0" || $2 = "AREA1" || $2 = "AREA2" ]]
		then
			if [[ $3 = "ESTADO0" || $3 = "ESTADO1" ]]
			then
				echo "TOTAL="$(wc -l < ./datos/datos_tp1.txt)
				echo "AREA="$2
				echo "CANTIDAD_AREA="$(grep $2 $1 |  wc -l)
				echo "ESTADO="$3
				echo "CANTIDAD_ESTADO="$(grep $3 $1 | wc -l)			
				echo "CANTIDAD_AREA_ESTADO="$(grep $2 $1 | grep $3 | wc -l)
				sumatoria=0
				for i in $(grep $2 $1 | grep $3 | cut -d ";" -f 4) 
				do 
					sumatoria=$((sumatoria + i)) 
				done 
				echo "SUMA_AREA_ESTADO="$sumatoria
				
			else
				echo -e "El estado no existe"
				exit 1
			fi
		else
			echo -e "El area no existe"
			exit 1
		fi
	fi
fi



exit 0
