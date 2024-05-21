cp -r ../module2 ../../ || exit
cd ../../module2 || exit
make clean || exit
make all || exit
cp counter.ko ../initramfs/lib/modules/6.7.4/ || exit
cd ../initramfs || exit
find . | cpio -ov --format=newc | gzip -9 > ../try/initramfs.gz || exit
cd ..
# cleanup
rm -r module2
rm initramfs/lib/modules/6.7.4/counter.ko
