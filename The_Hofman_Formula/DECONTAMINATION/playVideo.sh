# Starte Videodatei
omxplayer /home/pi/Strukturformel_1610.mp4 --loop --no-osd --nodeinterlace --fps 60 &
# in case of of some format OMX wont do, so for VLC
# vlc /home/pi/Strukturformel_1610.mp4 --loop --fullscreen
python3 /home/pi/STB-1/HF_Videotrigger/listenToGPIO.py

