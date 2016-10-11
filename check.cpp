#include "loderunner_platform.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // usleep
#include <time.h>
#include <limits.h>

#include "loderunner.h"

global XImage *gXImage;

int main(int argc, char const *argv[]) {
  Display *display;
  Window window;
  int screen;

  display = XOpenDisplay(0);
  if (display == 0) {
    fprintf(stderr, "Cannot open display\n");
    return 1;
  }

  screen = DefaultScreen(display);

  u32 border_color = WhitePixel(display, screen);
  u32 bg_color = BlackPixel(display, screen);

  const int kWindowWidth = 1500;
  const int kWindowHeight = 1000;

  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               kWindowWidth, kWindowHeight, 0, border_color,
                               bg_color);

  XSetStandardProperties(display, window, "My Window", "Hi!", None, NULL, 0,
                         NULL);

  XSelectInput(display, window,
               ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask);
  XMapRaised(display, window);

  Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteMessage, 1);

  usleep(5000);  // 50 ms

  Pixmap pixmap;
  GC gc;
  XGCValues gcvalues;

  // Create x image
  {
    // int depth = 32;
    // int bitmap_pad = 32;
    // int bytes_per_line = 0;
    // int offset = 0;

    // gXImage = XCreateImage(display, CopyFromParent, depth, ZPixmap, offset,
    //                        (char *)GameBackBuffer.Memory, kWindowWidth,
    //                        kWindowHeight, bitmap_pad, bytes_per_line);

    // TODO: find a way to do it with a newly created image
    usleep(500000);
    gXImage = XGetImage(display, window, 0, 0, kWindowWidth, kWindowHeight,
                        AllPlanes, ZPixmap);

    // u32 *Pixel = (u32 *)gXImage->data;
    // for (int i = 0; i < kWindowWidth * 700; i++) {
    //   *Pixel = 0x00FF00FF;
    //   Pixel++;
    // }

    // pixmap = XCreatePixmap(display, window, kWindowWidth,
    //                        kWindowHeight, depth);
    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  // TODO: query monitor refresh rate
  int target_fps = 60;
  r32 target_nspf = 1.0e9f / (r32)target_fps;  // Target ms per frame

  XCloseDisplay(display);

  return 0;
}