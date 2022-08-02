#!/bin/bash
set -e

testN=$1
cola=/Users/jray/Documents/PhD/CoLaCodon/
export DYLD_LIBRARY_PATH=$cola/build
bindir=$cola/apps/h264
foreman=$bindir/foreman.cfg

dir=$cola/benchmarks/h261/cola/$testN/
mkdir -p $dir
git log -1 &>$dir/commit

# compile
cd $bindir
$cola/build/colac build -release h264.seq

##### ADD BENCHMARK HERE
rm -f $dir/foreman.log
touch $dir/foreman.log

cp $cola/apps/eval/scripts/laptop/cola_h264.sh $dir # copy params

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
ffmpeg -y -i $dir/cola.264 -vcodec libx264 $dir/foreman.mp4
