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
				cant_A_E=$(grep $2 $1 | grep $3 | wc -l)
				echo "CANTIDAD_AREA_ESTADO="$cant_A_E

				#variable=0
				#
				#for x in $(grep $2 $1 | grep $3)
				#do	
				#	num="${x##*;}"
				#	#echo "Renglon:" $num
				#	variable=$((variable+=num))
				#done

				variable=0
				for x in $(grep $2 $1 | grep $3 | cut -d ';' -f4)
				do
					variable=$((variable+x))
				done

				echo "SUMA_AREA_ESTADO="$variable
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
