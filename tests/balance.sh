#!/bin/bash

for((j=4;j<=32;j*=2))
do
	: 
	for((i=0;i<j-1;i++))
	do 
		:
		cat pthread_av_tb_test_3_1080_$j.txt | grep "RES $i " | awk '{print $3}' >> 3_1080/3_1080_${j}_${i}.txt
		cat pthread_av_tb_test_4_720_$j.txt | grep "RES $i " | awk '{print $3}' >> 4_720/4_720_${j}_${i}.txt
		cat pthread_av_tb_test_5_1080_$j.txt | grep "RES $i " | awk '{print $3}' >> 5_1080/5_1080_${j}_${i}.txt
	done
done

