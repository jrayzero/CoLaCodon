#!/bin/bash
set -e

testN=$1
cola=/home/gridsan/je23693/CoLaCodon/
bindir=/home/gridsan/je23693/compression_benchmarks/jpeg-9e

dir=/home/gridsan/je23693/compression_benchmarks/tests/jpeg/jpeg-9e/$testN/

cd $bindir

##### ADD BENCHMARK HERE
mkdir -p $dir
rm -f $dir/flower.log
rm -f $dir/cathedral.log
rm -f $dir/spider.log
touch $dir/flower.log
touch $dir/cathedral.log
touch $dir/spider.log

cp $cola/apps/eval/scripts/jpeg9e.sh $dir # copy params

data=/home/gridsan/je23693/compression_benchmarks/data/rgb/

##### ADD BENCHMARK HERE

# FLOWER
cmd="$bindir/cjpeg -baseline -sample 1x1,1x1,1x1 -outfile /tmp/jpeg9e.jpg $data/flower_foveon.ppm"
echo $cmd
counter=1
while [ $counter -le 15 ]
do
    echo $counter
    echo "=======" &>>$dir/flower.log
    echo $counter  &>>$dir/flower.log
    $cmd &>> $dir/flower.log
    ((counter++))
done

# decode it and print the psnr
cp /tmp/jpeg9e.jpg $dir/flower.jpg
dbindir=/home/gridsan/je23693/compression_benchmarks/jpeg-9e/
$dbindir/djpeg -outfile $dir/flower.decoded.ppm $dir/flower.jpg 
$cola/build/colac run $cola/apps/common/compute_psnr.seq $dir/flower.decoded.ppm /home/gridsan/je23693/compression_benchmarks/data/rgb/flower_foveon.ppm &>>$dir/flower.log

# spider 
cmd="$bindir/cjpeg -baseline -sample 1x1,1x1,1x1 -outfile /tmp/jpeg9e.jpg $data/spider_web.ppm"
echo $cmd
counter=1
while [ $counter -le 15 ]
do
    echo $counter
    echo "=======" &>>$dir/spider.log
    echo $counter  &>>$dir/spider.log
    $cmd &>> $dir/spider.log
    ((counter++))
done

# decode it and print the psnr
cp /tmp/jpeg9e.jpg $dir/spider.jpg
dbindir=/home/gridsan/je23693/compression_benchmarks/jpeg-9e/
$dbindir/djpeg -outfile $dir/spider.decoded.ppm $dir/spider.jpg 
$cola/build/colac run $cola/apps/common/compute_psnr.seq $dir/spider.decoded.ppm /home/gridsan/je23693/compression_benchmarks/data/rgb/spider_web.ppm &>>$dir/spider.log

# cathedral
cmd="$bindir/cjpeg -baseline -sample 1x1,1x1,1x1 -outfile /tmp/jpeg9e.jpg $data/cathedral.ppm"
echo $cmd
counter=1
while [ $counter -le 15 ]
do
    echo $counter
    echo "=======" &>>$dir/cathedral.log
    echo $counter  &>>$dir/cathedral.log
    $cmd &>> $dir/cathedral.log
    ((counter++))
done

# decode it and print the psnr
cp /tmp/jpeg9e.jpg $dir/cathedral.jpg
dbindir=/home/gridsan/je23693/compression_benchmarks/jpeg-9e/
$dbindir/djpeg -outfile $dir/cathedral.decoded.ppm $dir/cathedral.jpg 
$cola/build/colac run $cola/apps/common/compute_psnr.seq $dir/cathedral.decoded.ppm /home/gridsan/je23693/compression_benchmarks/data/rgb/cathedral.ppm &>>$dir/cathedral.log
