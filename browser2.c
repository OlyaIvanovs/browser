
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H


static int gRunning;
// windows parameters
static const int kWindowWidth = 1500;
static const int kWindowHeight = 1000;
// form parameters
static const int formWidth = 500;
static const int formHeight = 500;
static XImage *gXImage;
static int stack_size;


#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#define COUNT_OF(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))
#define NKEYS (sizeof keywords / sizeof(keywords[0]))  
#define MAXLEN 50
#define MAXTEXT 1000  
#define MAXWORD 100
#define BUFSIZE 100
#define RED 0xFF0000
#define GREEN 0x378F1A
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"

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

char buf[BUFSIZE];
int bufp = 0;

struct stylenode {
  int height;
  // int width;
  union {
   int val;
   char *part;
   } width;
  int bg;
  int margintop;
  int marginbottom;
  int marginleft;
  int marginright;
  int paddingtop;
  int paddingbottom;
  int paddingleft;
  int paddingright;
  int paddingbottomline;
  int x;
  int y;
};

struct tnode {
  char *name;
  char *classname;
  char *textnode;
  struct tnode *parent;
  struct stylenode *css;
};

char *keywords[] = {
  "a", "div", "h1", "p", "span"
};

struct tnode *stack[MAXLEN];
struct tnode *tags_stack[MAXLEN];
int getch(void);
void ungeth(int c);
int getword(char *word);
int gettextnode(char *textnode);
struct tnode *addnode(struct tnode *p, char *w);
int binsearch(char *word, char *keywords[], int n);
char *my_strdup(char *s);
struct tnode *talloc(void);
struct stylenode *salloc(void);
void drawdiv(int x, int y, int height, int width, int bg);




int main(int argc, char **argv) {
  // для отрисовки
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

  int x = 0;
  int y = 0;
  int body_x, body_y, prev_x, prev_y;
  int form_width = 0;
  int form_height = 0;


  // для парсинга
  struct tnode *root;
  struct tnode *a;
  struct tnode *b;
  struct tnode *st[MAXLEN];
  char word[MAXWORD];
  char textnode[MAXTEXT];
  char htextnode[MAXTEXT];
  char styleline[MAXLEN];
  char param[MAXLEN];
  char value[MAXLEN];

  int k, l, c, n;
  int index; // for margin, padding
  int parent_exist;
  int i = 0;
  int u = 0;
  int tags_num = 0;


  // for text. FREE TYPE
  FT_Library library;
  FT_Face face;
  FT_GlyphSlot slot;
  FT_Error error;
  char *filename;
  char *text;
  int target_height;
  int n1, num_chars;

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
  for ( n1 = 0; n1 < num_chars; n1++ )
  {
    /* load glyph image into the slot (erase previous one) */

    error = FT_Load_Char( face, text[n1], FT_LOAD_RENDER );
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


  root = NULL;

  // создание окна
  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               kWindowWidth, kWindowHeight, 0, border_color,
                               bg_color);
  XSetStandardProperties(display, window, "Browser", "Hi!", None, NULL, 0,
                         NULL);
  XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask |
                                    ButtonPressMask | ButtonReleaseMask |
                                    StructureNotifyMask);
  // XMapRaised(display, window);
  XMapWindow( display , window );
  // XMapWindow( mydisplay , sliderbedWindow );
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


  //парсинг
  while (getword(word) != EOF) {
  // для закрывающего тега
    if (word[0] == '/') {
      l = 1;
      while(word[l] != '\0') {
        word[l-1] = word[l];
        l++;
      }
      word[l-1] = '\0';
      // удаления тега из списка
      if ((binsearch(word, keywords, NKEYS)) >= 0) {
        if (strcmp(stack[stack_size]->name, word) == 0) {
          stack[stack_size] = NULL;
          stack_size--;
          i--;
        } else {
          printf("Error: missing %s tag\n", stack[stack_size]->name);
          return 0;
        }
      }
      // открывающийся тег
    } else if ((binsearch(word, keywords, NKEYS)) >= 0) {
      root = addnode(root, word);
      stack[i] = root;
      tags_stack[tags_num] = root;
      stack_size = i;
      i++;
      tags_num++;
      // Yазвание класса
    } else if (strcmp("class", word) == 0) {
      while (getword(word) != '"');
      while (getword(word) != '"') {
        if (isalpha(word[0])) {
          stack[stack_size]->classname = my_strdup(word);
        }
      }
      //стили
    } else if (strcmp("style", word) == 0) {
      while (getword(word) != '"') ;
      while (getword(word) != '"') {
        if (strcmp("height", word) == 0) {
          while (getword(word) != ';') {
            if (isalnum(word[0])) {
              stack[stack_size]->css->height = atoi(word);
            }
          }
        } else if (strcmp("width", word) == 0){
          while (getword(word) != ';') {
            if (isalnum(word[0])) {
              stack[stack_size]->css->width.val = atoi(word);
            }
          }
        } else if (strcmp("background", word) == 0){
          while (getword(word) != ';') {
            if (isalnum(word[0])) {
              stack[stack_size]->css->bg = (int)strtol(word, NULL, 16);
            }
          }
        } else if (strcmp("margin", word) == 0){
          index = 0;
          while (getword(word) != ';') {
            if (isalnum(word[0])) {
              if (index == 0) {
                stack[stack_size]->css->margintop = atoi(word);
                index++;
              } else if (index == 1) {
                stack[stack_size]->css->marginright = atoi(word);
                index++;
              } else if (index == 2) {
                stack[stack_size]->css->marginbottom = atoi(word);
                index++;
              } else if (index == 3) {
                stack[stack_size]->css->marginleft = atoi(word);
              }
            }
          }
        } else if (strcmp("padding", word) == 0){
          index = 0;
          while (getword(word) != ';') {
            if (isalnum(word[0])) {
              if (index == 0) {
                stack[stack_size]->css->paddingtop = atoi(word);
                index++;
              } else if (index == 1) {
                stack[stack_size]->css->paddingright = atoi(word);
                index++;
              } else if (index == 2) {
                stack[stack_size]->css->paddingbottom = atoi(word);
                index++;
              } else if (index == 3) {
                stack[stack_size]->css->paddingleft = atoi(word);
              }
            }
          }
        } 
      }
    } else {
      //textnode 1.после '>'
      if (word[0] == '>'){
        // отделяем слово от '>'
        l = 1;
        while(word[l] != '\0') {
          word[l-1] = word[l];
          l++;
        }
        word[l-1] = '\0';
        if (l > 1) {
          gettextnode(textnode);
          strcpy(htextnode, word);
          strcat(htextnode, textnode);
          stack[stack_size]->textnode = my_strdup(htextnode);
        }
      } else {
        // если срока отделена от < пробелом, то:
        gettextnode(textnode);
        strcpy(htextnode, word);
        strcat(htextnode, textnode);
        stack[stack_size]->textnode = my_strdup(htextnode);
      }
    } 
  }


  for (k=0; k<i; k++) {
    printf("Error: missing %s tag\n", stack[k]->name);
  }


  x = y = body_x = body_y = 150;
  // draw html elements
  for (k=0; k<tags_num; k++) {
    if (tags_stack[k]->parent) {
      x = tags_stack[k]->parent->css->x;
      y = tags_stack[k]->parent->css->y;
    } 
    
    tags_stack[k]->css->x = x;
    tags_stack[k]->css->y = y;

    if (!tags_stack[k]->css->bg && tags_stack[k]->parent) {
      tags_stack[k]->css->bg = tags_stack[k]->parent->css->bg;
    }

    // если ест padding у родителя делаем отступ для элемента
    if (tags_stack[k]->parent && tags_stack[k]->parent->css->paddingleft) {
        tags_stack[k]->css->x += tags_stack[k]->parent->css->paddingleft;
    } 
    // если margin делаем отступ для элемента
    if (tags_stack[k]->css->marginleft) {
        tags_stack[k]->css->x += tags_stack[k]->css->marginleft;
    } 
    // если ширина не указана, растяшиваем блок на ширину родителя
    if (tags_stack[k]->css->width.val == 0) {
       if (tags_stack[k]->parent) {
        tags_stack[k]->css->width.val = tags_stack[k]->parent->css->width.val;
        // изменяем ширину если есть padding
        if (tags_stack[k]->parent->css->paddingleft || tags_stack[k]->parent->css->paddingright) {
            tags_stack[k]->css->width.val -= (tags_stack[k]->parent->css->paddingleft + tags_stack[k]->parent->css->paddingright);
        }
        // изменяем ширину если есть margin
        if (tags_stack[k]->css->marginleft || tags_stack[k]->css->marginright) {
          tags_stack[k]->css->width.val -= (tags_stack[k]->css->marginleft + tags_stack[k]->css->marginright);
        }
      } else {
        tags_stack[k]->css->width.val = kWindowWidth;
      }
    } else if ((tags_stack[k]->parent && tags_stack[k]->css->width.val < tags_stack[k]->parent->css->width.val) || (tags_stack[k]->css->width.val < kWindowWidth)) {
      if (tags_stack[k]->css->paddingleft || tags_stack[k]->css->paddingright) {
        tags_stack[k]->css->width.val += (tags_stack[k]->css->paddingleft + tags_stack[k]->css->paddingright);
      }
    }
    
    //paddingtop
    if (tags_stack[k]->css->paddingtop) {
      drawdiv(tags_stack[k]->css->x, tags_stack[k]->css->y, tags_stack[k]->css->paddingtop,tags_stack[k]->css->width.val, tags_stack[k]->css->bg);
      tags_stack[k]->css->y += tags_stack[k]->css->paddingtop;
      y += tags_stack[k]->css->paddingtop;
    }

    //margin-top
    int marg;
    if (tags_stack[k]->css->margintop) {
      //Problem: The margins of adjacent siblings are collapsed 
      if (k >= 1 && (tags_stack[k]->parent == tags_stack[k-1]->parent) && 
        !tags_stack[k-1]->css->paddingbottom && !tags_stack[k]->css->paddingtop 
        && tags_stack[k-1]->css->marginbottom) {
        marg = (tags_stack[k-1]->css->marginbottom >= tags_stack[k]->css->margintop) ? 0 : (tags_stack[k]->css->margintop - tags_stack[k-1]->css->marginbottom); 
      } else {
        marg = tags_stack[k]->css->margintop;
      }
      y += marg;
      tags_stack[k]->css->y += marg;
    }

    // отрисовка div с известными размерами: высота и ширина
    if (tags_stack[k]->css->height && tags_stack[k]->css->width.val)  {
      // coordinates x y
      form_height = tags_stack[k]->css->height;
      form_width = tags_stack[k]->css->width.val;
      drawdiv(tags_stack[k]->css->x, tags_stack[k]->css->y, form_height, form_width, tags_stack[k]->css->bg);
      y += form_height ;
      // margin-bottom
      if (tags_stack[k]->css->marginbottom) {
        y += tags_stack[k]->css->marginbottom;
        tags_stack[k]->css->y += tags_stack[k]->css->marginbottom;
      }
    }

    //отрисовка текста
    if (tags_stack[k]->textnode) {
      num_chars = strlen(tags_stack[k]->textnode);
      for (n1 = 0; n1 < num_chars; n1++ ) {
        error = FT_Load_Char( face, tags_stack[k]->textnode[n1], FT_LOAD_RENDER );
        if ( error ) continue;  
        draw_bitmap( &slot->bitmap, slot->bitmap_left + metr, slot->bitmap_top + metr_top);
        metr += slot->metrics.width/64*1.35;
        if (metr > (WIDTH - slot->metrics.width/64*1.35)) {
          metr = 0;
          metr_top += slot->metrics.height/64*1.35;
        } 
      }
    }

    //padding-bottom
    if (tags_stack[k]->css->paddingbottom) {
      drawdiv(tags_stack[k]->css->x, tags_stack[k]->css->y, tags_stack[k]->css->paddingbottom,tags_stack[k]->css->width.val, tags_stack[k]->css->bg);
      y += tags_stack[k]->css->paddingbottom;
    }

    //margin-bottom
    if (tags_stack[k]->css->marginbottom) {
        y += tags_stack[k]->css->marginbottom;
      }

    // дорисовка "родителей" diva
    if (!tags_stack[k]->parent) {
      // координаты точки 'потока'
      body_x = x;
      body_y = y;
    } else {
      a  = tags_stack[k];
      //старые координаты (x, y)  родителя
      prev_x = a->parent->css->x;
      prev_y = a->parent->css->y;
      // a->parent->css->x = x;
      a->parent->css->y = y;

      if (a->parent->css->height == 0) {
        // находим всех родителей и перерисовываем их
        u = 0;
        b = a->parent;
        st[u] = b;
        while (b->parent) {
          st[++u] = b->parent;
          b = b->parent;
        }

        int num_elems;
        int paddingbottomheight = 0;
        //paddingbottom; высота линии, которую дорисовываем внизу элемента paddingbottomheight (padding + marginbottom предыдущего элемента)
        for (num_elems=0; num_elems<=u; num_elems++) {
          if(st[num_elems]->css->height == 0) {
            paddingbottomheight += st[num_elems]->css->paddingbottom;
            if (num_elems-1 >= 0) {
              paddingbottomheight += st[num_elems-1]->css->marginbottom;
            }          
            st[num_elems]->css->paddingbottomline = paddingbottomheight;
          }
        }

        for (num_elems=u; num_elems>=0; num_elems--) {
          if(st[num_elems]->css->height == 0) {
            drawdiv(st[num_elems]->css->x, prev_y, y-prev_y, st[num_elems]->css->width.val, st[num_elems]->css->bg);
            st[num_elems]->css->y = y;
            drawdiv(st[num_elems]->css->x, st[num_elems]->css->y, st[num_elems]->css->paddingbottomline ,st[num_elems]->css->width.val, st[num_elems]->css->bg);
            if (st[num_elems]->parent) {
              st[num_elems]->parent->css->y += st[num_elems]->css->paddingbottomline;
            }
          }
        }
        // отрисовка элемента
        drawdiv(a->css->x, a->css->y - a->css->paddingtop, a->css->paddingtop + a->css->height + a->css->paddingbottom, a->css->width.val, a->css->bg);     
      }
    }
  }

  
  gRunning = 1;
  while (gRunning) {
    // Process events

    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      // Close window message
      if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wmDeleteMessage) {
          gRunning = 0;
        }
      }
    }

    uint32_t *pixel_data = (uint32_t *)gXImage->data;
    // отрисовка текста
    for (int v = 0; v < HEIGHT; v++) {
      for (int z = 0; z < WIDTH; z++) {
        if (image[v][z] != 0) {
          *(pixel_data + z) = 0x000000;
        }
      }
      pixel_data = pixel_data + kWindowWidth;
    }

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);

    usleep(10000);
  }

  XCloseDisplay(display);
  return 0;
}


void drawdiv(int x, int y, int height, int width, int bg) {
  uint32_t *pixel_data = (uint32_t *)gXImage->data;
  int v, z;

  // отрисовка квадрата
  pixel_data = pixel_data + (kWindowWidth * y) + x;
  for (v = 0; v < height; v++) {
    for (z = 0; z < width; z++) {
      *(pixel_data + z) = bg;
    }
    pixel_data = pixel_data + kWindowWidth;
  }
}


struct tnode *addnode(struct tnode *p, char *w) {
  int cond;

  p = talloc(); /* make a new node */
  p->name = my_strdup(w);
  p->parent = stack[stack_size];
  p->classname = NULL;
  p->css = salloc();
  p->css->height = 0;
  p->css->width.val = 0;
  p->css->bg = 0;
  p->css->x = 0;
  p->css->y = 0;
  p->css->margintop = 0;
  p->css->marginbottom = 0;
  p->css->marginleft = 0;
  p->css->marginright = 0;
  p->css->paddingtop = 0;
  p->css->paddingbottom = 0;
  p->css->paddingleft = 0;
  p->css->paddingright = 0;
  // p->css->bg = 0xFFFFFF;
  return p;
}

struct stylenode *addstyle(struct stylenode *p) {
  p->height = 0;
  p->width.val = 0;
  p->bg = 0;
  return p;
}

int gettextnode(char *textnode) {
  int c, getch(void);
  void ungetch(int);
  char *w = textnode;

  while ((c=getch()) != '<') {
    *w++ = c;
  }
  *w = '\0';
  ungetch(c);
  return textnode[0];
}

int getword(char *word) {
  int c, getch(void);
  void ungetch(int);
  char *w = word;

  while(isspace(c = getch()))  
    ;
  if (c == '<') {
    if ((isalpha(c = getch()) || (c == '/'))) {
      *w++ = c;
      while (isalnum(c = getch())) {
        *w++ = c;
      }
      *w = '\0';
      return word[0];
      //ignore comments
    } else if (c == '!' && (c = getch()) == '-' && (c = getch()) == '-') {
      while ((c = getch()) != '-' && (c = getch()) != '-' && (c = getch()) != '-')
        ;
    }
  }


  if (isalnum(c) || c == '>') {
    *w++ = c;
    while (isalnum(c = getch())) {
      *w++ = c;
    }
    ungetch(c);
    *w = '\0';
    return word[0];
  } else {
    *w = '\n';
    return c;
  }
}

int getch(void) {
  return (bufp > 0) ? buf[--bufp] : getchar();
}

void ungetch(int c) {
  if (bufp >= BUFSIZE) {
    printf("ungetch: too many characters\n");
  } else {
    buf[bufp++] = c;
  }
}

int binsearch(char *word, char *keywords[], int n) {
  int cond;
  int low, high, mid;

  low = 0;
  high = n - 1;

  while (low <= high) {
    mid = (low+high) / 2;
    if ((cond = strcmp(word, keywords[mid])) < 0) {
      high = mid - 1;
    } else if (cond > 0) {
      low = mid + 1;
    } else {
      return mid;
    }
  }
  return -1;
}

char *my_strdup(char *s) {
  char *p;

  p = (char *)malloc(strlen(s) + 1);
  if (p != NULL)
    strcpy(p, s);
  return p;
}

struct tnode *talloc(void) {
  return  (struct tnode *) malloc(sizeof(struct tnode));
}

struct stylenode *salloc(void) {
  return  (struct stylenode *) malloc(sizeof(struct stylenode));
}

