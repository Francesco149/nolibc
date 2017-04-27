This is an implementation of file compression using huffman coding.

It's meant to be tiny and in-memory. It will some day be part of
a small gzip deflate decompressor that I will use for http APIs
that return gzipped data.

The encode function is currently partially broken and will not work
for fully binary files that generate too long codes. Text appears
to be working fine 100% of the time.
