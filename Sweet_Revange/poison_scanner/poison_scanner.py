from time import sleep
import os
from subprocess import Popen, PIPE, DEVNULL

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

vid_command = 'omxplayer {0} --loop --no-osd --nodeinterlace --fps 60 &'
pic_command = 'sudo fbi -a -T 1 --noverbose {0}.jpg &'

def noram_scenario():
    card = Mifare()
    card.SAMconfigure()
    card.set_max_retries(MIFARE_SAFE_RETRIES)
    uid = card.scan_field()
    if uid:
        address = card.mifare_address(0,1)
        card.mifare_auth_a(address,MIFARE_FACTORY_KEY)
        data = card.mifare_read(address)
        print('data is: {}'.format(data.decode('utf-8')[:2]))

def wait_remove_card(uid):
    while uid:
        print('Same Card Still there')
        sleep(1)
        try:
            uid = pn532.read_passive_target(timeout=0.5)
        except RuntimeError as e:
            uid = None
            
        

def scan_field():
    count = 0
    while True:
        try:
            uid = pn532.read_passive_target(timeout=0.5)
        except RuntimeError as e:
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

    print('Welcome to Finger Print Scanner')
    #clean start
    # Kill all relavent applications

    #Actual start
    os.system("sudo pkill fbi")
    os.system(pic_command.format("default"))

    print('Waiting Card')
    
    while True:
        uid = scan_field()

        if uid:
            try:
   	            data = pn532.ntag2xx_read_block(read_block)
   	            print('Card found')
            except IOError as e:
            	print(e)
            	data = b"XX"

            read_data = data.decode('utf-8')[:2]
            print('data is: {}'.format(read_data))

            if read_data in poisoned_cards:
                print('Poisoned card')
                os.system(pic_command.format(read_data))
            elif read_data in non_poisoned_cards:
                os.system(pic_command.format(read_data))
                print('Clean Card')
            else: 
            	print('Wrong Card')

            wait_remove_card(uid)
            print("Card Removed")
            os.system("sudo killall -15 fbi")
            os.system(pic_command.format("default"))



if __name__ == "__main__":
    main()
