sudo rmmod usbhid
sudo rmmod uas 
sudo rmmod usb_storage
sudo rmmod btrtl

sudo insmod 1/usb_stat_km.ko

sudo modprobe btrtl
sudo modprobe usb_storage
sudo modprobe uas
sudo modprobe usbhid
