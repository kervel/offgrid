#!/bin/sh


while sleep 15
	do 
		if /usr/bin/lsusb | grep  1c9e:f000; then
			/usr/sbin/usb_modeswitch -c /etc/usb_modeswitch.conf
			sleep 3
		fi
		/usr/bin/wvdial 3gconnect
	done

