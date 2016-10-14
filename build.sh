set -e

mkdir -p build
gcc browser2.c  -I/usr/include/freetype2 -lfreetype -lX11 -o build/browser -g
# g++ browser.cpp  -I/usr/include/freetype2 -lfreetype -lX11 -o build/browser -g -std=c++11
# ./build/browser /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-C.ttf "SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE"
./build/browser <parse.html
# ./build/browser /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-C.ttf 


# g++ font_example.c  -I/usr/include/freetype2 -lfreetype -lX11 -o build/font_example -g -std=c++11
# ./build/font_example /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-C.ttf "SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE" > example.txt