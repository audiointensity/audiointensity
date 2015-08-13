@echo off
setlocal enabledelayedexpansion
where vlc 2> NUL
if errorlevel 1 (
	SET VLC=C:\Progra~2\VideoLAN\VLC\vlc.exe
 	if not exist C:\Progra~2\VideoLAN\VLC\vlc.exe (
 		SET VLC=C:\Progra~1\VideoLAN\VLC\vlc.exe
 		if not exist C:\Progra~1\VideoLAN\VLC\vlc.exe (
 			echo Unable to locate VLC. Install it from http://www.videolan.org
 		)
	)
) else (
	SET VLC=vlc
)

where ffmpeg 2> NUL 1>NUL
if errorlevel 1 (
	SET FFMPEG="%~dp0\ffmpeg.exe"
 	if not exist "%~dp0\ffmpeg.exe" (
		SET FFMPEG=C:\Progra~2\FFmpeg\bin\ffmpeg.exe
		if not exist C:\Progra~2\FFmpeg\bin\ffmpeg.exe (
			SET FFMPEG=C:\Progra~1\FFmpeg\bin\ffmpeg.exe
			if not exist C:\Progra~1\FFmpeg\bin\ffmpeg.exe (
				echo Unable to locate FFMPEG. Install it from http://ffmpeg.org/download.html
			)
		)
	)
) else (
	SET FFMPEG=ffmpeg
)

SET args=
set /a counter=0
for %%f in (%*) do (
	SET var=
	for /f "delims= " %%t in ('call "%FFMPEG%" -i %%f -vn -acodec pcm_s16le -ar 8000 -ac 1 -f wav -y - 2^>NUL ^| "%~dp0\audiointensity.exe"') do (
		SET args=!args! %%f :start-time=%%t :run-time=30 
 	)
)
"%VLC%" %args%
