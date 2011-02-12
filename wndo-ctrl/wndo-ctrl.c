/* license {{{ */
/* 

wndo-ctrl
A command line tool to interact with an EWMH/NetWM compatible X Window Manager.
Diluted/simplified version of wmctrl for use with wndo by:
    Steve Cooper <steve@wijjo.com>

wmctrl author, and current maintainer as of 5/25/07:
    Tomas Styblo <tripie@cpan.org>

Copyright (C) 2003

This program is free software which I release under the GNU General Public
License. You may redistribute and/or modify this program under the terms
of that license as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

To get a copy of the GNU General Puplic License,  write to the
Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
/* }}} */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>

#define MAX_PROPERTY_VALUE_LEN 4096

#define p_verbose(...) if (options.verbose) { \
    fprintf(stderr, __VA_ARGS__); \
}

const char* usageMsg[] =
{
"",
"Usage: wndo-ctrl ACTION [OPTIONS ...] ...",
"",
"  ACTION:",
"    -s DESK         Switch to specified desktop.",
"    -a WIN          Activate window by switching to its desktop and raising it.",
"    -c WIN          Close window gracefully.",
"    -R WIN          Move window to current desktop and activate it.",
"    -r WIN -t DESK  Move window to specified desktop.",
"    -r WIN -e GEOM  Resize and move window (GEOM described below).",
"    -h              Display help.",
"",
"  OPTION:",
"    -v              Be verbose. Useful for debugging.",
"",
"  Arguments:",
"    WIN             Window id in hex format \"0x...\".",
"    DESK            Desktop number starting from zero.",
"    GEOM            Geometry as X,Y,W,H.  Use -1 for unchanged values.",
"",
};

const size_t nUsageLines = sizeof(usageMsg) / sizeof(const char*);

/* declarations of static functions *//*{{{*/
static Bool wm_supports (Display *disp, const char *prop);
static int client_msg(Display *disp, Window win, char *msg, 
        unsigned long data0, unsigned long data1, 
        unsigned long data2, unsigned long data3,
        unsigned long data4);
static int switch_desktop (Display *disp);
static int action_window (Display *disp, Window win, char mode);
static int action_window_pid (Display *disp, char mode);
static int activate_window (Display *disp, Window win, 
        Bool switch_desktop);
static int close_window (Display *disp, Window win);
static int window_to_desktop (Display *disp, Window win, int desktop);
static char *get_property (Display *disp, Window win, 
        Atom xa_prop_type, char *prop_name, unsigned long *size);
static int window_move_resize (Display *disp, Window win, char *arg);

/*}}}*/
   
static struct {
    int verbose;
    char *param_window;
    char *param;
} options;

void usage(int retcode)
{
    int i;
    for (i = 0; i < nUsageLines; i++)
        printf("%s\n", usageMsg[i]);
    exit(retcode);
}

int main (int argc, char **argv) { /* {{{ */
    int opt;
    int action = 0;
    int ret = EXIT_SUCCESS;
    int missing_option = 1;
    Display *disp;

    memset(&options, 0, sizeof(options)); /* just for sure */
    
    /* necessary to make g_get_charset() and g_locale_*() work */
    setlocale(LC_ALL, "");
    
    /* make "--help" and "--version" work. I don't want to use
     * getopt_long for portability reasons */
    if (argc == 2 && argv[1]) {
        if (strcmp(argv[1], "--help") == 0) {
            usage(EXIT_SUCCESS);
        }
    }
   
    while ((opt = getopt(argc, argv, "vha:r:s:c:t:e:b:R:")) != -1) {
        missing_option = 0;
        switch (opt) {
            case 'v':
                options.verbose = 1;
                break;
            case 'a': case 'c': case 'R':
                options.param_window = optarg;
                action = opt;
                break;
            case 'r':
                options.param_window = optarg;
                break; 
            case 't': case 'e':
                options.param = optarg;
                action = opt;
                break;
            case 's':
                options.param = optarg;
                action = opt;
                break;
            case '?':
                return EXIT_FAILURE;
            default:
                action = opt;
        }
    }
   
    if (missing_option) {
        usage(EXIT_FAILURE);
    }
   
    if (! (disp = XOpenDisplay(NULL))) {
        fputs("Cannot open display.\n", stderr);
        return EXIT_FAILURE;
    }
   
    switch (action) {
        case 'h':
            usage(EXIT_SUCCESS);
            break;
        case 's':
            ret = switch_desktop(disp);
            break;
        case 'a': case 'c': case 'R': 
        case 't': case 'e': case 'N':
            if (! options.param_window) {
                fputs("No window was specified.\n", stderr);
                return EXIT_FAILURE;
            }
            ret = action_window_pid(disp, action);
            break;
    }
    
    XCloseDisplay(disp);
    return ret;
}
/* }}} */

static int client_msg(Display *disp, Window win, char *msg, /* {{{ */
        unsigned long data0, unsigned long data1, 
        unsigned long data2, unsigned long data3,
        unsigned long data4) {
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;
    
    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "Cannot send %s event.\n", msg);
        return EXIT_FAILURE;
    }
}/*}}}*/

static int switch_desktop (Display *disp) {/*{{{*/
    int target = -1;
    
    target = atoi(options.param); 
    if (target == -1) {
        fputs("Invalid desktop ID.\n", stderr);
        return EXIT_FAILURE;
    }
    
    return client_msg(disp, DefaultRootWindow(disp), "_NET_CURRENT_DESKTOP", 
            (unsigned long)target, 0, 0, 0, 0);
}/*}}}*/

static int window_to_desktop (Display *disp, Window win, int desktop) {/*{{{*/
    CARD32 *cur_desktop = NULL;
    Window root = DefaultRootWindow(disp);
   
    if (desktop == -1) {
        if (! (cur_desktop = (CARD32 *)get_property(disp, root,
                XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
            if (! (cur_desktop = (CARD32 *)get_property(disp, root,
                    XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
                fputs("Cannot get current desktop properties. "
                      "(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE property)"
                      "\n", stderr);
                return EXIT_FAILURE;
            }
        }
        desktop = *cur_desktop;
    }
    free(cur_desktop);

    return client_msg(disp, win, "_NET_WM_DESKTOP", (unsigned long)desktop,
            0, 0, 0, 0);
}/*}}}*/

static int activate_window (Display *disp, Window win, /* {{{ */
        Bool switch_desktop) {
    CARD32 *desktop;

    /* desktop ID */
    if ((desktop = (CARD32 *)get_property(disp, win,
            XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
        if ((desktop = (CARD32 *)get_property(disp, win,
                XA_CARDINAL, "_WIN_WORKSPACE", NULL)) == NULL) {
            p_verbose("Cannot find desktop ID of the window.\n");
        }
    }

    if (switch_desktop && desktop) {
        if (client_msg(disp, DefaultRootWindow(disp), 
                    "_NET_CURRENT_DESKTOP", 
                    *desktop, 0, 0, 0, 0) != EXIT_SUCCESS) {
            p_verbose("Cannot switch desktop.\n");
        }
        free(desktop);
    }

    client_msg(disp, win, "_NET_ACTIVE_WINDOW", 
            0, 0, 0, 0, 0);
    XMapRaised(disp, win);

    return EXIT_SUCCESS;
}/*}}}*/

static int close_window (Display *disp, Window win) {/*{{{*/
    return client_msg(disp, win, "_NET_CLOSE_WINDOW", 
            0, 0, 0, 0, 0);
}/*}}}*/

static Bool wm_supports (Display *disp, const char *prop) {/*{{{*/
    Atom xa_prop = XInternAtom(disp, prop, False);
    Atom *list;
    unsigned long size;
    int i;

    if (! (list = (Atom *)get_property(disp, DefaultRootWindow(disp),
            XA_ATOM, "_NET_SUPPORTED", &size))) {
        p_verbose("Cannot get _NET_SUPPORTED property.\n");
        return False;
    }

    for (i = 0; i < size / sizeof(Atom); i++) {
        if (list[i] == xa_prop) {
            free(list);
            return True;
        }
    }
    
    free(list);
    return False;
}/*}}}*/

static int window_move_resize (Display *disp, Window win, char *arg) {/*{{{*/
    signed long x, y, w, h;
    const char *argerr = "The -e option expects a list of comma separated integers: \"X,Y,width,height\"\n";
    
    if (!arg || strlen(arg) == 0) {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }

    if (sscanf(arg, "%ld,%ld,%ld,%ld", &x, &y, &w, &h) != 4) {
        fputs(argerr, stderr);
        return EXIT_FAILURE;
    }

    if ((w < 1 || h < 1) && (x >= 0 && y >= 0)) {
        XMoveWindow(disp, win, x, y);
    }
    else if ((x < 0 || y < 0) && (w >= 1 && h >= -1)) {
        XResizeWindow(disp, win, w, h);
    }
    else if (x >= 0 && y >= 0 && w >= 1 && h >= 1) {
        XMoveResizeWindow(disp, win, x, y, w, h);
    }
    return EXIT_SUCCESS;
}/*}}}*/

static int action_window (Display *disp, Window win, char mode) {/*{{{*/
    p_verbose("Using window: 0x%.8lx\n", win);
    switch (mode) {
        case 'a':
            return activate_window(disp, win, True);

        case 'c':
            return close_window(disp, win);

        case 'e':
            /* resize/move the window around the desktop => -r -e */
            return window_move_resize(disp, win, options.param);

        case 't':
            /* move the window to the specified desktop => -r -t */
            return window_to_desktop(disp, win, atoi(options.param));
        
        case 'R':
            /* move the window to the current desktop and activate it => -r */
            if (window_to_desktop(disp, win, -1) == EXIT_SUCCESS) {
                usleep(100000); /* 100 ms - make sure the WM has enough
                    time to move the window, before we activate it */
                return activate_window(disp, win, False);
            }
            else {
                return EXIT_FAILURE;
            }

        default:
            fprintf(stderr, "Unknown action: '%c'\n", mode);
            return EXIT_FAILURE;
    }
}/*}}}*/

static int action_window_pid (Display *disp, char mode) {/*{{{*/
    unsigned long wid;

    if (sscanf(options.param_window, "0x%lx", &wid) != 1 &&
            sscanf(options.param_window, "0X%lx", &wid) != 1 &&
            sscanf(options.param_window, "%lu", &wid) != 1) {
        fputs("Cannot convert argument to number.\n", stderr);
        return EXIT_FAILURE;
    }

    return action_window(disp, (Window)wid, mode);
}/*}}}*/

static char *get_property (Display *disp, Window win, /*{{{*/
        Atom xa_prop_type, char *prop_name, unsigned long *size) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    char *ret;
    
    xa_prop_name = XInternAtom(disp, prop_name, False);
    
    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     */
    if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,     
            &ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
        p_verbose("Cannot get %s property.\n", prop_name);
        return NULL;
    }
  
    if (xa_ret_type != xa_prop_type) {
        p_verbose("Invalid type of %s property.\n", prop_name);
        XFree(ret_prop);
        return NULL;
    }

    /* null terminate the result to make string handling easier */
    tmp_size = (ret_format / 8) * ret_nitems;
    ret = malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }
    
    XFree(ret_prop);
    return ret;
}/*}}}*/
