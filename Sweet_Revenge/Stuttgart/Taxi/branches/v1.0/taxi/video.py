import RPi.GPIO as GPIO
import time
import os
import subprocess
from omxplayer import OMXPlayer

SENSOR_PIN = 23
RELAY_PIN = 22

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(SENSOR_PIN, GPIO.IN)
GPIO.setup(RELAY_PIN, GPIO.OUT)
GPIO.output(RELAY_PIN, GPIO.HIGH)

taxi_start = OMXPlayer('/home/pi/taxi/taxi_start.mp4', ['--no-osd'])
time.sleep(13)
taxi_start.pause()

while True:
    i=GPIO.input(SENSOR_PIN)
    if i==1:
        time.sleep(2)
        print("Tuer geschlossen")

    elif i==0:
        print("Tuer offen")
        while True:
            z=GPIO.input(SENSOR_PIN)
            if z==0:
                print("Tuer noch offen")
                time.sleep(2)
            elif z==1:
                print("Tuer wieder geschlossen")
                taxi_start.play()
                time.sleep(2)
                taxi_start.pause()
                video = OMXPlayer('/home/pi/taxi/SuesseRache_S_Taxifahrt.mp4', ['--no-osd'])
                time.sleep(32)
                GPIO.output(RELAY_PIN, GPIO.LOW)
                video.quit()
                arrow = OMXPlayer('/home/pi/taxi/arrow.mp4', ['--no-osd'])
                time.sleep(5)
                arrow.pause()
                time.sleep(5400)
