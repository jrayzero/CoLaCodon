#!/bin/bash
set -e

testN=$1
cola=/home/gridsan/je23693/CoLaCodon/
bindir=/home/gridsan/je23693/CoLaCodon/apps/h261
foreman=$bindir/foreman.cfg

dir=/home/gridsan/je23693/compression_benchmarks/tests/h261/cola/$testN/
mkdir -p $dir
git log -1 &>$dir/commit

# compile
cd $bindir
$cola/build/colac build -release h261.seq

##### ADD BENCHMARK HERE
rm -f $dir/foreman.log
touch $dir/foreman.log

cp $cola/apps/eval/scripts/llsc/cola_h261.sh $dir # copy params

##### ADD BENCHMARK HERE

# Foremane
cmd="$bindir/h261 $foreman"
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

cp /tmp/cola.261 $dir
ffmpeg -y -i $dir/cola.261 -vcodec libx264 $dir/foreman.mp4
