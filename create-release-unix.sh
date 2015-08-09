#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 version-name"
    exit
fi
ver=$1

[ -f ffmpeg ] || cp $(which ffmpeg) . || (echo Please install ffmpeg before running this script. && exit)

gcc audiointensity.c -o audiointensity
7z a audiointensity-debian-$ver.zip audiointensity.sh ffmpeg audiointensity "Please install VLC if you do not already have it.txt"
