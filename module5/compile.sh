cp -r ../module5 ../../ || exit
cd ../../module5 || exit
make clean || exit
make all || exit
cp fifo_module.ko ../initramfs/lib/modules/6.7.4/ || exit
cd ../initramfs || exit
find . | cpio -ov --format=newc | gzip -9 > ../try/initramfs.gz || exit
cd ..
# cleanup
rm -r module5
rm initramfs/lib/modules/6.7.4/fifo_module.ko
