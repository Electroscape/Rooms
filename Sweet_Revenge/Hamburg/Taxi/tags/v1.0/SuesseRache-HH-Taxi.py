import time
import datetime
import os
import subprocess
import RPi.GPIO as GPIO
from omxplayer import OMXPlayer

SENSOR_PIN = 23
RELAY_PIN = 22

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(SENSOR_PIN, GPIO.IN)
GPIO.setup(RELAY_PIN, GPIO.OUT)
GPIO.output(RELAY_PIN, GPIO.HIGH)

logfile = open('/home/pi/log/videolog.csv','a')
date_stamp = datetime.datetime.fromtimestamp(time_stamp).strftime('%Y-%m-%d %H:%M:%S')
# Logging: Beginne Video laden

taxiVideo = OMXPlayer('/home/pi/Galerie-HH-Taxi/Galerie-HH-Taxi.mp4', ['--no-osd'], '-b')
taxiVideo.pause()
time.sleep(5)
# Logging: Ende Video laden

while True:
    i=GPIO.input(SENSOR_PIN)
    if i==0:
        time.sleep(2)
        print("Keine Bewegung")

    elif i==1:
        print("Bewegung erkannt")
        # Logging: Bewegung erkannt, Video wird abgespielt
        taxiVideo.play()
        time.sleep(25)
        GPIO.output(RELAY_PIN, GPIO.LOW)
        # Logging: Tuer wurde geöffnet
        taxiVideo.pause()
        # Logging: Video Ende
        time.sleep(3600)
