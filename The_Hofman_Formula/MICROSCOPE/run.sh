# Starte Videodatei
#omxplayer /home/pi/Periodensystem_169_schwarz.mp4 --loop --no-osd --nodeinterlace --fps 60

#Microscope
export DISPLAY=:0
pkill python
cd ~/Microscope
# for smooth transition instead of terminal appearance
sudo fbi -a -T 1 --noverbose blackscreen.jpg &
python microscope.py

# Starte Stream
#omxplayer rtsp://<username>:<password>@192.168.XXX.XXX:88/videoMain
