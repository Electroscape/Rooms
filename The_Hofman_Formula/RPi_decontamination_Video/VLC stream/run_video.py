# sudo pip3 install python-vlc
# pip3 install -r requirements.txt

# from moviepy.editor import VideoFileClip
# from pymediainfo import MediaInfo

import subprocess


video_process = subprocess.Popen(['/usr/bin/cvlc', 'rtsp://TeamEscape:****@192.168.**.**:88/videoMain',
                                  "--no-embedded-video", "--fullscreen", '--no-video-title'],
                                 shell=False)

# @reboot sleep 10 && DISPLAY=:0 python3 /home/pi/run_video.py
