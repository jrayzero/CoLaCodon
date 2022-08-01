#!/bin/bash
set -e

testN=$1
cola=/home/gridsan/je23693/CoLaCodon/
bindir=/home/gridsan/je23693/CoLaCodon/apps/h264
foreman=$bindir/foreman.cfg

dir=/home/gridsan/je23693/compression_benchmarks/tests/h264/cola/$testN/
git log -1 &>$dir/commit

# compile
cd $bindir
$cola/build/colac build -release h264.seq

##### ADD BENCHMARK HERE
mkdir -p $dir
rm -f $dir/foreman.log
touch $dir/foreman.log

cp $cola/apps/eval/scripts/cola_h264.sh $dir # copy params

##### ADD BENCHMARK HERE

# Foremane
cmd="$bindir/h264 $foreman"
echo $cmd
counter=1
while [ $counter -le 15 ]
do
    echo $counter
    echo "=======" &>>$dir/foreman.log
    echo $counter  &>>$dir/foreman.log
    $cmd &>> $dir/foreman.log
    ((counter++))
done

cp /tmp/cola.264 $dir
ffmpeg -y -i $dir/cola.264 -vcodec libx264 $dir/cola.mp4
