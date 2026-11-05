#!bin/bash
if[ $# != 3 ] then
	exit 1
else
	if[ ! -f $1 ] then
		exit 1
	else
		if [[ $2 = "ESTADO" || $2 = "AREA1" || $2="AREA2"]] then
			if [[ $3 = "ESTADO0" || $3 = "ESTADO1" ]] then
				echo $(grep $2 $1 | ec -l)
			else
				echo -e "El estado no existe"
				eit 1
			fi
		else
			echo -e "El area no existe"
			exit 1
		fi
	fi
fi

exit 0
