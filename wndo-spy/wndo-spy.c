// wndo-spy
//
//  Author: Steve Cooper <steve@wijjo.com>
//  Based on xev - original license supported and preserved below
//

/* $XConsortium: xev.c,v 1.15 94/04/17 20:45:20 keith Exp $ */
/*

Copyright (c) 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/
/* $XFree86: xc/programs/xev/xev.c,v 1.13 2003/10/24 20:38:17 tsi Exp $ */

/*
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/Xlocale.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>
#include <X11/extensions/XTest.h>

const char *Yes = "YES";
const char *No = "NO";
const char *Unknown = "unknown";

// One minute timer and counter used for longer period timers
static time_t timerNext   = 0;
static long   timerCount  = 0;
const  int    timerPeriod = 60;

static int pidsChk[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int maxPidsChk = sizeof(pidsChk) / sizeof(int);

const char *nameProg;
Display *dpy;
Window root;
int screen;
Bool verbose;

static char pathWndo[1024] = "wndo";

XIC xic = NULL;

static Atom atom_net_current_desktop;

static void sendWndoEvent(const char* event)
{
    int iSpawn;
    for (iSpawn = 0; iSpawn < maxPidsChk; iSpawn++)
        if (pidsChk[iSpawn] == 0)
            break;
    if (iSpawn == maxPidsChk)
    {
        fprintf(stderr, "Too many spawned tasks - skipping \"wndo events.handle %s\"\n", event);
        return;
    }

    int pid = fork();
    if (pid == 0)
    {
        printf("wndo-spy: wndo events.handle %s\n", event);
        execlp(pathWndo, pathWndo, "events.handle", event, NULL);
        fprintf(stderr, "Failed to run \"wndo event %s\"\n", event);
        _exit(1);
    }

    // Watch this pid until it exits (non-blocking in main event loop).
    // Prevents zombies.
    pidsChk[iSpawn] = pid;
}

static Bool getCardinalProperty(Window w, Atom atom, long length, long* values)
{
    Atom actualType;
    int actualFormat;
    unsigned long actualLength, remaining;
    unsigned char *data;
    XGetWindowProperty(dpy, w, atom, 0L, length, False, XA_CARDINAL,
                       &actualType, &actualFormat, &actualLength, &remaining, &data);
    if (data)
    {
        if (values != NULL)
        {
            long* pData = (long*)(data);
            int i;
            for (i = 0; i < length; i++)
            {
                if (i < actualLength)
                    values[i] = pData[i];
                else
                    values[i] = 0;
            }
        }
        XFree(data);
        return True;
    }
    else
        return False;
}

static void prologue(XEvent *eventp, char *event_name)
{
    XAnyEvent *e = (XAnyEvent *) eventp;

    if (verbose) {
        printf ("\n%s event, serial %ld, synthetic %s, window 0x%lx,\n",
                event_name, e->serial, e->send_event ? Yes : No, e->window);
    }
}

static void do_FocusIn(XEvent *eventp)
{
    XFocusChangeEvent *e = (XFocusChangeEvent *) eventp;
    char *mode, *detail;
    char dmode[10], ddetail[10];

    switch (e->mode) {
      case NotifyNormal:  mode = "NotifyNormal"; break;
      case NotifyGrab:  mode = "NotifyGrab"; break;
      case NotifyUngrab:  mode = "NotifyUngrab"; break;
      case NotifyWhileGrabbed:  mode = "NotifyWhileGrabbed"; break;
      default:  mode = dmode, sprintf (dmode, "%u", e->mode); break;
    }

    switch (e->detail) {
      case NotifyAncestor:  detail = "NotifyAncestor"; break;
      case NotifyVirtual:  detail = "NotifyVirtual"; break;
      case NotifyInferior:  detail = "NotifyInferior"; break;
      case NotifyNonlinear:  detail = "NotifyNonlinear"; break;
      case NotifyNonlinearVirtual:  detail = "NotifyNonlinearVirtual"; break;
      case NotifyPointer:  detail = "NotifyPointer"; break;
      case NotifyPointerRoot:  detail = "NotifyPointerRoot"; break;
      case NotifyDetailNone:  detail = "NotifyDetailNone"; break;
      default:  detail = ddetail; sprintf (ddetail, "%u", e->detail); break;
    }

    if (verbose) {
        printf ("    mode %s, detail %s\n", mode, detail);
    }
}

static void do_FocusOut(XEvent *eventp)
{
    do_FocusIn(eventp);                /* since it has same information */
}

static void do_VisibilityNotify(XEvent *eventp)
{
    XVisibilityEvent *e = (XVisibilityEvent *) eventp;
    char *v;
    char vdummy[10];

    switch (e->state) {
      case VisibilityUnobscured:  v = "VisibilityUnobscured"; break;
      case VisibilityPartiallyObscured:  v = "VisibilityPartiallyObscured"; break;
      case VisibilityFullyObscured:  v = "VisibilityFullyObscured"; break;
      default:  v = vdummy; sprintf (vdummy, "%d", e->state); break;
    }

    if (verbose) {
        printf ("    state %s\n", v);
    }
}

static void do_CreateNotify(XEvent *eventp)
{
    XCreateWindowEvent *e = (XCreateWindowEvent *) eventp;

    if (verbose) {
        printf ("    parent 0x%lx, window 0x%lx, (%d,%d), width %d, height %d\n",
                e->parent, e->window, e->x, e->y, e->width, e->height);
        printf ("border_width %d, override %s\n",
                e->border_width, e->override_redirect ? Yes : No);
    }
}

static void do_DestroyNotify(XEvent *eventp)
{
    XDestroyWindowEvent *e = (XDestroyWindowEvent *) eventp;

    if (verbose) {
        printf ("    event 0x%lx, window 0x%lx\n", e->event, e->window);
    }
}

static void do_UnmapNotify(XEvent *eventp)
{
    XUnmapEvent *e = (XUnmapEvent *) eventp;

    if (verbose) {
        printf ("    event 0x%lx, window 0x%lx, from_configure %s\n",
                e->event, e->window, e->from_configure ? Yes : No);
    }
}

static void do_MapNotify(XEvent *eventp)
{
    XMapEvent *e = (XMapEvent *) eventp;

    if (verbose) {
        printf ("    event 0x%lx, window 0x%lx, override %s\n",
                e->event, e->window, e->override_redirect ? Yes : No);
    }
}

static void do_MapRequest(XEvent *eventp)
{
    XMapRequestEvent *e = (XMapRequestEvent *) eventp;

    if (verbose) {
        printf ("    parent 0x%lx, window 0x%lx\n", e->parent, e->window);
    }
}

static void do_ReparentNotify(XEvent *eventp)
{
    XReparentEvent *e = (XReparentEvent *) eventp;

    if (verbose) {
        printf ("    event 0x%lx, window 0x%lx, parent 0x%lx,\n",
                e->event, e->window, e->parent);
        printf ("    (%d,%d), override %s\n", e->x, e->y,
                e->override_redirect ? Yes : No);
    }
}

static void do_ConfigureNotify(XEvent *eventp)
{
    XConfigureEvent *e = (XConfigureEvent *) eventp;

    if (verbose) {
        printf ("    event 0x%lx, window 0x%lx, (%d,%d), width %d, height %d,\n",
                e->event, e->window, e->x, e->y, e->width, e->height);
        printf ("    border_width %d, above 0x%lx, override %s\n",
                e->border_width, e->above, e->override_redirect ? Yes : No);
    }
}

static void do_ConfigureRequest(XEvent *eventp)
{
    XConfigureRequestEvent *e = (XConfigureRequestEvent *) eventp;
    char *detail;
    char ddummy[10];

    switch (e->detail) {
      case Above:  detail = "Above";  break;
      case Below:  detail = "Below";  break;
      case TopIf:  detail = "TopIf";  break;
      case BottomIf:  detail = "BottomIf"; break;
      case Opposite:  detail = "Opposite"; break;
      default:  detail = ddummy; sprintf (ddummy, "%d", e->detail); break;
    }

    if (verbose) {
        printf ("    parent 0x%lx, window 0x%lx, (%d,%d), width %d, height %d,\n",
                e->parent, e->window, e->x, e->y, e->width, e->height);
        printf ("    border_width %d, above 0x%lx, detail %s, value 0x%lx\n",
                e->border_width, e->above, detail, e->value_mask);
    }
}

static void do_GravityNotify(XEvent *eventp)
{
    XGravityEvent *e = (XGravityEvent *) eventp;

    if (verbose) {
        printf ("    event 0x%lx, window 0x%lx, (%d,%d)\n",
                e->event, e->window, e->x, e->y);
    }
}

static void do_ResizeRequest(XEvent *eventp)
{
    XResizeRequestEvent *e = (XResizeRequestEvent *) eventp;

    if (verbose) {
        printf ("    width %d, height %d\n", e->width, e->height);
    }
}

static void do_CirculateNotify(XEvent *eventp)
{
    XCirculateEvent *e = (XCirculateEvent *) eventp;
    char *p;
    char pdummy[10];

    switch (e->place) {
      case PlaceOnTop:  p = "PlaceOnTop"; break;
      case PlaceOnBottom:  p = "PlaceOnBottom"; break;
      default:  p = pdummy; sprintf (pdummy, "%d", e->place); break;
    }

    if (verbose) {
        printf ("    event 0x%lx, window 0x%lx, place %s\n",
                e->event, e->window, p);
    }
}

static void do_CirculateRequest(XEvent *eventp)
{
    XCirculateRequestEvent *e = (XCirculateRequestEvent *) eventp;
    char *p;
    char pdummy[10];

    switch (e->place) {
      case PlaceOnTop:  p = "PlaceOnTop"; break;
      case PlaceOnBottom:  p = "PlaceOnBottom"; break;
      default:  p = pdummy; sprintf (pdummy, "%d", e->place); break;
    }

    if (verbose) {
        printf ("    parent 0x%lx, window 0x%lx, place %s\n",
                e->parent, e->window, p);
    }
}

static void sendWorkspaceEvents(Atom atom)
{
    // Send event that indicates any workspace change
    sendWndoEvent("workspace:switch");
    // Send event for specific workspace
    long n;
    if (getCardinalProperty(root, atom, 1, &n))
    {
        char tmp[50];
        sprintf(tmp, "workspace:%d", n+1);
        sendWndoEvent(tmp);
    }
}

static void do_PropertyNotify(XEvent *eventp)
{
    XPropertyEvent* e = (XPropertyEvent*)eventp;
    char* aname = XGetAtomName(dpy, e->atom);
    char* s;
    char sdummy[10];

    switch (e->state)
    {
      case PropertyNewValue:
        s = "PropertyNewValue";
        if (strcmp(aname, "_NET_CURRENT_DESKTOP") == 0)
            sendWorkspaceEvents(e->atom);
        break;

      case PropertyDelete: 
        s = "PropertyDelete";
        break;

      default: 
        s = sdummy;
        sprintf(sdummy, "%d", e->state);
        break;
    }

    if (verbose) {
        printf("    atom 0x%lx (%s), time %lu, state %s\n",
               e->atom, aname ? aname : Unknown, e->time,  s);
    }

    if (aname)
        XFree(aname);
}

static void do_SelectionClear(XEvent *eventp)
{
    XSelectionClearEvent *e = (XSelectionClearEvent *) eventp;
    char *sname = XGetAtomName (dpy, e->selection);

    if (verbose) {
        printf ("    selection 0x%lx (%s), time %lu\n",
                e->selection, sname ? sname : Unknown, e->time);
    }

    if (sname) XFree (sname);
}

static void do_SelectionRequest(XEvent *eventp)
{
    XSelectionRequestEvent *e = (XSelectionRequestEvent *) eventp;
    char *sname = XGetAtomName (dpy, e->selection);
    char *tname = XGetAtomName (dpy, e->target);
    char *pname = XGetAtomName (dpy, e->property);

    if (verbose) {
        printf ("    owner 0x%lx, requestor 0x%lx, selection 0x%lx (%s),\n",
                e->owner, e->requestor, e->selection, sname ? sname : Unknown);
        printf ("    target 0x%lx (%s), property 0x%lx (%s), time %lu\n",
                e->target, tname ? tname : Unknown, e->property,
                pname ? pname : Unknown, e->time);
    }

    if (sname) XFree (sname);
    if (tname) XFree (tname);
    if (pname) XFree (pname);
}

static void do_SelectionNotify(XEvent *eventp)
{
    XSelectionEvent *e = (XSelectionEvent *) eventp;
    char *sname = XGetAtomName (dpy, e->selection);
    char *tname = XGetAtomName (dpy, e->target);
    char *pname = XGetAtomName (dpy, e->property);

    if (verbose) {
        printf ("    selection 0x%lx (%s), target 0x%lx (%s),\n",
                e->selection, sname ? sname : Unknown, e->target,
                tname ? tname : Unknown);
        printf ("    property 0x%lx (%s), time %lu\n",
                e->property, pname ? pname : Unknown, e->time);
    }

    if (sname) XFree (sname);
    if (tname) XFree (tname);
    if (pname) XFree (pname);
}

static void do_ClientMessage(XEvent *eventp)
{
    XClientMessageEvent* e = (XClientMessageEvent*)eventp;
    char *mname = XGetAtomName(dpy, e->message_type);

    if (verbose)
    {
        printf("    message_type 0x%lx (%s), format %d\n",
               e->message_type, mname ? mname : Unknown, e->format);
    }

    if (mname) XFree(mname);
}

static const char* formatKey(XKeyEvent* e, char* buf)
{
    buf[0] = '\0';
    // Wndo presumes alphabetical order
    if (e->state & Mod1Mask   ) strcat(buf, "Alt+"    );
    if (e->state & ControlMask) strcat(buf, "Control+");
    if (e->state & Mod2Mask   ) strcat(buf, "Mod2+"   );
    if (e->state & Mod3Mask   ) strcat(buf, "Mod3+"   );
    if (e->state & Mod5Mask   ) strcat(buf, "Mod5+"   );
    if (e->state & ShiftMask  ) strcat(buf, "Shift+"  );
    if (e->state & Mod4Mask   ) strcat(buf, "Super+"  );
    KeySym keysym = XKeycodeToKeysym(dpy, e->keycode, 0);
    strcat(buf, XKeysymToString(keysym));
    return buf;
}

static unsigned int keyPending = 0;
static char skeyPending[200] = "";

static void do_KeyPress(XEvent *eventp)
{
    XKeyEvent* e = (XKeyEvent*)eventp;
    keyPending = e->keycode;
    formatKey(e, skeyPending);
    if (verbose)
        printf("    KeyPress: %s\n", skeyPending);
}

static void do_KeyRelease(XEvent *eventp)
{
    XKeyEvent* e = (XKeyEvent*)eventp;
    Bool accept = (keyPending != 0 && e->keycode == keyPending);
    char buf[200];
    formatKey(e, buf);
    if (accept)
    {
        char buf2[250];
        sprintf(buf2, "key:%s", skeyPending);
        sendWndoEvent(buf2);
    }
    if (verbose)
        printf("    KeyRelease(%s): %s\n", accept ? "accept" : "ignore", formatKey(e, buf));
}

static void do_ButtonPress(XEvent *eventp)
{
    XButtonEvent* e = (XButtonEvent*)eventp;
    if (verbose)
    {
        printf("    ButtonPress: state=0x%08x button=0x%08x\n", e->state, e->button);
    }
}

static void do_ButtonRelease(XEvent *eventp)
{
    XButtonEvent* e = (XButtonEvent*)eventp;
    if (verbose)
    {
        printf("    ButtonRelease: state=0x%08x button=0x%08x\n", e->state, e->button);
    }
}

static void do_MotionNotify(XEvent *eventp)
{
    //TODO:
}

static void usage(void)
{
    static const char* msg[] =
    {
        "-d DISPLAY  X server to contact",
        "-p PATH     Path to wndo (e.g. for testing)",
        "-v          Display verbose output",
        NULL
    };
    const char** cpp;

    fprintf(stderr, "\nUsage:  %s [OPTION ...]\n", nameProg);
    fprintf(stderr, "\n  OPTION\n");

    for (cpp = msg; *cpp; cpp++)
        fprintf(stderr, "    %s\n", *cpp);
    fprintf(stderr, "\n");

    exit(1);
}

static void grabKeys(const char* path)
{
    // First drop existing grabs
    printf("Grabbing keys...\n");
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    FILE* f = fopen(path, "r");
    if (f != NULL)
    {
        char buf[1024];
        char* s = fgets(buf, sizeof(buf) / sizeof(char), f);
        while (s != NULL)
        {
            char* s2 = s + strspn(s, " \t");
            if (s2[0] == '[' && strncasecmp(s2+1, "key:", 4) == 0)
            {
                char* skey  = s2 + 5;
                char* skey2 = strchr(skey, ']');
                if (skey2 != NULL)
                {
                    *skey2 = '\0';
                    printf("Grabbing key \"%s\"\n", skey);
                    unsigned int mod = 0;
                    unsigned int key = 0;
                    char* tok;
                    for (tok = strtok(skey, "+"); tok != NULL; tok = strtok(NULL, "+"))
                    {
                        if (strcmp(tok, "Shift") == 0)
                            mod |= ShiftMask;
                        else if (strcmp(tok, "Control") == 0)
                            mod |= ControlMask;
                        else if (strcmp(tok, "Mod1") == 0 || strcmp(tok, "Alt") == 0)
                            mod |= Mod1Mask;
                        else if (strcmp(tok, "Mod2") == 0)
                            mod |= Mod2Mask;
                        else if (strcmp(tok, "Mod3") == 0)
                            mod |= Mod3Mask;
                        else if (strcmp(tok, "Mod4") == 0 || strcmp(tok, "Super") == 0)
                            mod |= Mod4Mask;
                        else if (strcmp(tok, "Mod5") == 0)
                            mod |= Mod5Mask;
                        else
                        {
                            KeySym keysym = XStringToKeysym(tok);
                            if (keysym != NoSymbol)
                                key = XKeysymToKeycode(dpy, keysym);
                            else
                                key = 0;
                            if (key == 0)
                            {
                                fprintf(stderr, "Skipping bad key symbol \"%s\"\n", tok);
                                break;
                            }
                        }
                    }
                    if (key != 0)
                        XGrabKey(dpy, key, mod, root, False, GrabModeAsync, GrabModeAsync);
                }
            }
            s = fgets(buf, sizeof(buf) / sizeof(char), f);
        }
        fclose(f);
    }
}

static int* nullXerror(Display* d, XErrorEvent* e)
{
    // Display warning only once
    static Bool already = False;
    if (already)
        return NULL;
      already = True;
    fprintf(stderr, "\n***********************************************\n");
    fprintf(stderr,   " WARNING\n");
    fprintf(stderr,   " An X error has occurred.\n");
    fprintf(stderr,   " E.g. One or more key bindings may have failed.\n");
    fprintf(stderr, "\n************************************************\n");
    return NULL;
}

int main(int argc, char **argv)
{
    char *nameDsp = NULL;
    Window w = 0;
    XSetWindowAttributes attr;
    XWindowAttributes wattr;
    XIM xim;
    XIMStyles *xim_styles;
    XIMStyle xim_style = 0;
    char *modifiers;
    char *imvalret;

    nameProg = argv[0];
    char* slash = strrchr(nameProg, '/');
    if (slash != NULL)
        nameProg = slash + 1;

    // Start the timer(s)
    timerNext  = time(NULL) + timerPeriod;
    timerCount = 0;

    if (setlocale(LC_ALL,"") == NULL)
        fprintf(stderr, "%s: warning: could not set default locale\n", nameProg);

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
            if (strchr(arg, 'p') != NULL)
            {
                if (++i >= argc)
                    usage();
                strcpy(pathWndo, argv[i]);
                printf("Setting wndo path to \"%s\"\n", pathWndo);
            }
            if (strchr(arg, 'v') != NULL)
                verbose = True;
            if (strspn(arg+1, "dpv") != strlen(arg+1))
                usage();
        }
        else
            usage();
    }

    dpy = XOpenDisplay(nameDsp);
    if (!dpy)
    {
        fprintf(stderr, "%s:  unable to open display '%s'\n", nameProg, XDisplayName(nameDsp));
        exit(1);
    }

    /* we're testing the default input method */
    modifiers = XSetLocaleModifiers("@im=none");
    if (modifiers == NULL)
        fprintf(stderr, "%s:  XSetLocaleModifiers failed\n", nameProg);

    xim = XOpenIM(dpy, NULL, NULL, NULL);
    if (xim == NULL)
        fprintf (stderr, "%s:  XOpenIM failed\n", nameProg);

    if (xim)
    {
        imvalret = XGetIMValues (xim, XNQueryInputStyle, &xim_styles, NULL);
        if (imvalret != NULL || xim_styles == NULL)
            fprintf(stderr, "%s:  input method doesn't support any styles\n", nameProg);

        if (xim_styles)
        {
            xim_style = 0;
            for (i = 0;  i < xim_styles->count_styles;  i++)
            {
                if (xim_styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing))
                {
                    xim_style = xim_styles->supported_styles[i];
                    break;
                }
            }

            if (xim_style == 0)
                fprintf(stderr, "%s: input method doesn't support style\n", nameProg);
            XFree(xim_styles);
        }
    }

    screen = DefaultScreen(dpy);

    // Set up necessary atoms
    atom_net_current_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);

//X    attr.event_mask = ExposureMask | VisibilityChangeMask |
//X                      StructureNotifyMask | /* ResizeRedirectMask | */
//X                      SubstructureNotifyMask | SubstructureRedirectMask |
//X                      FocusChangeMask | PropertyChangeMask;

    attr.event_mask =
          PropertyChangeMask
        | KeyPressMask
        | KeyReleaseMask
        | ButtonPressMask
        | ButtonReleaseMask
        | LeaveWindowMask
        | PointerMotionMask
        | Button1MotionMask
        | Button2MotionMask
        | Button3MotionMask
        | Button4MotionMask
        | Button5MotionMask
        | ButtonMotionMask
        ;

    w = root = RootWindow(dpy, screen);
    XGetWindowAttributes(dpy, w, &wattr);
    if (wattr.all_event_masks & ButtonPressMask)
        attr.event_mask &= ~ButtonPressMask;
    attr.event_mask &= ~SubstructureRedirectMask;

    XSelectInput(dpy, w, attr.event_mask);

    if (xim && xim_style)
    {
        xic = XCreateIC(xim, XNInputStyle, xim_style, XNClientWindow, w, XNFocusWindow, w, NULL);
        if (xic == NULL)
            fprintf (stderr, "XCreateIC failed\n");
    }

    // Notify of current workspace
    if (atom_net_current_desktop != None)
        sendWorkspaceEvents(atom_net_current_desktop);

    // Check now and poll periodically for changes to events.conf
    const int dPollKeys = 10;
    time_t tPollKeys = time(NULL);
    time_t tEventsConf = 0;

    XSetErrorHandler((XErrorHandler)nullXerror);

    Bool done = False;
    while (!done)
    {
        XEvent event;

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
            while (nQueued > 0)
            {
                XNextEvent (dpy, &event);

                if (time(NULL) > tPollKeys)
                {
                    const char* home = getenv("HOME");
                    if (home != NULL)
                    {
                        char path[200];
                        sprintf(path, "%s/.wndo/events.conf", home);
                        struct stat statbuf;
                        if (stat(path, &statbuf) == 0)
                        {
                            if (statbuf.st_mtime != tEventsConf)
                            {
                                tEventsConf = statbuf.st_mtime;
                                grabKeys(path);
                            }
                        }
                    }
                    tPollKeys += dPollKeys;
                }

                switch (event.type) {
                  case FocusIn:
                    prologue(&event, "FocusIn");
                    do_FocusIn(&event);
                    break;
                  case FocusOut:
                    prologue(&event, "FocusOut");
                    do_FocusOut(&event);
                    break;
                  case VisibilityNotify:
                    prologue(&event, "VisibilityNotify");
                    do_VisibilityNotify(&event);
                    break;
                  case CreateNotify:
                    prologue(&event, "CreateNotify");
                    do_CreateNotify(&event);
                    break;
                  case DestroyNotify:
                    prologue(&event, "DestroyNotify");
                    do_DestroyNotify(&event);
                    break;
                  case UnmapNotify:
                    prologue(&event, "UnmapNotify");
                    do_UnmapNotify(&event);
                    break;
                  case MapNotify:
                    prologue(&event, "MapNotify");
                    do_MapNotify(&event);
                    break;
                  case MapRequest:
                    prologue(&event, "MapRequest");
                    do_MapRequest(&event);
                    break;
                  case ReparentNotify:
                    prologue(&event, "ReparentNotify");
                    do_ReparentNotify(&event);
                    break;
                  case ConfigureNotify:
                    prologue(&event, "ConfigureNotify");
                    do_ConfigureNotify(&event);
                    break;
                  case ConfigureRequest:
                    prologue(&event, "ConfigureRequest");
                    do_ConfigureRequest(&event);
                    break;
                  case GravityNotify:
                    prologue(&event, "GravityNotify");
                    do_GravityNotify(&event);
                    break;
                  case ResizeRequest:
                    prologue(&event, "ResizeRequest");
                    do_ResizeRequest(&event);
                    break;
                  case CirculateNotify:
                    prologue(&event, "CirculateNotify");
                    do_CirculateNotify(&event);
                    break;
                  case CirculateRequest:
                    prologue(&event, "CirculateRequest");
                    do_CirculateRequest(&event);
                    break;
                  case PropertyNotify:
                    prologue(&event, "PropertyNotify");
                    do_PropertyNotify(&event);
                    break;
                  case SelectionClear:
                    prologue(&event, "SelectionClear");
                    do_SelectionClear(&event);
                    break;
                  case SelectionRequest:
                    prologue(&event, "SelectionRequest");
                    do_SelectionRequest(&event);
                    break;
                  case SelectionNotify:
                    prologue(&event, "SelectionNotify");
                    do_SelectionNotify(&event);
                    break;
                  case ClientMessage:
                    prologue(&event, "ClientMessage");
                    do_ClientMessage(&event);
                    break;
                  case KeyPress:
                    prologue(&event, "KeyPress");
                    do_KeyPress(&event);
                    break;
                  case KeyRelease:
                    prologue(&event, "KeyRelease");
                    do_KeyRelease(&event);
                    break;
                  case ButtonPress:
                    prologue(&event, "ButtonPress");
                    do_ButtonPress(&event);
                    break;
                  case ButtonRelease:
                    prologue(&event, "ButtonRelease");
                    do_ButtonRelease(&event);
                    break;
                  case MotionNotify:
                    prologue(&event, "MotionNotify");
                    do_MotionNotify(&event);
                    break;
                  default:
                    //printf ("Unknown event type %d\n", event.type);
                    break;
                }
                nQueued = XPending(dpy);
            }
        }

        // Check timer(s)
        if (time(NULL) >= timerNext)
        {
            timerNext += timerPeriod;
            timerCount++;
            if (verbose)
                printf("TIMER *1*\n");
            sendWndoEvent("timer:1");
            if (timerCount % 5 == 0)
            {
                if (verbose)
                    printf("TIMER *5*\n");
                sendWndoEvent("timer:5");
            }
            if (timerCount % 20 == 0)
            {
                if (verbose)
                    printf("TIMER *20*\n");
                sendWndoEvent("timer:20");
            }
            if (timerCount % 60 == 0)
            {
                if (verbose)
                    printf("TIMER *60*\n");
                sendWndoEvent("timer:60");
            }
        }

        // Check for exited subprocesses to prevent zombies.
        int ip;
        for (ip = 0; ip < maxPidsChk; ip++)
            if (pidsChk[ip] != 0 && waitpid(pidsChk[ip], NULL, WNOHANG) == pidsChk[ip])
                    pidsChk[ip] = 0;    // Free up slot
    }

    XUngrabKey(dpy, AnyKey, AnyModifier, root);

    XCloseDisplay (dpy);
    return 0;
}

// vim: et: ts=4: sts=4: sw=4
