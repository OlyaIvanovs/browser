set -e

mkdir -p build

# g++ browser.c  -I/usr/include/freetype2 -lfreetype -lX11 -o build/browser -g -std=c++11
# ./build/browser /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-C.ttf "SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE"


gcc browser2.c  -I/usr/include/freetype2 -lfreetype -lX11 -o build/browser -g
./build/browser <parse.html /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-L.ttf "SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE"

# Ubuntu-BI.ttf  Ubuntu-LI.ttf  UbuntuMono-BI.ttf  UbuntuMono-R.ttf  Ubuntu-R.ttf
# Ubuntu-B.ttf   Ubuntu-L.ttf   UbuntuMono-B.ttf   Ubuntu-M.ttf
# Ubuntu-C.ttf   Ubuntu-MI.ttf  UbuntuMono-RI.ttf  Ubuntu-RI.ttf





# ./build/browser /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-C.ttf 


# g++ font_example.c  -I/usr/include/freetype2 -lfreetype -lX11 -o build/font_example -g -std=c++11
# ./build/font_example /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-C.ttf "SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE SAMPLE_SAMPSAMPLE" > example.txt