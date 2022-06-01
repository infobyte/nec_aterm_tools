#!/bin/bash
tmpfile=$(mktemp)
python3 ./parseqr.py $1 > $tmpfile
source "$tmpfile"
rm $tmpfile

echo '#!/bin/sh' > ./chroot/bin/flash
echo "echo HW_RANDOM_KEY=$HW_RANDOM_KEY" >> ./chroot/bin/flash
chmod +x ./chroot/bin/flash

sudo chroot chroot /qemu-mips-static /bin/pwcrypt $DEVICE_NAME $MAC

rm ./chroot/bin/flash

