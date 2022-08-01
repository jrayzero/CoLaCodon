#!/bin/bash
set -e

testN=$1

cmd="x264 -I 1 --profile=baseline --output x264.264 --frames 300 --level 2.0  --no-deblock  --qp 31 --trellis 0 --input-res 352x288 --verbose  --tune psnr --no-asm --threads 1 --psnr  /home/gridsan/je23693/compression_benchmarks/data/foreman.cif.yuv"

dir=/home/gridsan/je23693/compression_benchmarks/tests/h264/x264/$testN
mkdir -p $dir
rm -f $dir/out.log
touch $dir/out.log

counter=1
while [ $counter -le 15 ]
do
    echo $counter
    echo "=======" &>>$dir/out.log
    echo $counter  &>>$dir/out.log
    $cmd &>> $dir/out.log
    ((counter++))
done

cp x264.sh $dir # move this since it has the parameters
mv x264.264 $dir
ffmpeg -y -i $dir/x264.264 -vcodec libx264 $dir/x264.mp4
