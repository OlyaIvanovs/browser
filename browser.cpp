#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


static bool gRunning;
// windows parameters
static const int kWindowWidth = 1500;
static const int kWindowHeight = 1000;
// form parameters
static const int formWidth = 500;
static const int formHeight = 500;
static XImage *gXImage;

#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#define COUNT_OF(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))
  
#define RED 0xFF0000

void drawdiv(int x, int y, int height, int width);


int main(int argc, char **argv) {
  Display *display;
  Window window;

  display = XOpenDisplay(0);
  if (display == 0) {
    fprintf(stderr, "Cannot open display\n");
    return 1;
  }

  int screen = DefaultScreen(display);
  int border_color = WhitePixel(display, screen);
  int bg_color = WhitePixel(display, screen);
  int x, y;
  int form_width, form_height;

  // создание окна
  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               kWindowWidth, kWindowHeight, 0, border_color,
                               bg_color);
  XSetStandardProperties(display, window, "Browser", "Hi!", None, NULL, 0,
                         NULL);
  XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask |
                                    ButtonPressMask | ButtonReleaseMask |
                                    StructureNotifyMask);
  XMapRaised(display, window);
  Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteMessage, 1);

  GC gc;
  XGCValues gcvalues;

  // Create x image
  {
    for (;;) {
      XEvent e;
      XNextEvent(display, &e);
      if (e.type == MapNotify) break;
    }
    gXImage = XGetImage(display, window, 0, 0, kWindowWidth, kWindowHeight,
                        AllPlanes, ZPixmap);
    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  gRunning = true;
  while (gRunning) {
    // Process events

    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      // Close window message
      if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wmDeleteMessage) {
          gRunning = false;
        }
      }
    }

    // отрисовка квадрата
    form_width = form_height = 50;
    x = y = 370;
    drawdiv(x, y, form_height, form_width);

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);

    usleep(10000);
  }

  XCloseDisplay(display);
  return 0;
}


void drawdiv(int x, int y, int height, int width) {
  uint32_t *pixel_data = (uint32_t *)gXImage->data;
  int v, z;

  // отрисовка квадрата
  pixel_data = pixel_data + (kWindowWidth * y) + x;
  for (v = 0; v < height; v++) {
    for (z = 0; z < width; z++) {
      *(pixel_data + z) = RED;
    }
    pixel_data = pixel_data + kWindowWidth;
  }
}



<div class="hoverBox" style="background: #dee256; width: 600; padding: 20 20 20 20;">
    <div class="obscuresLayer12" style="width: 500; background: #4286f4; padding: 30 30 30 30; margin: 20 20 20 20;">
        <div class="obscuresLayer123" style="width: 400; background: #42f450; padding: 30 30 30 30;">
            <div class="obscuresLayer1234567" style="width: 300; background: #2B49F0; padding: 20 20 20 20;">
                <div class="obscuresLayer12345678" style="height: 60; background: #F0D22B; margin: 20 20 20 20;"></div>
                <div class="obscuresLayer123456789" style="height: 60; background: #E36A14; margin: 20 20 20 20;"></div>
                <div class="obscuresLayer1234567890" style="height: 60; background: #F02BAC; margin: 10 10 10 10;"></div>
            </div>
            <div class="obscuresLayer1234" style="width: 300; height: 80; background: #1A728F; padding: 20 20 20 20; margin: 20 20 20 20;">
                <div class="obscuresLayer12345" style="height: 30; background: #E36A14;"></div>
                <div class="obscuresLayer123456" style="height: 30; background: #F02BAC;"></div>
            </div>
        </div>
        <div class="obscuresLayer12345678901" style="width: 400; background: #F02BAC; padding: 30 30 30 30;">
            <div class="obscuresLayer12345678902" style="width: 300; height: 160; background: #1A728F; padding: 20 20 20 20;">
                <div class="obscuresLayer12345678903" style="height: 60; background: #E36A14;"></div>
            </div>
        </div>
    </div>
</div>

