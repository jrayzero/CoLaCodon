#!/bin/bash
set -e

fn=$1

ffmpeg -y -i $fn -c:v copy $fn.mp4
open $fn.mp4
