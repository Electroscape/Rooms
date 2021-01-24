from time import sleep
import os
from subprocess import Popen, PIPE, DEVNULL

from py532lib.mifare import *
from py532lib.frame import *
from py532lib.constants import *

card = Mifare()
card.SAMconfigure()
card.set_max_retries(MIFARE_SAFE_RETRIES)

pic_path = '/home/pi/virusFoto1.jpg'
vid_path = '/home/pi/Periodensystem_169_schwarz.mp4' 
vid_command = 'omxplayer {} --loop --no-osd --nodeinterlace --fps 60 &'.format(vid_path)
pic_command = 'sudo fbi -T 1 --noverbose {} &'.format(pic_path)

def noram_scenario():
    card = Mifare()
    card.SAMconfigure()
    card.set_max_retries(MIFARE_SAFE_RETRIES)
    uid = card.scan_field()
    if uid:
        address = card.mifare_address(0,1)
        card.mifare_auth_a(address,MIFARE_FACTORY_KEY)
        data = card.mifare_read(address)


def main():

    print('Welcome to Microscope')

    os.system(vid_command)
    print('Waiting Card')
    while True:
        uid = card.scan_field()

        if uid:
            print('Card found')
            address = card.mifare_address(0,1)
            card.mifare_auth_a(address,MIFARE_FACTORY_KEY)
            data = card.mifare_read(address)
            print('data is: {}'.format(data.decode('utf-8')[:2]))

            if data.decode('utf-8')[:2] == 'VR':
                print('Stop video and Diplay pic')
                os.system("sudo killall -9 omxplayer.bin");
                #sleep(1)
                os.system(pic_command)

                while uid:
                    print('Card still here')
                    sleep(1)
                    uid = card.scan_field()

                # Card removed    
                print('No image back to video')
                os.system("sudo kill $(pgrep fbi)")
                os.system(vid_command)
            else:
            	print('Wrong Card')
            	while uid:
                    print('Remove Wrong Card')
                    sleep(1)
                    uid = card.scan_field()


if __name__ == "__main__":
    main()

