# Starte Videodatei
#omxplayer /home/pi/Strukturformel_169.mp4 --loop --no-osd --nodeinterlace --fps 60
# in case of of some format OMX wont do, so for VLC
# vlc /home/pi/hofman/180813_Strukturformel_Video16zu9.wmv --loop --fullscreen

# Starte Stream
sudo omxplayer rtsp://TeamEscape:****@192.168.**.**:88/videoMain -b -o hdmi -r &
python3 /home/pi/STB-1/HF_Videotrigger/listenToGPIO.py

# Display Riddle Photo
#sudo fbi --autozoom --noverbose --vt 1 riddle_new.jpg
