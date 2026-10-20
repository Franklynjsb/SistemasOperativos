#!/bin/bash
for i in {1..1000}
do
echo "$i;AREA$((i%3));ESTADO$((i%2));$(((i*37)%500+1))"
done > datos_tp1.txt
