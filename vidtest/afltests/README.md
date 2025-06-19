### These are tests done using AFL (american fuzzy lop) on different functions of libpng which have intermediate complexity and are not suitable for coverage using manual testing approaches.

1. To clear all previous custom installations run: 
```bash
cd ~/Desktop/libpng
make distclean || make clean
find . -name "*.gcno" -o -name "*.gcda" | xargs rm -f
```

2. Rebuild libpng with afl-gcc and coverage flags
```bash
CC=afl-gcc \
CFLAGS="-fprofile-arcs -ftest-coverage -O0 -g" \
./configure --prefix=$PWD/build-afl-cov-gcc

make -j$(nproc)
make install
```

3. Rebuild the fuzzer with afl-gcc (will create the executable): 
```bash 
cd ~/Desktop/libpng/afltests

afl-gcc -fprofile-arcs -ftest-coverage -O0 -g \
  -I../build-afl-cov-gcc/include \
  -L../build-afl-cov-gcc/lib \
  -o fuzz_png_read fuzz_png_read.c \
  -lpng16 -lz -lm
```

4. To avoid errors: 
```bash
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
```

4. Rerun the fuzzer (to regenerate coverage)
```bash
afl-fuzz -i in_corpus -o out_corpus -- ./fuzz_png_read @@
```

5. Generate cov report: 
```bash
afl-cov -d out_corpus \
        --code-dir .. \
        -e "./fuzz_png_read AFL_FILE"
```

6. To stop accumulating coverage over all runs: 
```bash
find . -name "*.gcda" -type f -delete
```

