#!/bin/bash
set -e

testN=$1
iters=5
cola=/Users/jray/Documents/PhD/CoLaCodon/
export DYLD_LIBRARY_PATH=$cola/build
bindir=$cola/apps/jpeg/baseline
spider=$bindir/jpeg_spider.laptop.cfg
flower=$bindir/jpeg_flower.laptop.cfg
cathedral=$bindir/jpeg_cathedral.laptop.cfg

dir=$cola/benchmarks/jpeg/cola/$testN/
mkdir -p $dir
git log -1 &>$dir/commit

# compile
cd $bindir
$cola/build/colac build -release jpeg.seq

##### ADD BENCHMARK HERE
rm -f $dir/flower.log
rm -f $dir/cathedral.log
rm -f $dir/spider.log
touch $dir/flower.log
touch $dir/cathedral.log
touch $dir/spider.log

cp $cola/apps/eval/scripts/laptop/cola_jpeg_baseline.sh $dir # copy params

##### ADD BENCHMARK HERE

# FLOWER
cmd="$bindir/jpeg $flower"
echo $cmd
counter=1
while [ $counter -le $iters ]
do
    echo $counter
    echo "=======" >> $dir/flower.log
    echo $counter  >> $dir/flower.log
    $cmd >> $dir/flower.log
    ((counter++))
done

# decode it and print the psnr
cp /tmp/cola.jpg $dir/flower.jpg
md5 $dir/flower.jpg >> $dir/flower.log
echo "Flower md5"
echo md5 $dir/flower.jpg

# spider 
cmd="$bindir/jpeg $spider"
echo $cmd
counter=1
while [ $counter -le $iters ]
do
    echo $counter
    echo "=======" >>$dir/spider.log
    echo $counter  >>$dir/spider.log
    $cmd >> $dir/spider.log
    ((counter++))
done

# decode it and print the psnr
cp /tmp/cola.jpg $dir/spider.jpg
md5 $dir/spider.jpg >> $dir/spider.log
echo "Spider md5"
echo md5 $dir/spider.jpg

# cathedral 
cmd="$bindir/jpeg $cathedral"
echo $cmd
counter=1
while [ $counter -le $iters ]
do
    echo $counter
    echo "=======" >>$dir/cathedral.log
    echo $counter  >>$dir/cathedral.log
    $cmd >> $dir/cathedral.log
    ((counter++))
done

# decode it and print the psnr
cp /tmp/cola.jpg $dir/cathedral.jpg
md5 $dir/cathedral.jpg >> $dir/cathedral.log
echo "Cathedral md5"
echo md5 $dir/cathedral.jpg
