# Starte Videodatei
omxplayer /home/pi/Strukturformel_1610.mp4 --loop --no-osd --nodeinterlace --fps 60 &
# in case of of some format OMX wont do, so for VLC
# vlc /home/pi/Strukturformel_1610.mp4 --loop --fullscreen

# Starte Stream
# sudo omxplayer rtsp://TeamEscape:****@192.168.87.27:88/videoMain -b -o hdmi -r &
python3 /home/pi/STB-1/HF_Videotrigger/listenToGPIO.py

# Display Riddle Photo
#sudo fbi --autozoom --noverbose --vt 1 riddle_new.jpg
