#!/bin/bash
set -e

testN=$1
cfg=$2

JM=/home/gridsan/je23693/compression_benchmarks/JM/bin
dir=/home/gridsan/je23693/compression_benchmarks/tests/h264/JM/$testN
mkdir $dir
touch $dir/out.log

counter=1
while [ $counter -le 15 ]
do
    echo $counter
    echo "=======" &>>$dir/out.log
    echo $counter  &>>$dir/out.log
    $JM/lencod.exe -d $cfg &>> $dir/out.log
    ((counter++))
done

cp $cfg $dir
mv $JM/test.264 $dir
ffmpeg -y -i $dir/test.264 -vcodec libx264 $dir/JM.mp4
