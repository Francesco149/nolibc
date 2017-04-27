C89 json parser that doesn't depend on the standard C library.

It includes a rudimentary error system that shows part of the json
and points at the bad character.

This isn't meant to be used as a library or stand-alone tool.
It's reusable code that I copypaste into my projects and tweak as
needed so that I don't have to add an entire library to my codebase
. The example in this repo simply prints the generated tree
structure for reference and debugging.

tl;dr I'm using github to back-up my code snippets, but if you like
this you are free to use it for your projects.

This example project is linux only, but it should easily plug into
a windows project as long as you define a windows version of write
to let it output errors and debug info or simply remove error
output.

# Linux 32-bit
```
$ git clone https://github.com/Francesco149/jsonp.git
$ cd jsonp
$ chmod +x ./build.sh
$ ./build.sh i386
$ ./jsonp test.json
$ ./jsonp test2.json
```

# Linux 64-bit
```
$ git clone https://github.com/Francesco149/jsonp.git
$ cd jsonp
$ chmod +x ./build.sh
$ ./build.sh
$ ./jsonp test.json
$ ./jsonp test2.json
```

# Performance
I quickly tested 1million iterations on the two test json files
and compared them to 1million iterations of node's ```JSON.parse```
and it's roughly 30% slower on larger objects (first file) and
twice as fast on small objects (second files).

I didn't try to optimise this, in fact this is the first time I
even write a non-trivial parser and I didn't really look at any
examples so it's likely far from optimal.

However, I'm very happy with how it turned out and I will use and
tweak it in my future projects that happen to require json parsing.

# License
This code is public domain and comes with no warranty.
You are free to do whatever you want with it. You can
contact me at lolisamurai@tfwno.gf but don't expect any
support.
I hope you will find the code useful or at least
interesting to read. Have fun!
