from time import sleep
import os
from subprocess import Popen, PIPE, DEVNULL

import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)

import board
import busio
from digitalio import DigitalInOut

from adafruit_pn532.i2c import PN532_I2C

# I2C connection:
i2c = busio.I2C(board.SCL, board.SDA)

# Non-hardware reset/request with I2C
pn532 = PN532_I2C(i2c, debug=False)

ic, ver, rev, support = pn532.firmware_version
print("Found PN532 with firmware version: {0}.{1}".format(ver, rev))

# this delay avoids some problems after wakeup
sleep(0.5)

# Configure PN532 to communicate with MiFare cards
pn532.SAM_configuration()

non_poisoned_cards = ['SB', 'DT', 'ZK', 'TB']
poisoned_cards = ['VM']
read_block = 4
UV_light_pin = 4

vid_command = 'omxplayer {0} --loop --no-osd --nodeinterlace --fps 60 &'
pic_command = 'sudo fbi -a -T 1 --noverbose {0}.jpg &'

GPIO.setup(UV_light_pin, GPIO.OUT)
GPIO.output(UV_light_pin, GPIO.HIGH) 

def wait_remove_card(uid):
    while uid:
        print('Same Card Still there')
        sleep(0.5)
        try:
            uid = pn532.read_passive_target(timeout=0.5)
        except RuntimeError:
            uid = None


def scan_field():
    count = 0
    while True:
        try:
            uid = pn532.read_passive_target(timeout=0.5)
        except RuntimeError:
            uid = None
            sleep(0.5)

        print('.', end="") if count <= 3 else print("", end="\r")
        # Try again if no card is available.
        if uid is None:
            count = count + 1 if count < 6 else 0
        else:
            print('Found card with UID:', [hex(i) for i in uid])
            break

    return uid


def main():

    print('Welcome to Poison Scanner')
    # clean start
    # Kill all relavent applications
    os.system("sudo pkill fbi")
    os.system(pic_command.format("default"))

    print('Waiting Card')

    while True:
        uid = scan_field()

        if uid:
            try:
                data = pn532.ntag2xx_read_block(read_block)
                print('Card found')
            except Exception:
                data = b"XX"

            read_data = data.decode('utf-8')[:2]
            print('data is: {}'.format(read_data))

            if read_data in poisoned_cards:
                GPIO.output(UV_light_pin, GPIO.LOW) 
                print('Poisoned card')
                os.system(pic_command.format(read_data))
            elif read_data in non_poisoned_cards:
                GPIO.output(UV_light_pin, GPIO.LOW) 
                print('Clean Card')
                os.system(pic_command.format(read_data))
            else:
                print('Wrong Card')
                os.system(pic_command.format("unknown"))

            wait_remove_card(uid)
            print("Card Removed")
            os.system("sudo pkill fbi")
            GPIO.output(UV_light_pin, GPIO.HIGH) 
            os.system(pic_command.format("default"))


if __name__ == "__main__":
    main()

