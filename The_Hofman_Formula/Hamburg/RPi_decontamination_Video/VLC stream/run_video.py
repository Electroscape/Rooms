# sudo pip3 install python-vlc
# pip3 install -r requirements.txt

# from moviepy.editor import VideoFileClip
# from pymediainfo import MediaInfo

import subprocess


video_process = subprocess.Popen(['/usr/bin/cvlc', 'rtsp://TeamEscape:EscapeS2016@192.168.87.22:88/videoMain',
                                  "--no-embedded-video", "--fullscreen", '--no-video-title'],
                                 shell=False)

# @reboot sleep 10 && DISPLAY=:0 python3 /home/pi/run_video.py

