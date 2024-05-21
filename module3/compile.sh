cp -r ../module3 ../../ || exit
cd ../../module3 || exit
make clean || exit
make all || exit
cp proc_mmaneg.ko ../initramfs/lib/modules/6.7.4/ || exit
cd ../initramfs || exit
find . | cpio -ov --format=newc | gzip -9 > ../try/initramfs.gz || exit
cd ..
# cleanup
rm -r module3
rm initramfs/lib/modules/6.7.4/proc_mmaneg.ko
