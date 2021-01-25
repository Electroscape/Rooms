export DISPLAY=:0.0
sleep 5
xhost +
sleep 5
cd /home/pi/fingerabdruck
sudo python3 /home/pi/fingerabdruck/GUI_forensik.py
