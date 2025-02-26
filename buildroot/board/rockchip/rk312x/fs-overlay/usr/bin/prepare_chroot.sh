#!/bin/sh

echo "mkdir chroot ..."
mkdir -p /tmp/chroot_bte/bin
mkdir -p /tmp/chroot_bte/lib32
mkdir -p /tmp/chroot_bte/proc
mkdir -p /tmp/chroot_bte/tmp
mkdir -p /tmp/chroot_bte/root
mkdir -p /tmp/chroot_bte/sys
mkdir -p /tmp/chroot_bte/dev
mkdir -p /tmp/chroot_bte/usr/lib
mkdir -p /tmp/chroot_bte/usr/bin
mkdir -p /tmp/chroot_bte/usr/sbin
mkdir -p /tmp/chroot_bte/usr/share
mkdir -p /tmp/chroot_bte/sbin
mkdir -p /tmp/chroot_bte/lib
mkdir -p /tmp/chroot_bte/etc
mkdir -p /tmp/chroot_bte/mnt
mkdir -p /tmp/chroot_bte/etc/xdg/weston/
mkdir -p /tmp/chroot_bte/var/run/
mkdir -p /tmp/chroot_bte/usr/libexec/
mkdir -p /tmp/chroot_bte/usr/lib/libweston-8/
mkdir -p /tmp/chroot_bte/usr/lib/weston/
mkdir -p /tmp/chroot_bte/etc/fonts/

echo "mount --bind stage"
mount --bind /proc/ /tmp/chroot_bte/proc/
mount --bind /sys/ /tmp/chroot_bte/sys/
mount --bind /dev/ /tmp/chroot_bte/dev/

echo "cp -raf required file stage"
echo "copy openssl lib"
cp -raf /usr/lib/libssl.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libcrypto.so* /tmp/chroot_bte/usr/lib/
cp -raf /lib/libdl* /tmp/chroot_bte/lib/
cp -raf /lib/libpthread* /tmp/chroot_bte/lib/
cp -raf /lib/libc-2.29.so /tmp/chroot_bte/lib/
cp -raf /lib/libc.so.6 /tmp/chroot_bte/lib/
cp -raf /lib/ld-2.29.so /tmp/chroot_bte/lib/
cp -raf /lib/ld-linux-armhf.so.3 /tmp/chroot_bte/lib/
cp -raf /lib/libatomic.so* /tmp/chroot_bte/lib/

echo "copy ota lib"
cp -raf /usr/lib/libSDL2-2.0.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libSDL2_image-2.0.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libSDL2_ttf-2.0.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libSDL2_mixer-2.0.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libasound.so* /tmp/chroot_bte/usr/lib/
cp -raf /lib/libm.so.6 /tmp/chroot_bte/lib/
cp -raf /lib/libgcc_s.so* /tmp/chroot_bte/lib/
cp -raf /lib/librt.so* /tmp/chroot_bte/lib/
cp -raf /usr/lib/libfreetype.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libpng16.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libz.so* /tmp/chroot_bte/usr/lib/

echo "copy binary file"
cp -raf /usr/bin/openssl /tmp/chroot_bte/usr/bin/
cp -raf /usr/bin/unzip /tmp/chroot_bte/usr/bin/
cp -raf /usr/sbin/chroot /tmp/chroot_bte/usr/sbin/
cp -raf /usr/bin/weston_daemon.sh /tmp/chroot_bte/usr/bin/
cp -raf /usr/bin/weston* /tmp/chroot_bte/usr/bin/
cp -raf /bin/* /tmp/chroot_bte/bin/
cp -raf /etc/fonts/fonts.conf /tmp/chroot_bte/etc/fonts/
cp -raf /etc/model /tmp/chroot_bte/etc/
cp -raf /usr/bin/cksum /tmp/chroot_bte/usr/bin/
cp -raf /usr/bin/awk /tmp/chroot_bte/usr/bin/
cp -raf /usr/bin/xargs /tmp/chroot_bte/usr/bin/
cp -raf /root/* /tmp/chroot_bte/root/
cp -raf /usr/bin/md5sum /tmp/chroot_bte/usr/bin/
cp -raf /etc/fonts/fonts.conf /tmp/chroot_bte/etc/fonts/
cp -raf /usr/bin/printf /tmp/chroot_bte/usr/bin/

echo "copy weston lib"
cp -raf /usr/lib/weston/libexec_weston.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libweston-8.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libwayland-client.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libwayland-server.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libinput.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libevdev.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libpixman-1.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libxkbcommon.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libmali.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libffi.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libmtdev.so* /tmp/chroot_bte/usr/lib/
cp -raf /lib/libudev.so* /tmp/chroot_bte/lib/
cp -raf /usr/lib/librga.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libdrm.so* /tmp/chroot_bte/lib/
cp -raf /usr/lib/libstdc++.so* /tmp/chroot_bte/lib/
cp -raf /lib/librt.so* /tmp/chroot_bte/lib/
cp -raf /lib/librt-2.29.so /tmp/chroot_bte/lib/
cp -raf /lib/libm-2.29.so /tmp/chroot_bte/lib/
cp -raf /usr/lib/libmali-utgard-400-r7p0-r1p1-wayland.so /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libweston-8/* /tmp/chroot_bte/usr/lib/libweston-8/
cp -raf /usr/lib/weston/* /tmp/chroot_bte/usr/lib/weston/
cp -raf /lib/libuuid.so* /tmp/chroot_bte/lib/

echo "copy weston-desktop-shell & weston-keyboard lib"
cp -raf /usr/lib/libcairo.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libpixman-1.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libjpeg.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libxkbcommon.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libwayland-cursor.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libffi.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/librga.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libfontconfig.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libexpat.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libglapi.so* /tmp/chroot_bte/usr/lib/
cp -raf /lib/libgcc_s.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libweston-desktop-8.so* /tmp/chroot_bte/usr/lib
cp -raf /usr/lib/libwayland-egl.so* /tmp/chroot_bte/usr/

echo "copy egl lib"
cp -raf /usr/lib/libEGL.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libGLESv1_CM.so* /tmp/chroot_bte/usr/lib/
cp -raf /usr/lib/libGLESv2.so* /tmp/chroot_bte/usr/lib/

# for ota library
cp -raf /etc/model /tmp/chroot_bte/etc/
cp -raf /root/* /tmp/chroot_bte/root/
# for weston
cp -raf /etc/xdg/weston/* /tmp/chroot_bte/etc/xdg/weston/
cp -raf /usr/share/* /tmp/chroot_bte/usr/share/
# for weston keyboard & desktop-shell
cp -raf /usr/libexec/* /tmp/chroot_bte/usr/libexec/
cp -raf /usr/lib/* /tmp/chroot_bte/usr/lib/

mv /tmp/ota.zip /tmp/chroot_bte/tmp/

echo "create chroot script..."
cat > /tmp/chroot_bte/usr/bin/fw_chroot.sh << EOF
#!/bin/sh

export FONTCONFIG_PATH=/etc/fonts
# run weston
/usr/bin/weston_daemon.sh
sleep 1;

unzip /tmp/ota.zip ota.enc
unzip -p /tmp/ota.zip ota.key.enc | openssl rsautl -decrypt -inkey /root/ota_key.priv | openssl enc -d -aes-256-cbc -pbkdf2 -in ota.enc -out /tmp/ota -pass stdin
if [ $? -eq 0 ]
then
        echo "Got ota pass"
        chmod +x /tmp/ota
        ./tmp/ota
else
        echo "ota fail"
        chmod +x /tmp/ota
        ./tmp/ota fail
fi
EOF

chmod 775 /tmp/chroot_bte/usr/bin/fw_chroot.sh
echo "prepare chroot ending..."



