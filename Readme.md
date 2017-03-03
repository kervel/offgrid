* follow the official sleepypi instructions
* if uploading to arduino fails, make sure to check if your getty on ttyS0 is not running anymore

check  sudo systemctl stop serial-getty@ttyAMA0.service --> should not be running

* reduce the baudrate of i2c (arduino is not fast enough and raspberry pi does not allow clock stretching)

add the following line to /boot/config.txt:
dtparam=i2c_baudrate=50000
   
