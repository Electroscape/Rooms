# sudo pip3 install python-vlc
# pip3 install -r requirements.txt

# from moviepy.editor import VideoFileClip
# from pymediainfo import MediaInfo

import subprocess
import os
from time import sleep

from run_video_cfg import Settings as cfg


script_dir = os.path.dirname(__file__)

filename = cfg["filename"]
file_duration = int(cfg["file_duration"])


# optional stuff that used to work but bugger RPis not having codecs
# not wasting half an hour on this when its not needed
'''
def get_video_length():
    print("checking filepath")
    print(os.path.exists(filename))
    print(filename)
    media_info = MediaInfo.parse(filename)
    # duration in milliseconds
    return media_info.tracks[0].duration
'''


def run_video():

    video_fullpath = os.path.join(script_dir, "event_files", filename)
    print(video_fullpath)
    
    if not os.path.exists(video_fullpath): 
        print("!Error invalid video path")

    #  --no-embedded-video
    print(video_fullpath)
    video_process = subprocess.Popen(['omxplayer', '-b', '/home/pi/STB-1/HF_Videotrigger/event_files/191030_V2_CountdownStimmeReinraum_fp.mp4'])

    sleep(file_duration)
    video_process.kill()
    sleep(0.5)


def main():
    run_video()


if __name__ == "__main__":
    # stuff only to run when not called via 'import' here
    main()


def __init__():
    print("run_video_once.py imported")
