#!/bin/bash
set -e

testN=$1
outfn=$2
output_dir=$3
dir=$output_dir/h264/cola/$testN
mkdir -p $dir
touch $dir/out.log

echo "Compiling"
/home/gridsan/je23693/CoLaCodon/build/colac build -release /home/gridsan/je23693/CoLaCodon/apps/h264/h264.seq
echo "Done compiling"

counter=1
while [ $counter -le 15 ]
do
    echo $counter
    echo "=======" &>>$dir/out.log
    echo $counter  &>>$dir/out.log
    ./h264 /home/gridsan/je23693/CoLaCodon/apps/h264/h264_enc.cfg &>> $dir/out.log
    cp /home/gridsan/je23693/CoLaCodon/apps/h264/h264_enc.cfg $dir
    mv /home/gridsan/je23693/CoLaCodon/apps/h264/$outfn $dir
    ffmpeg -y -i $dir/$outfn -vcodec libx264 $dir/cola.mp4
    ((counter++))
done
