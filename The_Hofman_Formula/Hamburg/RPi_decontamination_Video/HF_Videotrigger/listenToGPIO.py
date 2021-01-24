from time import sleep
import RPi.GPIO as GPIO
import run_video_once
import sshTrigger
import subprocess
import os

GPIO.setmode(GPIO.BOARD)
GPIO.setup(7, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

print("listening to gpios to trigger HF video")

def main():

    while True:
        hysteresis = 0
        print (GPIO.input(7))
        while (GPIO.input(7)):
            hysteresis += 1
            if hysteresis > 5:
                print("successfully detected")
                print("sshTrigger")
                # subprocess.Popen('sshTrigger.sh', shell=False, cwd=os.getcwd())
                sshTrigger.trigger_script()
                sleep(0.5)
                
                os.system("killall \"omxplayer.bin\"")
                print("calling own video")
                sleep(0.5)
                run_video_once.run_video()
                subprocess.Popen(['omxplayer', '-b', '--loop', '--no-osd', '--nodeinterlace','/home/pi/Strukturformel_1610.mp4'])
                sleep(5)

        sleep(0.1)

main()
