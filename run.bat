dmesg -c
make
insmod chardevice.ko
insmod miModulo.ko
echo "hola" > /dev/chardev
dmesg
cat /dev/chardev
