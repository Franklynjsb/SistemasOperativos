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
				echo "TOTAL=" $(wc -l < ./datos/datos_tp1.txt)
				echo "AREA="$2
				echo "CANTIDAD_AREA=" $(grep $1 $1 |  wc -l)
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
