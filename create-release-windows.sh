#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 version"
    exit
fi
ver=$1

[ -f ffmpeg ] || cp $(which ffmpeg) . || (echo Please install ffmpeg before running this script. && exit)

gcc audiointensity.c -o audiointensity.exe
7z a audiointensity-windows-$ver.zip audiointensity.bat ffmpeg.exe audiointensity.exe "Link to VideoLAN.org - Download VLC Player here.url"
