## Linear DM module with sysfs read/write stats

Compile
```
make
```
Install the module
```
insmod dm-linear-stats.ko
```

Check if installed
```
lsmod | grep dm-linear
```

Create underlying test device
```
dd if=/dev/zero of=bd bs=512 count=10 
losetup --show -f bd 
ls -al /dev/loop0
```

Create our proxy device
```
dmsetup create proxy-device --table "0 $(blockdev --getsz /dev/loop0) linear-stats /dev/loop0"
```

Or use the provided script run.sh 
```
sudo ./run.sh /dev/loop0
```

Check if created
```
dmsetup ls
```

Write to our proxy device
```
echo Hello World > hello
dd if=hello of=/dev/mapper/proxy-device bs=512 count=1
```

Read from our proxy device
```
dd if=/dev/mapper/proxy-device of=proxy-out bs=512 count=1
```

Check contents of underlying device
```
cat bd
```

Remove the proxy device from device mapper
```
dmsetup remove proxy-device
dmsetup ls
```

Unload the module
```
rmmod dm_linear_stats
```

Remove the loop mapping
```
losetup -d /dev/loop0
```

Check the read/write stats
```
cat /sys/module/dm_linear_stats/stat/stats
```

To reload the module and recreate our device
```
sudo ./run.sh /dev/loop0
```
