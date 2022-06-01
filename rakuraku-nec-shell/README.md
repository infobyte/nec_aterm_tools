# rakuraku-NEC-shell

## Dependencies

Since we need to execute MIPS binaries in order to generate the password, we need to install a few dependencies. First and foremost, we need `qemu-user-static` in order to execute `pwcrypt`, and we need to `chroot` into a fake filesystem so that `pwcrypt`'s dependencies are found (the reason we need qemu to be static is precisely because we will be working inside a chrooted environment). On top of that, we need a few python packages to be able to read the QR code (we are not actually reading the QR code ourselves, but rather using python to upload the image to an online QR-decoding service).

To install these dependencies and prepare the environment just run:

```
./setup.sh
pip3 install -r requirements.txt
```

> You could also install the python dependencies in a virtual environment, but since we are already installing system-wide packages we thought it was unnecessary to take that extra step.

## Running

Running the script is simple, just do:

```
./gen_password.sh [your qr image]
```

and the password will be printed via standard out. We provide a sample qr code obtained from the internet in order to test that everything is working properly. You may test the setup using this code by executing: `./gen_password.sh ./sample_qr.png`.

## How does it work?

`gen_password.sh` will invoke a python script that uploads the QR photo to a webserver in order to be decoded (*please note that this is a third-party service and is not associated with Faraday in any way*, we are just using it because it was more convenient than parsing the QR code locally). This python script will extract the MAC address, the device name and the value of `HW_RANDOM_KEY` from the decoded URL.
The aforementioned bash script will then parse these values into temporary environment variables and use them to set up a chroot environment capable of executing `pwcrypt`. Finally, it will chroot into it and execute `pwcrypt`, which will output the correct password for the device associated with the QR code provided by the user.

