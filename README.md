# audiointensity - A Porn Movie Orgasm Detector

I spent a few hours writing this program that analyses the sound from a movie, finds the points of highest sound intensity (supposedly during climax) and automatically plays the video from those points. It uses some simple hand-written digital signal filters and peak finding algorithms; no external libraries.

# What It Does
When running it, it will take some time (seconds to minutes depending on your computer and the size of the video) before anything happens since FFmpeg has to convert the movie audio to WAV before sending it to the program. VLC will then start. It will jump to a time a little before the highest audio intensity (orgasm?), play for 30 seconds and then switch to the time of next-highest audio intensity and so on, up to 5 different points. The program works as expected if you give it several movies, processing and showing them in-order.

