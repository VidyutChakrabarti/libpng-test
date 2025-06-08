### split_png.c
 
 Read an input PNG, split it vertically into left/right halves,
 and write each half out to separate PNG files.
 
  This version uses the manual read-info → allocate rows → png_read_image →
  png_read_end sequence instead of png_read_png(), to avoid
  “Read Error” mismatches on some libpng builds.

 **Compile with:**
 ```bash
 gcc -o split_png split_png.c -lpng -lz
 ```
 
 **Usage:**
 ```bash
 ./split_png <input.png> <left_output.png> <right_output.png>
 ```