#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H

static bool gRunning;

// windows parameters
static const int kWindowWidth = 1500;
static const int kWindowHeight = 1000;

// form parameters
static const int formWidth = 500;
static const int formHeight = 500;

static XImage *gXImage;

#define Assert(Expression) \
  if (!(Expression)) {     \
    *(int *)0 = 0;         \
  }

#define COUNT_OF(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

// the structure for keyboard
struct user_input {
  union {
    bool buttons[8];
    struct {
      bool up;
      bool down;
      bool left;
      bool right;
      bool mouse_left;
      bool mouse_middle;
      bool mouse_right;

      bool terminator;
    };
  };
};

// textarea parameters
#define WIDTH 640
#define HEIGHT 480
unsigned char image[HEIGHT][WIDTH];

// textarea symbols
void
draw_bitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y) {
  FT_Int i, j, p, q;
  FT_Int x_max = x + bitmap->width;
  FT_Int y_max = y + bitmap->rows;

  for (i = x, p = 0; i < x_max; i++, p++) {
    for (j = y, q = 0; j < y_max; j++, q++) {
      if (i < 0 || j < 0 || i >= WIDTH || j >= HEIGHT) continue;

      image[j][i] = bitmap->buffer[q * bitmap->width + p];
    }
  }
}

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

  int X_mouse, Y_mouse, x, y;
  int form_width, form_height;

  // для увеличения квадрата по клику мышкой
  bool mouse_click;
  bool enlargment_form;

  // for text. FREE TYPE
  FT_Library library;
  FT_Face face;
  FT_GlyphSlot slot;
  FT_Error error;
  char *filename;
  char *text;
  int target_height;
  int n, num_chars;

  // if (argc != 3) {
  //   fprintf(stderr, "usage: %s font sample-text\n", argv[0]);
  //   exit(1);
  // }

  filename = argv[1];
  text = argv[2];
  num_chars = strlen(text);

  // Errors Freetype
  error = FT_Init_FreeType(&library);                 /* initialize library */
  if ( error ) {
    printf("an error occurred during library initialization");
  }

  error = FT_New_Face(library, filename, 0, &face);   /* create face object */
  if ( error == FT_Err_Unknown_File_Format ) {
    printf("that its font format is unsupported");
  } else if ( error ) {
    printf("font file could not be opened or read, or that it is broken");
  }

  error = FT_Set_Char_Size(face, 20 * 64, 0, 100, 0); /* set character size */
  if ( error ) {
    printf("an error occurred during seting character size");
  }


  slot = face->glyph;
  int metr = 0;
  int metr_top = 0;

  // картинка для каждой отдельной буквы
  for ( n = 0; n < num_chars; n++ )
  {
    /* load glyph image into the slot (erase previous one) */

    error = FT_Load_Char( face, text[n], FT_LOAD_RENDER );
    if ( error )
      continue;                 /* ignore errors */

    draw_bitmap( &slot->bitmap,
                 slot->bitmap_left + metr,
                 slot->bitmap_top + metr_top);
    // каждую следующую букву смещаем на ширину предыдущей * 1.35
    // ширинв slot измеряется не в пикселях а в точках. Чтобы получить кол-во пиксеоей надо поделить на 64
    metr += slot->metrics.width/64*1.35;
    // если строчка в ширину кончилась, перенос на следующую строку
    if (metr > (WIDTH - slot->metrics.width/64*1.35)) {
      metr = 0;
      metr_top += slot->metrics.height/64*1.35;
    } 
  }


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


  // for keyboards events
  user_input inputs[2];
  user_input *old_input = &inputs[0];
  user_input *new_input = &inputs[1];
  *new_input = {};

  Assert(&new_input->terminator - &new_input->buttons[0] <
         COUNT_OF(new_input->buttons))


  // для увеличения квадрата по клику
  gRunning = true;
  enlargment_form = false;
  mouse_click = false;
  form_width = form_height = 50;
  x = y = 370;
  printf("lalalal\n");

  while (gRunning) {
    // Process events

    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      // Mouse Click
      // Get info about current pointer position */
      if (event.type == ButtonPress) {
        // printf("event type for button press = (%d)\n", event.type);
        XQueryPointer(display, window, &event.xbutton.root,
                      &event.xbutton.window, &event.xbutton.x_root,
                      &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
                      &event.xbutton.state);
        // printf("Mouse Coordinates: %d %d\n", event.xbutton.x,
        // event.xbutton.y);

        X_mouse = event.xbutton.x;
        Y_mouse = event.xbutton.y;

        form_width = form_height = 0;

        mouse_click = true;
        enlargment_form = true;
      }

      if (event.type == ButtonRelease) {
        enlargment_form = false;
      }

      // KeyPress
      KeySym key;
      char buf[256];
      char symbol = 0;

      if (XLookupString(&event.xkey, buf, 255, &key, 0) == 1) {
        symbol = buf[0];
      }

      bool pressed = false;
      bool released = false;
      bool retriggered = false;

      // Process user input
      if (event.type == KeyPress) {
        pressed = true;
      }

      if (event.type == KeyRelease) {
        if (XEventsQueued(display, QueuedAfterReading)) {
          XEvent nev;
          XPeekEvent(display, &nev);

          if (nev.type == KeyPress && nev.xkey.time == event.xkey.time &&
              nev.xkey.keycode == event.xkey.keycode) {
            // Ignore. Key wasn't actually released
            XNextEvent(display, &event);
            retriggered = true;
          }
        }
        if (!retriggered) {
          released = true;
        }
      }

      if (pressed || released) {
        if (key == XK_Escape) {
          gRunning = false;
        } else if (key == XK_Up) {
          new_input->up = pressed;
        } else if (key == XK_Down) {
          new_input->down = pressed;
        } else if (key == XK_Left) {
          new_input->left = pressed;
        } else if (key == XK_Right) {
          new_input->right = pressed;
        }
      }

      // Close window message
      if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wmDeleteMessage) {
          gRunning = false;
        }
      }
    }

    // draw
    {
      uint32_t *pixel_data = (uint32_t *)gXImage->data;

      for (int pixel = 0; pixel < kWindowHeight * kWindowWidth; pixel++) {
        *(pixel_data + pixel) = 0x00FFFF;
      }

      if (new_input->up) {
        y -= 5;
      }
      if (new_input->down) {
        y += 5;
      }
      if (new_input->right) {
        x += 5;
      }
      if (new_input->left) {
        x -= 5;
      }

      if (mouse_click) {
        x = X_mouse;
        y = Y_mouse;
      }

      if (enlargment_form) {
        if (form_width < formWidth) {
          form_width += 5;
        }

        if (form_height < formHeight) {
          form_height += 5;
        }
      }

      // отрисовка текста
      for (int v = 0; v < HEIGHT; v++) {
        for (int z = 0; z < WIDTH; z++) {
          if (image[v][z] != 0) {
            *(pixel_data + z) = 0x000000;
          }
        }
        pixel_data = pixel_data + kWindowWidth;
      }
    }

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);

    // Swap inputs
    user_input *tmp = old_input;
    old_input = new_input;
    new_input = tmp;

    // Zero input
    *new_input = {};

    // Retain the button state
    for (int i = 0; i < COUNT_OF(new_input->buttons); i++) {
      new_input->buttons[i] = old_input->buttons[i];
    }

    usleep(10000);
  }
  FT_Done_Face    ( face );
  FT_Done_FreeType( library );
  XCloseDisplay(display);

  return 0;
}
