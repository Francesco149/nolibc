This is a partial implementation of a DNS client that doesn't
depend on libc or any third party library. It builds to a rather
small < 4kb executable. The codebase is C89 and should build even
on old compilers.

The main purpose of this project is having libc-free code that I
can copypaste in projects that need to resolve domains.

Currently supported architectures are amd64 and i386.

It queries the A record for a given hostname, using a hardcoded
dns server (currently 185.121.177.177, one of opennic's servers).

No ipv6 support until I actually need it.

Note that this does not check for /etc/hosts or any config file at
all.

# Linux 32-bit
```
$ git clone https://github.com/Francesco149/namae.git
$ cd namae
$ chmod +x ./build.sh
$ ./build.sh i386
$ ./namae sdf.org
```

# Linux 64-bit
```
$ git clone https://github.com/Francesco149/namae.git
$ cd namae
$ chmod +x ./build.sh
$ ./build.sh
$ ./namae sdf.org
```

# License
This code is public domain and comes with no warranty. You are free
to do whatever you want with it.

You can contact me at
[lolisamurai@tfwno.gf](mailto:lolisamurai@tfwno.gf) but don't
expect any support.

I hope you will find the code useful or at least interesting to
read. Have fun!
