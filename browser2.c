
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>


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
#define MAXLEN 100
#define MAXWORD 100
#define BUFSIZE 100
#define RED 0xFF0000
#define GREEN 0x378F1A
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"


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
  int x;
  int y;
};

struct tnode {
  char *name;
  char *classname;
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
  char styleline[MAXLEN];
  char param[MAXLEN];
  char value[MAXLEN];

  int k, l, c, n;
  int index; // for margin, padding
  int parent_exist;
  int i = 0;
  int u = 0;
  int tags_num = 0;

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
      while (getword(word) != '>') {
        if (isalpha(word[0])) {
          stack[stack_size]->classname = my_strdup(word);
          break;
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
                break;
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
                break;
              }
            }
          }
        }
      }
    }
  }


  for (k=0; k<i; k++) {
    printf("Error: missing %s tag\n", stack[k]->name);
  }


  x = y = body_x = body_y = 0;
  // draw html elements
  for (k=0; k<tags_num; k++) {
    if (tags_stack[k]->parent) {
      x = tags_stack[k]->parent->css->x;
      y = tags_stack[k]->parent->css->y;
    } 
    
    tags_stack[k]->css->x = x;
    tags_stack[k]->css->y = y;

    // если ест padding у родителя делаем отступ для элемента
    if (tags_stack[k]->parent && tags_stack[k]->parent->css->paddingleft) {
        tags_stack[k]->css->x += tags_stack[k]->parent->css->paddingleft;
    } 
    // если ширина не указана, растяшиваем блок на ширину родителя
    if (tags_stack[k]->css->width.val == 0) {
       if (tags_stack[k]->parent) {
        tags_stack[k]->css->width.val = tags_stack[k]->parent->css->width.val;
       } else {
        tags_stack[k]->css->width.val = kWindowWidth;
       }
    }
    // изменяем ширину если есть padding
    if (tags_stack[k]->parent && (tags_stack[k]->parent->css->paddingleft || tags_stack[k]->parent->css->paddingright)) {
        tags_stack[k]->css->width.val -= (tags_stack[k]->parent->css->paddingleft + tags_stack[k]->parent->css->paddingright);
    }
    // если у родителя элемента есть padding верхний, отрисоваваем родителя элемента на высоту padding
    // if (tags_stack[k]->parent && tags_stack[k]->parent->css->paddingtop) {
    //   drawdiv(x, y, tags_stack[k]->parent->css->paddingtop,tags_stack[k]->parent->css->width.val, tags_stack[k]->parent->css->bg);
    //   tags_stack[k]->css->y += tags_stack[k]->parent->css->paddingtop; 
    //   y += tags_stack[k]->parent->css->paddingtop;
    //   // обнуляем paddingб чтобы не изменялись координаты остальных детей блока с padding
    //   tags_stack[k]->parent->css->paddingtop = 0;
    // } 
    if (tags_stack[k]->css->paddingtop) {
      drawdiv(tags_stack[k]->css->x, tags_stack[k]->css->y, tags_stack[k]->css->paddingtop,tags_stack[k]->css->width.val, tags_stack[k]->css->bg);
      tags_stack[k]->css->y += tags_stack[k]->css->paddingtop;
      y += tags_stack[k]->css->paddingtop;
    }



    // отрисовка div с известными размерами: ширина и длина
    if (tags_stack[k]->css->height && tags_stack[k]->css->width.val)  {
      // coordinates x y
      form_height = tags_stack[k]->css->height;
      form_width = tags_stack[k]->css->width.val;
      drawdiv(tags_stack[k]->css->x, tags_stack[k]->css->y, form_height, form_width, tags_stack[k]->css->bg);
      y += form_height ;
      // margin-bottom
      if (tags_stack[k]->css->marginbottom) {
        y += tags_stack[k]->css->marginbottom;
      }
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

        int ll;
        for (ll=u; ll>=0; ll--) {
          if(st[ll]->css->height == 0) {
            drawdiv(st[ll]->css->x, prev_y, y-prev_y, st[ll]->css->width.val, st[ll]->css->bg);
            st[ll]->css->y = y;
          }
        }
        // отрисовка элемента
        if (a->css->paddingtop) {
          drawdiv(a->css->x, a->css->y - a->css->paddingtop, a->css->height + a->css->paddingtop, a->css->width.val, a->css->bg);
        } else {
          drawdiv(a->css->x, a->css->y, a->css->height, a->css->width.val, a->css->bg);
        }
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

int getword(char *word) {
  int c, getch(void);
  void ungetch(int);
  char *w = word;

  while(isspace(c = getch()))  
    ;

  if ( (c == '<') && (isalpha(c = getch()) || (c == '/'))) {
    *w++ = c;
    while (isalnum(c = getch())) {
      *w++ = c;
    }
    *w = '\0';
    return word[0];
  } 

  if (isalnum(c)) {
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

