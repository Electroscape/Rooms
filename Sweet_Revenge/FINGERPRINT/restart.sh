sudo pkill python

export DISPLAY=:0
xhost +
xinput map-to-output "Logitech USB Optical Mouse" "HDMI-2"

bash ~/FINGERPRINT/start.sh
