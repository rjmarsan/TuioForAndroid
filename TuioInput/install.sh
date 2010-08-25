adb -d shell "mount -o rw,remount -t yaffs2 /dev/block/mtdblock3 /system"
adb -d push TuioInput /system/bin/TuioInput
