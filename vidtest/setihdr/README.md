Instructions: 

1. make clean
2. Clean up other custom installations of libpng if you have one to allow proper dynamic linking with the instrumented version only.
```bash
find /usr/local /usr /lib /usr/lib /usr/local/lib -name "libpng*"
find /usr/local/include /usr/include -name "png*"
```

3. Install
```bash
CFLAGS="--coverage -O0" ./configure --prefix=$PWD/build
make
make install
```

4. Compile
```bash
gcc --coverage -O0 \
    -I$PWD/build/include \
    -L$PWD/build/lib \
    -o test_setihdr test_setihdr.c \
    -lpng16 -lz
```

5. Run your test
```bash
LD_LIBRARY_PATH=$PWD/build/lib ./test_setihdr
```

6. generate coverage report
```bash 
find . -name "*.gcda" -delete && LD_LIBRARY_PATH=$PWD/build/lib ./test_setihdr
```

7. see lines that have been executed: 
```bash
grep -A 20 'png_set_IHDR' pngset.c.gcov
```

Interpret the Output:

- Lines with a number (e.g., 12:) were executed that many times.
- Lines with #####: were not executed.
- Lines with -: are not executable (comments, braces, etc.).

