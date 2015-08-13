#!/bin/bash
SCRIPTPATH="dirname $0"

VLC=$(which vlc)
VLC="/C/Program Files (x86)/VideoLAN/VLC/vlc.exe"
if [ ! -e "$VLC" ]; then
	VLC="/usr/bin/vlc"
	if [ ! -e "$VLC" ]; then
		VLC="/usr/local/bin/vlc"
		if [ ! -e "$VLC" ]; then
			echo "Unable to locate VLC. Install it from http://www.videolan.org"
			exit
		fi
	fi
fi

FFMPEG=$(which ffmpeg)
if [ ! -e "$FFMPEG" ]; then
	FFMPEG="/usr/local/bin/ffmpeg"
	if [ ! -e "$FFMPEG" ]; then
		FFMPEG="/usr/bin/ffmpeg"
		if [ ! -e "$FFMPEG" ]; then
			echo "Unable to locate FFMPEG. Install it from http://ffmpeg.org/download.html"
			exit
		fi
	fi
fi 

args=
for f in "$@"; do
	ts=$("$FFMPEG" -i "$f" -vn -acodec pcm_s16le -ar 8000 -ac 1 -f wav -y - 2>/dev/null | "$SCRIPTPATH/audiointensity")
	for t in $ts; do
		#echo time:"$t"  
		args="$args ""$f"" :start-time=$t :run-time=30 "
	done
done
if [ ! -z "$args" ]; then
	"$VLC" $args	
else 
	echo "Usage: $0 /path/to/first_video [/optional/path/to/next_video [...]]"
fi
