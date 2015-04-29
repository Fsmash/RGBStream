//
//371 Lab1 Selfie Image Editor
//
//lab1.cpp
//cs371 Fall 2014
//Gordon Griesel
//
//sources of interest
//http://caca.zoy.org/study/index.html
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "log.h"
#include "ppm.h"

//-------------------------------------------
//XWindows globals to make your life easier.
Display *dpy;
Window win;
GC gc;
//---------------------------
void init();
void init_xwindows();
void cleanup_xwindows();
void check_resize(XEvent *e);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void render();
//
#define MAX_IMAGES 9
//a global structure
struct G {
    int xres, yres;
    int draw;
    int imageNum;
    int grayscale;
    int dither;
    int rgbDither;
    int rotate;
    int menuWidth;
    char imageName[MAX_IMAGES][64];
    int nimages;
} g;


int main()
{
    int done=0;
    logOpen();
    init();
    init_xwindows();
    while(!done) {
        //Check the event queue
        while(XPending(dpy)) {
            XEvent e;
            XNextEvent(dpy, &e);
            check_resize(&e);
            check_mouse(&e);
            done = check_keys(&e);
            render();
        }
    }
    cleanup_xwindows();
    logClose();
    return 0;
}

void init()
{
    //Get a list of all .ppm files in current directory.
    struct dirent *entry;
    DIR *dp = opendir(".");
    g.nimages = 0;
    if (dp) {
        while ((entry = readdir(dp)) != NULL
                && g.nimages < MAX_IMAGES ) {
            if (strstr(entry->d_name, ".ppm"))
                strcpy(g.imageName[g.nimages++], entry->d_name);
        }
        closedir(dp);
    }
    //initialize variables
    g.xres=640;
    g.yres=480;
    g.draw=0;
    g.imageNum=0;
    g.grayscale=0;
    g.dither=0;
    g.rgbDither=0;
    g.rotate=0;
    g.menuWidth=160;
}

void cleanup_xwindows()
{
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void init_xwindows()
{
    int scr;

    if(!(dpy=XOpenDisplay(NULL))) {
        fprintf(stderr, "ERROR: could not open display\n");
        exit(EXIT_FAILURE);
    }
    scr = DefaultScreen(dpy);
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 1, 1,
            g.xres, g.yres, 0, 0x00ffffff, 0x00000000);
    gc = XCreateGC(dpy, win, 0, NULL);
    XMapWindow(dpy, win);
    XSelectInput(dpy, win, ExposureMask | StructureNotifyMask |
            PointerMotionMask | ButtonPressMask |
            ButtonReleaseMask | KeyPressMask);
    XStoreName(dpy, win, "CS371 - Lab1 Student Selfie Editor");
}

void check_resize(XEvent *e)
{
    //ConfigureNotify is sent when window size changes.
    if (e->type != ConfigureNotify)
        return;
    XConfigureEvent xce = e->xconfigure;
    g.xres = xce.width;
    g.yres = xce.height;
}

void resize_window(int w, int h)
{
    g.xres = w;
    g.yres = h;
    unsigned int value_mask=0;
    value_mask |= CWWidth;
    value_mask |= CWHeight;
    XWindowChanges xwc;
    xwc.width = w;
    xwc.height = h;
    XConfigureWindow(dpy, win, value_mask, &xwc);
}

void check_image_fit(int width, int height)
{
    //make sure image fits in window.
    if (width+g.menuWidth > g.xres || height > g.yres) {
        resize_window(width+g.menuWidth, height);
    }
}

void clear_screen()
{
    XClearWindow(dpy, win);
}

void check_mouse(XEvent *e)
{
    static int savex = 0;
    static int savey = 0;
    //
    int mx = e->xbutton.x;
    int my = e->xbutton.y;
    //
    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        //Log("ButtonPress %i %i\n", e->xbutton.x, e->xbutton.y);
        if (e->xbutton.button==1) {
            //Left button pressed
        }
        if (e->xbutton.button==3) {
            //Right button pressed
        }
    }
    if (savex != mx || savey != my) {
        //mouse moved
        savex = mx;
        savey = my;
    }
}

int check_keys(XEvent *e)
{
    int key = XLookupKeysym(&e->xkey, 0);
    if (e->type != KeyPress)
        return 0;
    if (g.nimages <= 0)
        return 1;
    //a key was pressed
    if (key >= XK_1 && key <= XK_9) {
        g.imageNum = key - XK_1;
        g.imageNum = (g.imageNum > g.nimages) ? g.nimages : g.imageNum;
        g.draw=1;
    }
    switch (key) {
        case XK_r:
            g.rgbDither ^= 1;
            break;
        case XK_g:
            g.grayscale ^= 1;
            break;
        case XK_d:
            g.dither ^= 1;
            if (g.dither && !g.grayscale)
                g.grayscale=1;
            break;
        case XK_z:
            if (g.rotate >= 3) {
                g.rotate = 0;
            }
            else {
                g.rotate++;
            }
            break;
        case XK_equal:
        case XK_minus:
            break;
        case XK_Delete:
            break;
        case XK_Escape:
            //quitting the program
            return 1;
    }
    //clear_screen();
    g.draw=1;
    return 0;
}

inline void set_color_3i(int r, int g, int b)
{
    unsigned long cref = 0L;
    cref += r;
    cref <<= 8;
    cref += g;
    cref <<= 8;
    cref += b;
    XSetForeground(dpy, gc, cref);
}

void show_menu()
{
    int i,x=10, y=12;
    char ts[64];
    set_color_3i(10,10,10);
    XFillRectangle(dpy, win, gc, 0, 0, g.menuWidth, g.yres);
    if (g.nimages <= 0) {
        set_color_3i(255,255,0);
        sprintf(ts,"NOTICE:");
        XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
        y += 16;
        sprintf(ts,"No image found!");
        XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
        y += 16;
        sprintf(ts,"Put some PPM images in your directory.");
        XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
        return;
    }
    set_color_3i(255,255,255);
    sprintf(ts,"Show Image...");
    XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
    y += 16;
    for (i=0; i<g.nimages; i++) {
        sprintf(ts,"%i - %s",i+1,g.imageName[i]);
        XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
        y += 16;
    }
    y += 16;
    sprintf(ts,(g.grayscale)?"G - Gray Scale (ON)":"G - Gray Scale (OFF)");
    XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
    y += 16;
    sprintf(ts,(g.dither)?"D - Dither (ON)":"D - Dither (OFF)");
    XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
    y += 16;
    sprintf(ts,(g.rgbDither)?"R - RGB Dither (ON)":"R - RGB Dither (OFF)");
    XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
    y += 16;
    if (g.rotate == 0) sprintf(ts,"Z - Rotate 0 degrees");
    if (g.rotate == 1) sprintf(ts,"Z - Rotate 90 degrees");
    if (g.rotate == 2) sprintf(ts,"Z - Rotate 180 degrees");
    if (g.rotate == 3) sprintf(ts,"Z - Rotate 270 degrees");
    XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
    y += 16;
    sprintf(ts,"Esc - Quit");
    XDrawString(dpy, win, gc, x, y, ts, strlen(ts));
    y += 16;
    //vertical line
    //set_color_3i(200,200,200);
    //XDrawLine(dpy, win, gc, 160, 0,160,yres);
}

void floydSteinbergDither(unsigned char *ucdata, int h, int w)
{
    //Floyd Steinberg dithering
    //
    //Follow these steps:
    //1. Create an integer array that can hold 3 colors for each pixel.
    //2. Copy the color data to the integer array.
    //      note: We use integers because a color value could go negative or
    //            above 255 during the algorithm.
    //3. Apply Floyd-Steinberg algorithm:
    //   Starting at the beginning of the integer data, apply the
    //      At each pixel...
    //         Set the pixel to black or white
    //         Record the change in color that happened
    //         Distribute this change (error) to the pixels around it
    //         using the following pattern:
    //
    //                  *     7/16
    //          3/16   5/16   1/16 
    //
    //      This means:
    //      Add 7/16 of the error to the pixel to the right
    //      Add 3/16 of the error to the pixel to the lower-left
    //      Add 5/16 of the error to the pixel below
    //      Add 1/16 of the error to the pixel to the lower-right
    //
    //   Check pixel coordinates to verify they are not outside the image.
    //
    //4. Put integer pixels back into the original data structure.
    //5. Display the image on screen.
    //6. Free the integer array.
    int *idata = new int[w * h * 3]; 
    int *iptr = idata;

    for (int i = 0; i < w * h * 3; i++) {
        idata[i] = (int)ucdata[i];
    }

    for (int j = 0; j < h; j++) {
        for (int k = 0; k < w; k++) {
            int error;
            int newpixel;    
            int oldpixel = *iptr;

            if (oldpixel > 128){
                newpixel = 255;
                error = oldpixel - 255;
            }

            else { 
                newpixel = 0;
                error = oldpixel;
            }

            *(iptr + 2) = *(iptr + 1) = *(iptr) = newpixel;

            if (k == 0 && j != h - 1) {
                *(iptr + 3) = *(iptr + 3) + error * 7/16;
                *(iptr + (w * 3) + 3) = *(iptr + (w * 3) + 3) + error * 1/16; 
                *(iptr + (w * 3)) = *(iptr + (w * 3)) + error * 5/16; 
            }

            if (k == w - 1 && j != h - 1) {
                *(iptr + (w * 3)) = *(iptr + (w * 3)) + error * 5/16; 
                *(iptr + (w * 3) - 3) = *(iptr + (w * 3) - 3) + error * 3/16; 
            }

            if (j == h - 1 && k != w - 1) {
                *(iptr + 3) = *(iptr + 3) + error * 7/16;
            }

            else if (j != h - 1 && k != w - 1){
                *(iptr + 3) = *(iptr + 3) + error * 7/16;
                *(iptr + (w * 3) - 3) = *(iptr + (w * 3) - 3) + error * 3/16; 
                *(iptr + (w * 3) + 3) = *(iptr + (w * 3) + 3) + error * 1/16; 
                *(iptr + (w * 3)) = *(iptr + (w * 3)) + error * 5/16; 
            }
            iptr+=3; 
        }
    }

    for (int i = 0; i < w * h * 3; i++) {
        ucdata[i] = (char)idata[i];
    }

    delete idata;
}

void do_rgb_dither(unsigned char *ucdata, int h, int w)
{
    int *idata = new int[w * h *3]; 
    int *iptr = idata;

    for (int i = 0; i < w * h * 3; i++) {
        idata[i] = (int)ucdata[i];
    }

    int RGBcount = 0;

    while (RGBcount < 3) {
        iptr = idata + RGBcount;

        for (int j = 0; j < h; j++) {
            for (int k = 0; k < w; k++) {
                int error;
                int newpixel;    
                int oldpixel = *iptr;

                if (oldpixel > 128){
                    newpixel = 255;
                    error = oldpixel - 255;
                }

                else { 
                    newpixel = 0;
                    error = oldpixel;
                }

                *iptr = newpixel;

                if (k == 0 && j != h - 1) {
                    *(iptr + 3) = *(iptr + 3) + error * 7/16;
                    *(iptr + (w * 3) + 3) = *(iptr + (w * 3) + 3) + error * 1/16; 
                    *(iptr + (w * 3)) = *(iptr + (w * 3)) + error * 5/16; 
                }

                if (k == w - 1 && j != h - 1) {
                    *(iptr + (w * 3)) = *(iptr + (w * 3)) + error * 5/16; 
                    *(iptr + (w * 3) - 3) = *(iptr + (w * 3) - 3) + error * 3/16; 
                }

                if (j == h - 1 && k != w - 1) {
                    *(iptr + 3) = *(iptr + 3) + error * 7/16;
                }

                else if (j != h - 1 && k != w - 1){
                    *(iptr + 3) = *(iptr + 3) + error * 7/16;
                    *(iptr + (w * 3) - 3) = *(iptr + (w * 3) - 3) + error * 3/16; 
                    *(iptr + (w * 3) + 3) = *(iptr + (w * 3) + 3) + error * 1/16; 
                    *(iptr + (w * 3)) = *(iptr + (w * 3)) + error * 5/16; 
                }
                iptr+=3; 
            }
        }
        RGBcount++;
    }

    for (int i = 0; i < w * h * 3; i++) {
        ucdata[i] = (char)idata[i];
    }

    delete idata;
}

void show_image()
{
    //Show image on the screen.
    //
    int i, j, red, green, blue, avg;
    Ppmimage *image = ppm6GetImage(g.imageName[g.imageNum]);
    int w = image->width;
    int h = image->height;
    unsigned char *p, *ptr = (unsigned char *)image->data;
    //make sure image fits in window.
    check_image_fit(w, h);
    //
    if (g.grayscale) {
        //make image gray-scale now
        int col;
        p = ptr;
        for (i=0; i<h; i++) {
            for (j=0; j<w; j++) {
                //
                //Please add some code here...
                //
                //find the average of the 3 color components
                //red   = *(p+0)
                //green = *(p+1)
                //blue  = *(p+2)
                red = *(p+0);
                green = *(p+1);
                blue = *(p+2);

                //avg = (red+green+blue) / 3
                avg = (red+green+blue) / 3;

                //Put gray color into pixel
                //*(p+0) = avg;
                //*(p+1) = avg;
                //*(p+2) = avg;
                *(p+0) = avg;
                *(p+1) = avg;
                *(p+2) = avg;

                //increment the pointer here
                p+=3;
            }
        }
        if (g.dither) {
            floydSteinbergDither(ptr, h, w);
        }
    }

    if (g.rgbDither) {
        do_rgb_dither(ptr, h, w);
    }

    if (g.rotate == 1) {
        p = ptr;
        for (int i = h; i > 0; i--) {
            for (int j = 0; j < w; j++) {
                set_color_3i(*p, *(p+1), *(p+2));
                XDrawPoint(dpy, win, gc, g.menuWidth+i, j);
                p += 3;
            }
        }
    }

    else if (g.rotate == 2) {
        p = ptr;
        for (int i = h; i > 0; i--) {
            for (int j = w; j > 0; j--) {
                set_color_3i(*p, *(p+1), *(p+2));
                XDrawPoint(dpy, win, gc, g.menuWidth+j, i);
                p += 3;
            }
        }
    }

    else if (g.rotate == 3) {
        p = ptr;
        for (int i = 0; i < h; i++) {
            for (int j = w; j > 0; j--) {
                set_color_3i(*p, *(p+1), *(p+2));
                XDrawPoint(dpy, win, gc, g.menuWidth+i, j);
                p += 3;
            }
        }
    }

    //display the image data on screen
    else {
        p = ptr;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                set_color_3i(*p, *(p+1), *(p+2));
                XDrawPoint(dpy, win, gc, g.menuWidth+j, i);
                p += 3;
            }
        }
    }
    //finish
    ppm6CleanupImage(image);
    g.draw=0;
}


void render()
{
    if (g.draw)
        show_image();
    show_menu();
}



