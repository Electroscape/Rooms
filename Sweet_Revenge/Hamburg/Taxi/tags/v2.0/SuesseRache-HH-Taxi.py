# coding=utf-8

import time
import datetime
import os
import subprocess
import RPi.GPIO as GPIO
from subprocess import Popen

SENSOR_PIN = 23
RELAY_PIN = 22

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(SENSOR_PIN, GPIO.IN)
GPIO.setup(RELAY_PIN, GPIO.OUT)
GPIO.output(RELAY_PIN, GPIO.HIGH)

os.system("sudo fbi -T 1 -noverbose -a /home/pi/SuesseRache-HH-Taxi/Videos/TaxiLogo.png")
time.sleep(15)
i= 0

while True:
    i=GPIO.input(SENSOR_PIN)
    if i==0:
        time.sleep(2)
        print("Keine Bewegung")

    elif i==1:
        print("Bewegung erkannt")
        os.system("sudo killall -9 fbi")
        Popen(['omxplayer', '-b', "/home/pi/SuesseRache-HH-Taxi/Videos/TE_SR_Taxi.mp4"])
        time.sleep(25)
        GPIO.output(RELAY_PIN, GPIO.LOW)
        # Logging: Tuer wurde geöffnet
        print("Door Open")
        os.system("sudo fbi -T 1 -noverbose -a /home/pi/SuesseRache-HH-Taxi/Videos/ArrowLogo.png")
        break
