////////////////////////////////////////////////////////////////////////////////
// wndo-pick - a copy of xkill.c tweaked to just print the chosen window id
//
// Maintainer: Steve Cooper <steve@wijjo.com>
//
// Based on: xkill Copyright 1988, 1998  The Open Group
//           Author:  Jim Fulton, MIT X Consortium; Dana Chee, Bellcore
//
// Permission to use, copy, modify, distribute, and sell this software and its
// documentation for any purpose is hereby granted without fee, provided that
// the above copyright notice appear in all copies and that both that copyright
// notice and this permission notice appear in supporting documentation.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of The Open Group shall not be
// used in advertising or otherwise to promote the sale, use or other dealings
// in this Software without prior written authorization from The Open Group.
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>
#include <X11/Xlocale.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

static Display* dpy = NULL;
static char *nameProg;
static Bool verbose = False;

#define SelectButtonAny (-1)
#define SelectButtonFirst (-2)

static void pick_window(int screen, int button);
static void pick_key(int screen);
static const char* format_key(KeyCode kc, unsigned int mods, char* buf);
static int  get_button1(void);
static Bool wm_state_set(Window win);
static Bool wm_running(int screen);
static int  parse_button(char* s, int* buttonp);
static XID  wait_click_window(int screen, int button);
static Bool wait_key(int screen, KeyCode* kc, unsigned int* mods);
static void grab_keys(int screen);
static void ungrab_keys(int screen);

static KeySym keygrabs[] =
{
    XK_BackSpace,
    XK_Tab,
    XK_Linefeed,
    XK_Clear,
    XK_Return,
    XK_Escape,
    XK_Delete,
    XK_Home,
    XK_Left,
    XK_Up,
    XK_Right,
    XK_Down,
    XK_Prior,
    XK_Page_Up,
    XK_Next,
    XK_Page_Down,
    XK_End,
    XK_Begin,
    XK_KP_Space,
    XK_KP_Tab,
    XK_KP_Enter,
    XK_KP_F1,
    XK_KP_F2,
    XK_KP_F3,
    XK_KP_F4,
    XK_KP_Home,
    XK_KP_Left,
    XK_KP_Up,
    XK_KP_Right,
    XK_KP_Down,
    XK_KP_Prior,
    XK_KP_Page_Up,
    XK_KP_Next,
    XK_KP_Page_Down,
    XK_KP_End,
    XK_KP_Begin,
    XK_KP_Insert,
    XK_KP_Delete,
    XK_KP_Equal,
    XK_KP_Multiply,
    XK_KP_Add,
    XK_KP_Separator,
    XK_KP_Subtract,
    XK_KP_Decimal,
    XK_KP_Divide,
    XK_KP_0,
    XK_KP_1,
    XK_KP_2,
    XK_KP_3,
    XK_KP_4,
    XK_KP_5,
    XK_KP_6,
    XK_KP_7,
    XK_KP_8,
    XK_KP_9,
    XK_F1,
    XK_F2,
    XK_F3,
    XK_F4,
    XK_F5,
    XK_F6,
    XK_F7,
    XK_F8,
    XK_F9,
    XK_F10,
    XK_F11,
    XK_F12,
};
const size_t nkeygrabs = sizeof(keygrabs) / sizeof(KeySym);

static void cleanup_and_exit(int code)
{
    if (dpy)
        XCloseDisplay(dpy);
    exit(code);
}

static void usage(void)
{
    static char* options[] =
    {
        "-d DISPLAY  X server to contact",
        "-k          Wait for key instead of selecting window",
        "-v          Display verbose messages",
        NULL
    };

    char** cpp;
    fprintf(stderr, "\nUsage:  %s [OPTION ...]\n", nameProg);
    fprintf(stderr, "\n  OPTION\n");
    for (cpp = options; *cpp; cpp++)
        fprintf(stderr, "    %s\n", *cpp);
    fprintf(stderr, "\n");
    cleanup_and_exit(1);
}

static void error_exit(const char* fmt, ...)
{
    va_list ap;
    char msg2[512];
    va_start(ap, fmt);
    vsprintf(msg2, fmt, ap);
    fprintf(stderr, "%s: %s\n", nameProg, msg2);
    va_end(ap);
    cleanup_and_exit(1);
}

static void trace(const char* fmt, ...)
{
    va_list ap;
    char msg2[512];
    va_start(ap, fmt);
    vsprintf(msg2, fmt, ap);
    printf("%s: %s\n", nameProg, msg2);
    va_end(ap);
}

int main(int argc, char* argv[])
{
    char* nameDsp = NULL;           // name of server to contact
    Bool optKey = False;

    nameProg = argv[0];
    char* slash = strrchr(nameProg, '/');
    if (slash != NULL)
        nameProg = slash + 1;

    int i;
    for (i = 1; i < argc; i++)
    {
        char* arg = argv[i];

        if (arg[0] == '-')
        {
            if (strchr(arg, 'd') != NULL)
            {
                if (++i >= argc || argv[i][0] != ':')
                    usage();
                nameDsp = argv[i];
            }
            if (strchr(arg, 'k') != NULL)
                optKey = True;
            if (strchr(arg, 'v') != NULL)
                verbose = True;
            if (strspn(arg+1, "kvd") != strlen(arg+1))
                usage();
        }
        else
            usage();
    }

    dpy = XOpenDisplay(nameDsp);
    if (!dpy)
        error_exit("unable to open display \"%s\"", XDisplayName(nameDsp));

    int screen = DefaultScreen(dpy);

    int button = get_button1();

    if (optKey)
        pick_key(screen);
    else
        pick_window(screen, button);

    cleanup_and_exit(0);

    /*NOTREACHED*/
    return 0;
}

static const char* format_key(KeyCode kc, unsigned int mods, char* buf)
{
    buf[0] = '\0';
    if (mods & ShiftMask  ) strcat(buf, "Shift+"  );
    if (mods & ControlMask) strcat(buf, "Control+");
    if (mods & Mod1Mask   ) strcat(buf, "Alt+"    );
    if (mods & Mod2Mask   ) strcat(buf, "Mod2+"   );
    if (mods & Mod3Mask   ) strcat(buf, "Mod3+"   );
    if (mods & Mod4Mask   ) strcat(buf, "Super+"  );
    if (mods & Mod5Mask   ) strcat(buf, "Mod5+"   );
    KeySym keysym = XKeycodeToKeysym(dpy, kc, 0);
    strcat(buf, XKeysymToString(keysym));
    return buf;
}

// choose a key
static void pick_key(int screen)
{
    KeyCode kc;
    unsigned int mods;
    char buf[512];
    if (wait_key(screen, &kc, &mods))
        printf("%s\n", format_key(kc, mods, buf));
}

// choose a window
static void pick_window(int screen, int button)
{
    XID id = wait_click_window(screen, button);
    if (id)
    {
        if (id == RootWindow(dpy, screen))
            id = None;
        else
        {
            XID indicated = id = XmuClientWindow(dpy, indicated);
            if (id == indicated)
                // Try to not kill the window manager
                if (!wm_state_set(id) && wm_running(screen))
                    id = None;
        }
    }

    if (id != None)
    {
        printf("0x%lx\n", id);
        XSync(dpy, 0);
    }
}

// choose a window
static int get_button1(void)
{
    char* button_name = XGetDefault(dpy, nameProg, "Button");
    int button = -1;                    // button number or negative for all

    if (!button_name)
        button = SelectButtonFirst;
    else if (!parse_button(button_name, &button))
        error_exit("invalid button specification \"%s\"", button_name);

    if (button >= 0 || button == SelectButtonFirst)
    {
        unsigned char pointer_map[256];         // 8 bits of pointer num
        int count, j;
        unsigned int ub = (unsigned int) button;

        count = XGetPointerMapping(dpy, pointer_map, 256);
        if (count <= 0)
            error_exit("no pointer mapping");

        if (button >= 0)                        // check button
        {
            for (j = 0; j < count; j++)
                if (ub == (unsigned int) pointer_map[j]) break;
            if (j == count)
                error_exit("button %u not mapped", ub);
        }
        else                                    // get first entry
            button = (int)((unsigned int)pointer_map[0]);
    }

    return button;
}

// Wait for keyboard/mouse input and select window
static XID wait_click_window(int screen, int button)
{
    Window root = RootWindow(dpy, screen);

    Cursor cursor = XCreateFontCursor(dpy, XC_question_arrow);
    if (cursor == None)
        error_exit("unable to create selection cursor");

    XSync(dpy, 0);

    unsigned int mask = (ButtonPressMask | ButtonReleaseMask);
    if (XGrabPointer(dpy, root, False, mask, GrabModeSync, GrabModeAsync,
                          None, cursor, CurrentTime) != GrabSuccess)
        error_exit("unable to grab pointer");

    Window win = None;          // the window that got selected
    int buttonPressed = -1;     // button used to select window
    int pressed = 0;            // count of number of buttons pressed

    // from dsimple.c in xwininfo
    while (win == None || pressed != 0)
    {
        XEvent event;
        XAllowEvents(dpy, SyncPointer, CurrentTime);
        XWindowEvent(dpy, root, mask, &event);
        switch (event.type)
        {
          case ButtonPress:
            if (win == None)
            {
                buttonPressed = event.xbutton.button;
                win = (event.xbutton.subwindow != None ?  event.xbutton.subwindow : root);
            }
            pressed++;
            break;
          case ButtonRelease:
            if (pressed > 0)
                pressed--;
            break;
        }
    }

    XUngrabPointer(dpy, CurrentTime);
    XFreeCursor(dpy, cursor);
    XSync(dpy, 0);

    return ((button == -1 || buttonPressed == button) ? win : None);
}

static Bool wait_key(int screen, KeyCode* kc, unsigned int* mods)
{
    grab_keys(screen);

    KeyCode waitfor = 0;
    *kc = 0;
    Bool done = False;
    while (!done)
    {
        // Use select to make sure we check our timer(s) at least once a second
        fd_set rd;
        int fd = ConnectionNumber(dpy);
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&rd);
        FD_SET(fd, &rd);
        if (select(fd+1, &rd, NULL, NULL, &tv) > 0)
        {
            int nQueued = XPending(dpy);
            if (nQueued == 0)
                printf("Waiting for key...\n");
            while (nQueued > 0)
            {
                XEvent event;
                XNextEvent(dpy, &event);
                if (verbose)
                    trace("event.type=%d", event.type);
                switch (event.type)
                {
                  case KeyPress:
                    {
                        XKeyEvent* e = (XKeyEvent*)&event;
                        if (verbose)
                            trace("KeyPress 0x%x/0x%x", e->keycode, e->state);
                        if (waitfor == 0)
                        {
                            waitfor = e->keycode;
                            *mods = e->state;
                            if (verbose)
                                trace("wait for key 0x%x release...", waitfor);
                        }
                    }
                    break;
                  case KeyRelease:
                    {
                        XKeyEvent* e = (XKeyEvent*)&event;
                        if (verbose)
                            trace("KeyRelease 0x%x/0x%x", e->keycode, e->state);
                        if (e->keycode == waitfor)
                        {
                            *kc = waitfor;
                            if (verbose)
                                trace("got key 0x%x release!", waitfor);
                        }
                        done = True;
                    }
                    break;
                }
                if (!done)
                    nQueued = XPending(dpy);
                else
                    nQueued = 0;
            }
        }
    }

    ungrab_keys(screen);

    return (Bool)(*kc != 0);
}

static void grab_keys(int screen)
{
    // use default input method
    XIM xim = XOpenIM(dpy, NULL, NULL, NULL);
    if (xim == NULL)
        error_exit("XOpenIM failed");

    XIMStyle xim_style = 0;
    if (xim)
    {
        XIMStyles *xim_styles;
        char* imvalret = XGetIMValues (xim, XNQueryInputStyle, &xim_styles, NULL);
        if (imvalret != NULL || xim_styles == NULL)
            error_exit("input method doesn't support any styles");

        if (xim_styles)
        {
            int i;
            for (i = 0;  i < xim_styles->count_styles;  i++)
            {
                if (xim_styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing))
                {
                    xim_style = xim_styles->supported_styles[i];
                    break;
                }
            }
            if (xim_style == 0)
                error_exit("input method doesn't support the style we support");
            XFree(xim_styles);
        }
    }

    XSetWindowAttributes attr;
    attr.event_mask = (KeyPressMask | KeyReleaseMask);

    Window root = RootWindow(dpy, screen);
    Window w = root;
    XWindowAttributes wattr;
    XGetWindowAttributes(dpy, w, &wattr);

    XSelectInput(dpy, w, attr.event_mask);

    // Grab all supported keys
    KeySym ks;
    for (ks = 0; ks <= 255; ks++)
    {
        KeyCode kc = XKeysymToKeycode(dpy, ks);
        if (kc != 0)
        {
            XGrabKey(dpy, kc,         0, root, False, GrabModeAsync, GrabModeAsync);
            XGrabKey(dpy, kc, ShiftMask, root, False, GrabModeAsync, GrabModeAsync);
        }
    }
    int iks;
    for (iks = 0; iks < nkeygrabs; iks++)
    {
        KeyCode kc = XKeysymToKeycode(dpy, keygrabs[iks]);
        if (kc != 0)
        {
            XGrabKey(dpy, kc,         0, root, False, GrabModeAsync, GrabModeAsync);
            XGrabKey(dpy, kc, ShiftMask, root, False, GrabModeAsync, GrabModeAsync);
        }
    }

    XIC xic = NULL;
    if (xim && xim_style)
    {
        xic = XCreateIC(xim, XNInputStyle, xim_style, XNClientWindow, w, XNFocusWindow, w, NULL);
        if (xic == NULL)
            error_exit("XCreateIC failed");
    }
}

static void ungrab_keys(int screen)
{
    Window root = RootWindow(dpy, screen);
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
}

static int parse_button(char* s, int* buttonp)
{
    register char* cp;

    // lower case name
    for (cp = s; *cp; cp++)
    {
        if (isascii(*cp) && isupper(*cp))
        {
#ifdef _tolower
            *cp = _tolower(*cp);
#else
            *cp = tolower(*cp);
#endif
        }
    }

    if (strcmp(s, "any") == 0)
    {
        *buttonp = SelectButtonAny;
        return 1;
    }

    // check for non-numeric input
    for (cp = s; *cp; cp++)
        if (!(isascii(*cp) && isdigit(*cp)))
            return 0;  // bogus name

    *buttonp = atoi(s);
    return 1;
}

// Return True if the property WM_STATE is set on the window, otherwise False.
static Bool wm_state_set(Window win) 
{
    Atom wm_state;
    Atom actual_type;
    int success;
    int actual_format;
    unsigned long nitems, remaining;
    unsigned char* prop = NULL;

    wm_state = XInternAtom(dpy, "WM_STATE", True);
    if (wm_state == None)
        return False;
    success = XGetWindowProperty(dpy,
                                 win,
                                 wm_state,
                                 0L,
                                 0L,
                                 False,
                                 AnyPropertyType,
                                 &actual_type,
                                 &actual_format,
                                 &nitems,
                                 &remaining,
                                 &prop);
    if (prop)
        XFree((char*)prop);
    return (success == Success && actual_type != None && actual_format);
}

// Heuristic returns True if a window manager is running, otherwise False.
static Bool wm_running(int screen)
{
    XWindowAttributes xwa;
    Status status = XGetWindowAttributes(dpy, RootWindow(dpy, screen), &xwa);
    return (status &&
            ((xwa.all_event_masks & SubstructureRedirectMask) ||
             (xwa.all_event_masks & SubstructureNotifyMask)));
}
