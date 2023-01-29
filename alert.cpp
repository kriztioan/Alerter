/**
 *  @file   alert.cpp
 *  @brief  Alerter
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-19
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include <dirent.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>
#include <strings.h>

#include <netdb.h>

#include <sys/errno.h>

#include <ctype.h>

#include <sys/time.h>
#include <time.h>

#include <math.h>

#include <stdarg.h>

#include "Audio.h"

#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/shape.h>
#include <X11/keysymdef.h>
#include <X11/xpm.h>

/* PROGRAM INFO */
#define VERSION "v4.1.1"

/* UPDATE INTERFACE */
#define BUFFLEN 64
#define UPDATEURL "christiaanboersma.ddns.net/software/alert_curr_version"

char *retrieveURL(const char *url, int &total_bytes_recv, short port = 80);

/* SPRITE INTERFACE */
typedef struct _XSprite {

  Pixmap pixmap, mask;

  XImage *hotspot;

  unsigned int xwidth, xheight, xframe, xframewidth, xframeheight, xposx, xposy;

} XSprite;

/* OUTPUT INTERFACE */
int tfprintf(FILE *restrict_stream, const char *restrict_format, ...);

/* IMAGES */
#include "xpm/defaultbackground.xpm"
#include "xpm/defaultcoffeesprite.xpm"
#include "xpm/defaultlunchsprite.xpm"
#include "xpm/defaultteasprite.xpm"
#include "xpm/icon.xpm"
#include "xpm/logo.xpm"

/* MOTIF */
#include <Xm/MwmUtil.h>
#include <Xm/XmAll.h>

Widget configwidget, messagewidget, messagetext, clientstext;

/* CONFIG INTERFACE */
#define CONFIGFILE ".alert"

typedef struct _ConfigStruct {

  char version[9], ip[16], nick[21], themedir[64], theme[64], sounddir[64],
      sound[64];

  unsigned int refresh, reconnect, port;

  int xposx, xposy, xconfigposx, xconfigposy, xmessageposx, xmessageposy,
      volume;

  bool raisealert, raisechat, playsounds;

} configStruct;

int XmListFiles(XmStringTable &xmstringtable, const char *dir,
                const char *filter);

void configGUIEventPos(Widget w, XtPointer clientdata, XEvent *event,
                       Boolean *continue_to_dispatch);
void configGUIip(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUInick(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIport(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIposx(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIposy(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIreconnect(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIthemedir(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUItheme(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIsounddir(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIsound(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIraisealert(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIraisechat(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIplaysounds(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIvolume(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIrefresh(_WidgetRec *w, XtPointer clientdata, void *cbs);
void configGUIclose(_WidgetRec *w, XtPointer clientdata, void *cbs);

/* THEME INTERFACE*/

typedef struct _Pair {

  char *key;
  char *val;
} Pair;

int PairCmp(const void *c1, const void *c2);

void XLoadTheme(const char *filename);
Pair *ReadThemeVarsFromFile(const char *file, int &nvar);
void FreeThemeVars(Pair *var, int nvar);
char *GetThemeVar(Pair *var, int nvar, const char *key);

/* MESSAGE INTERFACE */

typedef struct _Message {

  _Message *previous, *next;

  int expired;

  char *text;

} Message;

Message *AddMessage(Message *messages, const char *text, int expired);
Message *DelMessage(Message *messages, Message *message);
Message *FreeMessages(Message *messages);

void messageGUIstart(char *xapplicationname, char *xapplicationclass,
                     Pixmap xiconpixmap, Pixmap xiconpixmapmask);
void messageGUIsend(_WidgetRec *w, XtPointer clientdata, void *cbs);
void messageGUIclose(_WidgetRec *w, XtPointer clientdata, void *cbs);
void messageGUIEventPos(Widget w, XtPointer clientdata, XEvent *event,
                        Boolean *continue_to_dispatch);

char clients[254];

/* MAJOR XALERT STRUCT */
typedef struct _XAlert {

  char *trigger;

  struct _XSprite sprite;

  struct _Message *message;

  bool active;

} XAlert;

void XFreeAlert(Display *xdisplay, XAlert &xalert);

/* OTHER */
#define LOGFILE "alert.log"

#define int_p_NULL (int *)NULL

char *typestr(int type);

extern int errno;
extern int h_errno;

char *strtrim(char *str);

/* CONVINIENT GLOBALS */

Display *xdisplay;

Window xwindow;

XdbeSwapInfo xdbeswapinfo;

XtAppContext xtappcontext;

Pixmap drawclippixmap = None, textpixmap, textclippixmap, xbackgroundpixmap,
       xbackgroundpixmapmask;

GC gcdraw, gcset = NULL, gcclear = NULL;

XFontStruct *xfont = NULL;

int xwidth, xheight;

int connection;

Colormap xcolormap;

int textposx = 17, textposy = 67, textwidth = 158, textheight, textstep = 5;

XAlert *xalerts;
size_t nalerts;

configStruct config = {VERSION,     "127.0.0.1", "Name", "./themes/", "",
                       "./sounds/", "",          100,    180,         7777,
                       0,           0,           0,      0,           0,
                       0,           50,          true,   true,        false};

char *configfile = NULL;

struct timeval select_timer, frame_timer;

int sockfd = -1;

Audio audio;

/* PACKAGE INTERFACE */

typedef struct _PACKET {

  int type;

  time_t timestamp;

  char nick[21], msg[127], data[256];

} PACKET;

#define STANDARD 0
#define IM 1
#define SERVER 2

PACKET pckt_send;

/* CONNECTION INTERFACE */

#define CONNECTION_OPENED 0
#define CONNECTION_CLOSE 1
#define CONNECTION_NEW 2
#define CONNECTION_OPEN 3

/* PNG INTERFACE */
#include <png.h>

typedef struct _XRGBAPixmap {

  int width;
  int height;

  Pixmap pixmap;
  Pixmap mask;

} XRGBAPixmap;

XRGBAPixmap *XReadPNGPixmapFromFile(const char *filename);

/* Signal handler */
void sighandler(int sig);

int main(int argc, char *argv[], char **envp) {

  // Get the correct dirs

  char *logfile = NULL, *homedir = getenv("HOME");

  if (NULL != homedir) {
    size_t len = strlen(homedir);

    configfile = (char *)realloc(configfile,
                                 (strlen(CONFIGFILE) + len + 2) * sizeof(char));
    configfile = strcpy(configfile, homedir);
    configfile[len] = '/';
    strcpy(configfile + len + 1, CONFIGFILE);

    logfile =
        (char *)realloc(logfile, (strlen(LOGFILE) + len + 2) * sizeof(char));
    logfile = strcpy(logfile, homedir);
    logfile[len] = '/';
    strcpy(logfile + len + 1, LOGFILE);
  } else {
    configfile =
        (char *)realloc(configfile, (strlen(CONFIGFILE) + 1) * sizeof(char));
    configfile = strcpy(configfile, LOGFILE);

    logfile = (char *)realloc(logfile, (strlen(LOGFILE) + 1) * sizeof(char));
    logfile = strcpy(logfile, LOGFILE);
  }

  // Some redirections

  fclose(stdin);

  FILE *fp = NULL;
  if ((fp = fopen(logfile, "a")) == NULL) {
    tfprintf(stderr, "fopen : %s\n", strerror(errno));
    exit(1);
  }

  if (freopen(logfile, "a", stderr) == NULL) {
    tfprintf(stderr, "freopen : %s\n", strerror(errno));
    exit(1);
  }

  free(logfile);

  // Configuration
  if (access(configfile, R_OK) == 0) {

    tfprintf(fp, "READING config file : %s\n", configfile);

    FILE *fpc = fopen(configfile, "r");

    if (fpc == NULL)
      tfprintf(stdout, "fopen : %s\n", strerror(errno));
    else
      fread(&config, sizeof(configStruct), 1, fpc);

    fclose(fpc);
  } else
    tfprintf(stderr, "access : %s\n", strerror(errno));

  if (strcmp(VERSION, config.version) != 0) {
    tfprintf(
        stdout,
        "Incompatible configuration file, try removing your .alert file\n");
    exit(1);
  }

  // Some program variables

  char *xapplicationclass = "Alarm", *xapplicationname = basename(argv[0]),
       *xwindowtitle = "Alert", *xicontitle = "Alert - iconified";

  // Xt stuff

  XtToolkitInitialize();

  xtappcontext = XtCreateApplicationContext();

  xdisplay = XtOpenDisplay(xtappcontext, NULL, xapplicationname,
                           xapplicationclass, NULL, 0, &argc, argv);
  if (xdisplay == NULL) {
    tfprintf(stderr, "XtOpenDisplay\n");
    exit(1);
  }

  tfprintf(fp, "XLib running on %s\n", XDisplayName((char *)xdisplay));

  int xfd = ConnectionNumber(xdisplay);

  // Check double buffering and shape extension

  int xshapeevent, xshapeerror;
  if (!XShapeQueryExtension(xdisplay, &xshapeevent, &xshapeerror)) {
    tfprintf(stderr, "Shape extension NOT supported\n");
    exit(1);
  }

  int xdbe_major_version, xdbe_minor_version;
  if (XdbeQueryExtension(xdisplay, &xdbe_major_version, &xdbe_minor_version) ==
      0) {
    tfprintf(stderr, "Double buffering NOT supported\n");
    exit(1);
  } else
    tfprintf(fp, "Double buffering version %d.%d supported\n",
             xdbe_major_version, xdbe_minor_version);

  // Major Graphics Context
  XGCValues xgcvalues;
  unsigned long xgcvalues_mask = GCGraphicsExposures;

  xgcvalues.graphics_exposures = false;

  gcdraw = XCreateGC(xdisplay, DefaultRootWindow(xdisplay), xgcvalues_mask,
                     &xgcvalues);

  XSetGraphicsExposures(xdisplay, DefaultGC(xdisplay, DefaultScreen(xdisplay)),
                        false);

  // Color map

  xcolormap = DefaultColormap(xdisplay, DefaultScreen(xdisplay));

  // Initialise alerts

  nalerts = 3;
  xalerts = (XAlert *)calloc(nalerts, sizeof(struct _XAlert));

  char *triggers[] = {"TEA", "COFFEE", "LUNCH"};

  for (size_t i = 0; i < nalerts; i++) {
    xalerts[i].trigger =
        (char *)malloc((strlen(triggers[i]) + 1) * sizeof(char));
    strcpy(xalerts[i].trigger, triggers[i]);
  }

  // Set theme

  char *path = (char *)malloc(
      (strlen(config.themedir) + strlen(config.theme) + 1) * sizeof(char));

  path = strcpy(path, config.themedir);
  path = strcat(path, config.theme);

  XLoadTheme(path);

  free(path);

  // Init audio system

  if (config.playsounds && *config.sound != '\0') {
    char *path = NULL;
    path = (char *)malloc((strlen(config.sounddir) + strlen(config.sound) + 1) *
                          sizeof(char));
    strcpy(path, config.sounddir);
    path = strcat(path, config.sound);
    audio.loadSoundFromFile(path);
    free(path);
    audio.setVolume(config.volume);
  }

  // Some widgets

  Widget posxsc = NULL, posysc = NULL, themelist = NULL, soundlist = NULL;

  // Create main application window

  XSetWindowAttributes xwindowattributes;
  xwindowattributes.background_pixmap = xbackgroundpixmap;
  xwindowattributes.save_under = false;
  xwindowattributes.backing_store = NotUseful;
  xwindowattributes.override_redirect = false;
  xwindowattributes.event_mask = StructureNotifyMask | KeyPressMask |
                                 ExposureMask | ButtonPressMask |
                                 Button1MotionMask;
  Cursor xcursor = XCreateFontCursor(xdisplay, XC_draft_large);
  xwindowattributes.cursor = xcursor;
  xwindowattributes.do_not_propagate_mask = false;
  unsigned int xwindowmask = CWBackPixmap | CWSaveUnder | CWOverrideRedirect |
                             CWEventMask | CWDontPropagate | CWCursor |
                             CWBackingStore;

  xwindow = XCreateWindow(
      xdisplay, DefaultRootWindow(xdisplay), config.xposx, config.xposy, xwidth,
      xheight, 0, DefaultDepth(xdisplay, DefaultScreen(xdisplay)), InputOutput,
      DefaultVisual(xdisplay, DefaultScreen(xdisplay)), xwindowmask,
      &xwindowattributes);

  Atom wmProtocols = XInternAtom(xdisplay, "WM_PROTOCOLS", false),
       wmDeleteWindow = XInternAtom(xdisplay, "WM_DELETE_WINDOW", false),
       wmSaveState = XInternAtom(xdisplay, "WM_SAVE_YOURSELF", true),
       wmHints = XInternAtom(xdisplay, "_MOTIF_WM_HINTS", true),
       wmActiveWindow = XInternAtom(xdisplay, "_NET_ACTIVE_WINDOW;", false);

  Atom wmProtocolsOnWindow[] = {wmDeleteWindow, wmSaveState};

  MwmHints mwmHints = {MWM_HINTS_DECORATIONS, MWM_FUNC_MOVE, 0, 0, 0};

  if (wmHints != None) {
    tfprintf(fp, "WM accepts MOTIF WM hints\n");
    XChangeProperty(xdisplay, xwindow, wmHints, wmHints, 32, PropModeReplace,
                    (unsigned char *)&mwmHints,
                    sizeof(mwmHints) / sizeof(long));
  }
  wmHints = XInternAtom(xdisplay, "KWM_WIN_DECORATION", True);
  if (wmHints != None) {
    tfprintf(fp, "WM accepts KWM WM hints\n");
    long KWMHints = 0;
    XChangeProperty(xdisplay, xwindow, wmHints, wmHints, 32, PropModeReplace,
                    (unsigned char *)&KWMHints,
                    sizeof(KWMHints) / sizeof(long));
  }
  wmHints = XInternAtom(xdisplay, "_WIN_HINTS", True);
  if (wmHints != None) {
    tfprintf(fp, "WM accepts GNOME WM hints\n");
    long GNOMEHints = 0;
    XChangeProperty(xdisplay, xwindow, wmHints, wmHints, 32, PropModeReplace,
                    (unsigned char *)&GNOMEHints,
                    sizeof(GNOMEHints) / sizeof(long));
  }

  //  if(XSetWMProtocols(xdisplay, xwindow, &wmDeleteWindow, 1) == 0)
  //  tfprintf(stderr, "XSetWMProtocols\n");

  if (XSetWMProtocols(xdisplay, xwindow, wmProtocolsOnWindow, 2) == 0)
    tfprintf(stderr, "XSetWMProtocols\n");

  XClientMessageEvent xactivewindowevent;
  xactivewindowevent.type = ClientMessage;
  xactivewindowevent.message_type = wmActiveWindow;
  xactivewindowevent.format = 32;
  xactivewindowevent.data.l[0] = 2;
  xactivewindowevent.data.l[1] = CurrentTime;
  xactivewindowevent.data.l[2] = xwindow;

  XSizeHints xsizehints;
  xsizehints.flags = PPosition;
  xsizehints.x = config.xposx;
  xsizehints.y = config.xposy;

  XpmAttributes xpmattributes;
  xpmattributes.valuemask = XpmSize;

  Pixmap xiconpixmap, xiconpixmapmask;

  if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay), xpmicon,
                            &xiconpixmap, &xiconpixmapmask,
                            &xpmattributes) != 0) {
    tfprintf(stderr, "XCreatePixmapFromData\n");
    exit(1);
  }

  XWMHints xwmhints;
  xwmhints.flags = StateHint | InputHint | IconPixmapHint | IconMaskHint;
  xwmhints.initial_state = NormalState;
  xwmhints.input = true;
  xwmhints.icon_pixmap = xiconpixmap;
  xwmhints.icon_mask = xiconpixmapmask;

  XClassHint xclasshint;
  xclasshint.res_class = xapplicationclass;
  xclasshint.res_name = xapplicationname;

  XTextProperty xwindowname, xiconname;
  if (XStringListToTextProperty(&(xwindowtitle), 1, &xwindowname) == 0) {
    tfprintf(stderr, "XStringListToTextProperty\n");
    exit(1);
  }

  if (XStringListToTextProperty(&(xicontitle), 1, &xiconname) == 0) {
    tfprintf(stderr, "XStringListToTextProperty\n");
    exit(1);
  }

  XSetWMProperties(xdisplay, xwindow, &xwindowname, &xiconname, argv, argc,
                   &xsizehints, &xwmhints, &xclasshint);

  XFree(xwindowname.value);
  XFree(xiconname.value);

  // Enable double buffering

  XdbeBackBuffer xdbebackbuffer;

  xdbebackbuffer = XdbeAllocateBackBufferName(xdisplay, xwindow, XdbeUndefined);
  xdbeswapinfo.swap_window = xwindow;
  xdbeswapinfo.swap_action = XdbeBackground;

  textpixmap =
      XCreatePixmap(xdisplay, DefaultRootWindow(xdisplay), textwidth, xheight,
                    DefaultDepth(xdisplay, DefaultScreen(xdisplay)));
  textclippixmap = XCreatePixmap(xdisplay, DefaultRootWindow(xdisplay),
                                 textwidth, xheight, 1);

  // Create the Graphic Contexts

  XFontStruct *xlargefont = XLoadQueryFont(xdisplay, "*-demibold-i-*-24-*");
  if (xlargefont == NULL)
    tfprintf(stderr, "XLoadQueryFont : cannot load font\n");

  xgcvalues_mask |= GCFont;

  if (NULL != xfont)
    xgcvalues.font = xfont->fid;

  GC gcclip = XCreateGC(xdisplay, drawclippixmap, xgcvalues_mask, &xgcvalues);

  XSetFunction(xdisplay, gcdraw, GXcopy);

  XSetFunction(xdisplay, gcclip, GXor);

  // Clear text mask

  XFillRectangle(xdisplay, textclippixmap, gcclear, 0, 0, textwidth, xheight);

  // Apply shape mask

  XShapeCombineMask(xdisplay, xwindow, ShapeBounding, 0, 0,
                    xbackgroundpixmapmask, ShapeSet);
  XShapeCombineMask(xdisplay, xwindow, ShapeClip, 0, 0, xbackgroundpixmapmask,
                    ShapeSet);

  // Message system initialisation

  Message *messages = NULL, *messageconnect = NULL;

  int bytes_recv = 0;
  char *curr_ver = retrieveURL(UPDATEURL, bytes_recv);

  if (NULL != curr_ver && strcmp(curr_ver, VERSION) != 0)
    messages = AddMessage(messages, "A new version is available", 2);
  free(curr_ver);

  messages = AddMessage(messages, "'q' quits", 1);
  messages = AddMessage(messages, "Alert log file is 'alert.log'", 1);
  messages =
      AddMessage(messages, "Developed by Christiaan Boersma (2007-2021)", 1);
  messages =
      AddMessage(messages, "Welcome to Alert, this is version " VERSION, 1);

  // Some initial vars

  struct sockaddr_in local, remote;

  local.sin_family = AF_INET;
  local.sin_port = htons(0);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&(local.sin_zero), '\0', 8);

  fd_set readfds, testfds;

  FD_ZERO(&readfds);

  FD_SET(xfd, &readfds);

  testfds = readfds;

  int fdmax = xfd;

  PACKET pckt_recv;

  KeySym xkey;

  int sel = 0, recieved = 0, yes = 1;

  struct timeval *timeoutp, timeout, reconnect_timer;

  if (gettimeofday(&select_timer, NULL) == -1)
    tfprintf(stderr, "gettimeofday : %s\n", strerror(errno));

  reconnect_timer = frame_timer = select_timer;

  connection = CONNECTION_NEW;

  bool messageGUIinit = true, configGUIinit = true;

  char *text = NULL;

  memset(clients, 0, 254);

  int textlen = 0, textpos = 0;

  // Install the signal handlers

  signal(SIGHUP, sighandler);
  signal(SIGINT, sighandler);
  signal(SIGQUIT, sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGABRT, sighandler);
  signal(SIGSEGV, sighandler);
  signal(SIGPIPE, sighandler);
  signal(SIGBUS, sighandler);

  if (audio.OK())
    tfprintf(fp, "Audio STARTED, using SOUNDCARD\n");
  else
    tfprintf(fp, "Audio STARTED, using TERMINAL\n");

  // Raise window

  XEvent xevent;

  XMapRaised(xdisplay, xwindow);
  while (true) {
    XNextEvent(xdisplay, &(xevent));
    if (xevent.type == MapNotify)
      break;
  }

  tfprintf(fp, "Client STARTED\n");

  while (XtAppGetExitFlag(xtappcontext) == 0) {

    if (gettimeofday(&select_timer, NULL) == -1)
      tfprintf(stderr, "gettimeofday : %s\n", strerror(errno));

    if (connection != CONNECTION_OPENED) {

      switch (connection) {

      case CONNECTION_CLOSE:

        FD_CLR(sockfd, &readfds);

        close(sockfd);

        if (sockfd == fdmax)
          fdmax--;

        tfprintf(fp, "CLOSED connection with %s:%d on socket %d\n",
                 inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), sockfd);

      case CONNECTION_NEW:

        remote.sin_family = AF_INET;
        remote.sin_port = htons(config.port);
        if (inet_aton(config.ip, &(remote.sin_addr)) == 0) {
          tfprintf(stderr, "inet_aton\n");
          connection = CONNECTION_NEW;
          break;
        }
        memset(&(remote.sin_zero), '\0', 8);

        reconnect_timer = select_timer;

        connection = CONNECTION_OPEN;

      case CONNECTION_OPEN:

        if (NULL == messageconnect) {
          messages = AddMessage(messages, "|: connecting :|", -1);
          messageconnect = messages;
        }

        if (!timerisset(&frame_timer))
          frame_timer = select_timer;

        if (timercmp(&reconnect_timer, &select_timer, <=)) {

          struct timeval tv = {config.reconnect, 0};

          timeradd(&select_timer, &tv, &reconnect_timer);

          if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) > -1) {

            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                           sizeof(int)) == 0) {

              if (bind(sockfd, (struct sockaddr *)&local,
                       sizeof(struct sockaddr)) == 0) {

                if (connect(sockfd, (struct sockaddr *)&remote,
                            sizeof(struct sockaddr)) == 0) {

                  tfprintf(fp, "Connection OPENED to %s:%d on socket %d\n",
                           inet_ntoa(remote.sin_addr), ntohs(remote.sin_port),
                           sockfd);

                  messages = DelMessage(messages, messageconnect);
                  messageconnect = NULL;

                  struct hostent *remote_host =
                      gethostbyaddr((char *)&remote.sin_addr.s_addr,
                                    sizeof(remote.sin_addr.s_addr), AF_INET);

                  if (NULL == remote_host)
                    tfprintf(stderr, "gethostbyaddr : %s\n",
                             hstrerror(h_errno));

                  size_t text_len = strlen(remote_host->h_name) + 40;
                  char *text = (char *)malloc(
                       text_len * sizeof(char));

                  snprintf(text, text_len, "(:::) connected to %s on port %i (:::)",
                          remote_host->h_name, ntohs(remote.sin_port));

                  messages = AddMessage(messages, text, 1);

                  free(text);

                  FD_SET(sockfd, &readfds);

                  if (sockfd > fdmax)
                    fdmax = sockfd;

                  timerclear(&reconnect_timer);

                  connection = CONNECTION_OPENED;

                  PACKET handshake;
                  handshake.type = SERVER;
                  handshake.timestamp = time(NULL);
                  strcpy(handshake.nick, config.nick);
                  strcpy(handshake.msg, "HANDSHAKE");

                  if (send(sockfd, &handshake, sizeof(struct _PACKET), 0) == -1)
                    tfprintf(stderr, "send : %s\n", strerror(errno));

                } else {
                  tfprintf(stderr, "connect : %s\n", strerror(errno));
                  close(sockfd);
                }
              } else {
                tfprintf(stderr, "bind : %s\n", strerror(errno));
                close(sockfd);
              }
            } else {
              tfprintf(stderr, "setsockopt : %s\n", strerror(errno));
              close(sockfd);
            }
          } else {
            tfprintf(stderr, "socket : %s\n", strerror(errno));
            close(sockfd);
          }
        }

      default:

        break;
      };
    }

    if (timerisset(&frame_timer) && timercmp(&frame_timer, &select_timer, <=)) {

      for (size_t i = 0; i < nalerts; i++) {
        if (xalerts[i].active) {
          if (xalerts[i].sprite.xframe ==
              ((xalerts[i].sprite.xwidth * xalerts[i].sprite.xheight /
                (xalerts[i].sprite.xframewidth *
                 xalerts[i].sprite.xframeheight)) -
               1))
            xalerts[i].sprite.xframe = 0;
          else
            ++xalerts[i].sprite.xframe;
        }

        XCopyArea(
            xdisplay, xalerts[i].sprite.mask, drawclippixmap, gcclip,
            xalerts[i].sprite.xframe %
                (xalerts[i].sprite.xwidth / xalerts[i].sprite.xframewidth) *
                xalerts[i].sprite.xframewidth,
            xalerts[i].sprite.xframe /
                (xalerts[i].sprite.xwidth / xalerts[i].sprite.xframewidth) *
                xalerts[i].sprite.xframeheight,
            xalerts[i].sprite.xframewidth, xalerts[i].sprite.xframeheight,
            xalerts[i].sprite.xposx, xalerts[i].sprite.xposy);
        XSetClipMask(xdisplay, gcdraw, drawclippixmap);
        XCopyArea(
            xdisplay, xalerts[i].sprite.pixmap, xdbebackbuffer, gcdraw,
            xalerts[i].sprite.xframe %
                (xalerts[i].sprite.xwidth / xalerts[i].sprite.xframewidth) *
                xalerts[i].sprite.xframewidth,
            xalerts[i].sprite.xframe /
                (xalerts[i].sprite.xwidth / xalerts[i].sprite.xframewidth) *
                xalerts[i].sprite.xframeheight,
            xalerts[i].sprite.xframewidth, xalerts[i].sprite.xframeheight,
            xalerts[i].sprite.xposx, xalerts[i].sprite.xposy);
        XFillRectangle(xdisplay, drawclippixmap, gcclear,
                       xalerts[i].sprite.xposx, xalerts[i].sprite.xposy,
                       xalerts[i].sprite.xframewidth,
                       xalerts[i].sprite.xframeheight);
      }

      // messages
      if (textpos >= textlen) {

        textlen = 64;
        text = (char *)malloc(textlen * sizeof(char));
        strcpy(text, "...");

        int messagelen, nmessages = 0, offset = 3;

        Message *message = messages;
        while (NULL != message) {

          if (0 == message->expired--) {
            Message *tmpmessage = message->next;
            messages = DelMessage(messages, message);
            message = tmpmessage;
            continue;
          }
          ++nmessages;

          messagelen = strlen(message->text) + 5;
          if (offset + messagelen >= textlen) {
            textlen += (4 * messagelen);
            text = (char *)realloc(text, textlen * sizeof(char));
          }
          snprintf(text + offset, messagelen, " %s ...", message->text);

          offset += messagelen;

          message = message->next;
        }

        int direction, ascent, descent, textnchar = textlen = offset;

        text = (char *)realloc(text, (textlen + 1) * sizeof(char));
        text[textlen] = '\0';

        static int stop;

        if (nmessages > 0) {

          XCharStruct xcharstruct;
          XTextExtents(xfont, text, textlen, &direction, &ascent, &descent,
                       &xcharstruct);

          textheight = ascent + descent;
          textlen = xcharstruct.width;

          Pixmap tmppixmap = XCreatePixmap(
                     xdisplay, xwindow, textwidth + textlen, textheight,
                     DefaultDepth(xdisplay, DefaultScreen(xdisplay))),
                 tmpclippixmap = XCreatePixmap(
                     xdisplay, xwindow, textwidth + textlen, textheight, 1);

          XSetClipMask(xdisplay, gcdraw, None);

          XFillRectangle(xdisplay, tmpclippixmap, gcclear, 0, 0,
                         textwidth + textlen, textheight);

          XCopyArea(xdisplay, textpixmap, tmppixmap, gcdraw, textpos, 0,
                    textwidth, textheight, 0, 0);
          XCopyArea(xdisplay, textclippixmap, tmpclippixmap, gcclip, textpos, 0,
                    textwidth, textheight, 0, 0);

          XDrawString(xdisplay, tmppixmap, gcdraw, textwidth, ascent, text,
                      textnchar);
          XDrawString(xdisplay, tmpclippixmap, gcset, textwidth, ascent, text,
                      textnchar);

          XFreePixmap(xdisplay, textpixmap);
          textpixmap = tmppixmap;

          XFreePixmap(xdisplay, textclippixmap);
          textclippixmap = tmpclippixmap;

          stop = textwidth;
        } else if (stop > 0) {

          Pixmap tmppixmap = XCreatePixmap(
                     xdisplay, xwindow, textwidth, textheight,
                     DefaultDepth(xdisplay, DefaultScreen(xdisplay))),
                 tmpclippixmap =
                     XCreatePixmap(xdisplay, xwindow, textwidth, textheight, 1);

          XSetClipMask(xdisplay, gcdraw, None);

          XFillRectangle(xdisplay, tmpclippixmap, gcclear, 0, 0, textwidth,
                         textheight);

          XCopyArea(xdisplay, textpixmap, tmppixmap, gcdraw, textpos, 0, stop,
                    textheight, 0, 0);
          XCopyArea(xdisplay, textclippixmap, tmpclippixmap, gcclip, textpos, 0,
                    stop, textheight, 0, 0);

          XFreePixmap(xdisplay, textpixmap);
          textpixmap = tmppixmap;

          XFreePixmap(xdisplay, textclippixmap);
          textclippixmap = tmpclippixmap;

          stop -= textstep;

          textlen = 1;
        } else
          textlen = 0;

        free(text);
        text = NULL;

        textpos = 0;
      }

      if (textlen) {

        XCopyArea(xdisplay, textclippixmap, drawclippixmap, gcclip, textpos, 0,
                  textwidth, textheight, textposx, textposy);
        XSetClipMask(xdisplay, gcdraw, drawclippixmap);
        XCopyArea(xdisplay, textpixmap, xdbebackbuffer, gcdraw, textpos, 0,
                  textwidth, textheight, textposx, textposy);
        XFillRectangle(xdisplay, drawclippixmap, gcclear, textposx, textposy,
                       textwidth, textheight);

        textpos += textstep;

        struct timeval tv = {config.refresh / 1000,
                             (config.refresh % 1000) * 1000};
        timeradd(&select_timer, &tv, &frame_timer);
      } else
        timerclear(&frame_timer);

      XdbeSwapBuffers(xdisplay, &xdbeswapinfo, 1);

      XSync(xdisplay, false);
    }

    if (timercmp(&frame_timer, &select_timer, >) &&
        (!timerisset(&reconnect_timer) ||
         timercmp(&frame_timer, &reconnect_timer, <))) {
      timersub(&frame_timer, &select_timer, &timeout);
      timeoutp = &timeout;
    } else if (timercmp(&reconnect_timer, &select_timer, >)) {
      timersub(&reconnect_timer, &select_timer, &timeout);
      timeoutp = &timeout;
    } else
      timeoutp = NULL;

    if ((sel = select(fdmax + 1, &testfds, NULL, NULL, timeoutp)) == -1)
      break;

    if (FD_ISSET(xfd, &testfds)) {

      while (XtAppPending(xtappcontext) > 0) {

        XtAppNextEvent(xtappcontext, &xevent);

        if (xevent.xany.window != xwindow) {

          if (!XtDispatchEvent(&xevent))
            tfprintf(stderr, "XtDispatchhEvent : %s\n", typestr(xevent.type));

        } else {

          static int x, y;

          switch (xevent.type) {

          case Expose:
            if (!xevent.xexpose.count)
              frame_timer = select_timer;
            break;

          case MotionNotify:

            config.xposx = config.xposx + xevent.xmotion.x - x;
            config.xposy = config.xposy + xevent.xmotion.y - y;

            XMoveWindow(xdisplay, xwindow, config.xposx, config.xposy);

            if (!configGUIinit) {
              XtVaSetValues(posxsc, XmNvalue, config.xposx, NULL);
              XtVaSetValues(posysc, XmNvalue, config.xposy, NULL);
            }

            break;

          case ButtonPress:

            x = xevent.xbutton.x;
            y = xevent.xbutton.y;

            switch (xevent.xbutton.button) {

            case Button1:

              for (size_t i = 0; i < nalerts; i++) {
                if (xevent.xbutton.x - xalerts[i].sprite.xposx <
                        xalerts[i].sprite.xframewidth &&
                    xevent.xbutton.y - xalerts[i].sprite.xposy <
                        xalerts[i].sprite.xframeheight) {
                  if (XGetPixel(xalerts[i].sprite.hotspot,
                                xevent.xbutton.x - xalerts[i].sprite.xposx,
                                xevent.xbutton.y - xalerts[i].sprite.xposy)) {
                    if (xalerts[i].active) {
                      xalerts[i].active = false;
                      xalerts[i].sprite.xframe = 0;
                      if (NULL != xalerts[i].message) {
                        if (xalerts[i].message->expired == -1)
                          xalerts[i].message->expired = 1;
                        else
                          messages = DelMessage(messages, xalerts[i].message);
                        xalerts[i].message = NULL;
                      }
                    } else if (connection == CONNECTION_OPENED) {
                      pckt_send.type = STANDARD;
                      pckt_send.timestamp = time(NULL);
                      strcpy(pckt_send.nick, config.nick);
                      strncpy(pckt_send.msg, xalerts[i].trigger, 127);
                      if (send(sockfd, &pckt_send, sizeof(struct _PACKET), 0) ==
                          -1)
                        tfprintf(stderr, "send : %s\n", strerror(errno));
                    }
                  }
                }
              }
              break;

            case Button2:

              if (messageGUIinit) {

                messageGUIstart(xapplicationname, xapplicationclass,
                                xiconpixmap, xiconpixmapmask);
                messageGUIinit = false;
              }

              XtVaSetValues(messagewidget, XtNx, config.xmessageposx, XtNy,
                            config.xmessageposy, NULL);

              XtMapWidget(messagewidget);

              XRaiseWindow(xdisplay, XtWindow(messagewidget));

              break;

            case Button3:

              if (configGUIinit) {

                Widget form, logo, header, nicktext, iptext, porttext,
                    disclaimertext, themetext, soundtext, nicktb, iptb, porttb,
                    themedirtb, sounddirtb, reconnectdelaysc, refreshdelaysc,
                    volumesc, raisealertrb, raisechatrb, playsoundsrb,
                    disclaimer, closebutton;

                configwidget = XtVaAppCreateShell(
                    xapplicationname, xapplicationclass,
                    topLevelShellWidgetClass, xdisplay, XmNdeleteResponse,
                    XmUNMAP, XmNmappedWhenManaged, false, NULL);

                XtVaSetValues(configwidget, XmNiconPixmap, xiconpixmap,
                              XmNiconMask, xiconpixmapmask, NULL);

                form = XtVaCreateManagedWidget(
                    "Form", xmFormWidgetClass, configwidget, XmNwidth, 390,
                    XmNheight, 710, XmNshadowType, XmSHADOW_IN, NULL);

                XtRealizeWidget(configwidget);

                XtAddEventHandler(configwidget, StructureNotifyMask, false,
                                  configGUIEventPos, &config);

                closebutton = XtVaCreateManagedWidget(
                    "Close", xmPushButtonWidgetClass, form, XmNwidth, 50,
                    XmNbottomAttachment, XmATTACH_FORM, XmNrightAttachment,
                    XmATTACH_FORM, NULL);

                XtAddCallback(closebutton, XmNactivateCallback, configGUIclose,
                              &configwidget);

                XmFontList fontlist = NULL;
                if (NULL != xlargefont) {
                  XmFontListEntry fontentry = XmFontListEntryCreate(
                      XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONT, xlargefont);
                  fontlist = XmFontListAppendEntry(NULL, fontentry);
                  XmFontListEntryFree(&fontentry);
                }

                XmString xmstring = XmStringCreateLocalized("Configuration");

                header = XtVaCreateManagedWidget(
                    "Header", xmLabelWidgetClass, form, XmNfontList, fontlist,
                    XmNlabelString, xmstring, XmNtopAttachment, XmATTACH_FORM,
                    NULL);

                if (NULL != fontlist)
                  XmFontListFree(fontlist);

                XmStringFree(xmstring);

                xmstring = XmStringCreateLocalized("Nick");

                nicktext = XtVaCreateManagedWidget(
                    "NickText", xmLabelWidgetClass, form, XmNlabelString,
                    xmstring, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, header,
                    NULL);

                XmStringFree(xmstring);

                nicktb = XtVaCreateManagedWidget(
                    "Nick", xmTextFieldWidgetClass, form, XmNwidth, 250,
                    XmNmaxLength, 12, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, nicktext,
                    NULL);

                XmTextFieldSetString(nicktb, config.nick);

                XmTextFieldSetInsertionPosition(nicktb, strlen(config.nick));

                XtAddCallback(nicktb, XmNlosingFocusCallback, configGUInick,
                              &config);

                xmstring = XmStringCreateLocalized("IP");

                iptext = XtVaCreateManagedWidget(
                    "IPText", xmLabelWidgetClass, form, XmNlabelString,
                    xmstring, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, nicktb,
                    NULL);

                XmStringFree(xmstring);

                iptb = XtVaCreateManagedWidget(
                    "IP", xmTextFieldWidgetClass, form, XmNwidth, 250,
                    XmNmaxLength, 15, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, iptext,
                    NULL);

                XmTextFieldSetString(iptb, config.ip);

                XmTextFieldSetInsertionPosition(iptb, strlen(config.ip));

                XtAddCallback(iptb, XmNlosingFocusCallback, configGUIip,
                              &config);

                xmstring = XmStringCreateLocalized("Port");

                porttext = XtVaCreateManagedWidget(
                    "PortText", xmLabelWidgetClass, form, XmNlabelString,
                    xmstring, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, iptb,
                    NULL);

                XmStringFree(xmstring);

                porttb = XtVaCreateManagedWidget(
                    "Port", xmTextFieldWidgetClass, form, XmNwidth, 250,
                    XmNmaxLength, 5, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, porttext,
                    NULL);

                char portstr[6];
                if (snprintf(portstr, 5, "%d", config.port) < 0)
                  tfprintf(stderr, "snprintf\n");

                XmTextFieldSetString(porttb, portstr);

                XmTextFieldSetInsertionPosition(porttb, strlen(portstr));

                XtAddCallback(porttb, XmNlosingFocusCallback, configGUIport,
                              &config);

                XpmColorSymbol xpmcolorsymbols;
                xpmcolorsymbols.name = NULL;
                xpmcolorsymbols.value = "None";

                Pixel backgroundpixel;
                XtVaGetValues(form, XmNbackground, &backgroundpixel, NULL);

                xpmcolorsymbols.pixel = backgroundpixel;

                xpmattributes.colorsymbols = &xpmcolorsymbols;
                xpmattributes.numsymbols = 1;
                xpmattributes.valuemask = XpmSize | XpmColorSymbols;

                if (XCreatePixmapFromData(xdisplay, XtWindow(form), xpmlogo,
                                          &xiconpixmap, &xiconpixmapmask,
                                          &xpmattributes) != 0)
                  tfprintf(stderr, "XCreatePixmapFromData\n");

                logo = XtVaCreateManagedWidget(
                    "Logo", xmLabelWidgetClass, form, XmNlabelType, XmPIXMAP,
                    XmNlabelPixmap, xiconpixmap, XmNmarginWidth, 0,
                    XmNmarginHeight, 25, XmNrightAttachment, XmATTACH_FORM,
                    NULL);

                XWindowAttributes xwindowattributes;
                XGetWindowAttributes(xdisplay, DefaultRootWindow(xdisplay),
                                     &xwindowattributes);

                xmstring =
                    XmStringCreateLocalized("Horizontal position (pixels)");

                posxsc = XtVaCreateManagedWidget(
                    "Posx", xmScaleWidgetClass, form, XmNtitleString, xmstring,
                    XmNorientation, XmHORIZONTAL, XmNminimum, 0, XmNmaximum,
                    xwindowattributes.width, XmNvalue, config.xposx,
                    XmNdecimalPoints, 0, XmNshowValue, true, XmNwidth, 195,
                    XmNheight, 50, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, porttb,
                    NULL);

                XmStringFree(xmstring);

                XtAddCallback(posxsc, XmNvalueChangedCallback, configGUIposx,
                              &config);

                xmstring =
                    XmStringCreateLocalized("Vertical position (pixels)");

                posysc = XtVaCreateManagedWidget(
                    "Posy", xmScaleWidgetClass, form, XmNtitleString, xmstring,
                    XmNorientation, XmHORIZONTAL, XmNminimum, 0, XmNmaximum,
                    xwindowattributes.height, XmNvalue, config.xposy,
                    XmNdecimalPoints, 0, XmNshowValue, true, XmNwidth, 195,
                    XmNheight, 50, XmNleftAttachment, XmATTACH_WIDGET,
                    XmNleftWidget, posxsc, XmNtopAttachment, XmATTACH_WIDGET,
                    XmNtopWidget, porttb, NULL);

                XmStringFree(xmstring);

                XtAddCallback(posysc, XmNvalueChangedCallback, configGUIposy,
                              &config);

                xmstring = XmStringCreateLocalized("Reconnect timeout (s)");

                reconnectdelaysc = XtVaCreateManagedWidget(
                    "Reconnectdelay", xmScaleWidgetClass, form, XmNtitleString,
                    xmstring, XmNorientation, XmHORIZONTAL, XmNminimum, 10,
                    XmNmaximum, 300, XmNvalue, config.reconnect,
                    XmNdecimalPoints, 0, XmNshowValue, true, XmNwidth, 195,
                    XmNheight, 50, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, posxsc,
                    NULL);

                XmStringFree(xmstring);

                XtAddCallback(reconnectdelaysc, XmNvalueChangedCallback,
                              configGUIreconnect, &config);

                xmstring = XmStringCreateLocalized("Refresh timeout (ms)");

                refreshdelaysc = XtVaCreateManagedWidget(
                    "Refreshdelay", xmScaleWidgetClass, form, XmNtitleString,
                    xmstring, XmNorientation, XmHORIZONTAL, XmNminimum, 10,
                    XmNmaximum, 2500, XmNvalue, config.refresh,
                    XmNdecimalPoints, 0, XmNshowValue, true, XmNwidth, 195,
                    XmNheight, 50, XmNleftAttachment, XmATTACH_WIDGET,
                    XmNleftWidget, reconnectdelaysc, XmNtopAttachment,
                    XmATTACH_WIDGET, XmNtopWidget, posysc, NULL);

                XmStringFree(xmstring);

                XtAddCallback(refreshdelaysc, XmNvalueChangedCallback,
                              configGUIrefresh, &config);

                xmstring = XmStringCreateLocalized("Theme");

                themetext = XtVaCreateManagedWidget(
                    "ThemeText", xmLabelWidgetClass, form, XmNlabelString,
                    xmstring, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget,
                    refreshdelaysc, NULL);

                XmStringFree(xmstring);

                themedirtb = XtVaCreateManagedWidget(
                    "ThemeDir", xmTextFieldWidgetClass, form, XmNwidth, 372,
                    XmNmaxLength, 64, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, themetext,
                    NULL);

                XmTextFieldSetString(themedirtb, config.themedir);

                XtAddCallback(themedirtb, XmNactivateCallback,
                              configGUIthemedir, &themelist);

                Arg arglist[10];

                XtSetArg(arglist[0], XmNheight, 65);
                XtSetArg(arglist[1], XmNscrollBarDisplayPolicy, XmAS_NEEDED);
                XtSetArg(arglist[2], XmNitemCount, 0);
                XtSetArg(arglist[3], XmNlistSizePolicy, XmCONSTANT);
                XtSetArg(arglist[4], XmNwidth, 370);
                XtSetArg(arglist[5], XmNautomaticSelection, XmAUTO_SELECT);
                XtSetArg(arglist[6], XmNleftAttachment, XmATTACH_FORM);
                XtSetArg(arglist[7], XmNtopAttachment, XmATTACH_WIDGET);
                XtSetArg(arglist[8], XmNtopWidget, themedirtb);
                themelist = XmCreateScrolledList(form, "ThemeList", arglist, 9);

                XtAddCallback(themelist, XmNbrowseSelectionCallback,
                              configGUItheme, &config);

                xmstring = XmStringCreateLocalized("Sound");

                soundtext = XtVaCreateManagedWidget(
                    "Sound", xmLabelWidgetClass, form, XmNlabelString, xmstring,
                    XmNleftAttachment, XmATTACH_FORM, XmNtopAttachment,
                    XmATTACH_WIDGET, XmNtopWidget, themelist, NULL);

                XmStringFree(xmstring);

                sounddirtb = XtVaCreateManagedWidget(
                    "SoundDir", xmTextFieldWidgetClass, form, XmNwidth, 372,
                    XmNmaxLength, 64, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, soundtext,
                    NULL);

                XmTextFieldSetString(sounddirtb, config.sounddir);

                XtAddCallback(sounddirtb, XmNactivateCallback,
                              configGUIsounddir, &soundlist);

                xmstring = XmStringCreateLocalized("");

                volumesc = XtVaCreateManagedWidget(
                    "Volume", xmScaleWidgetClass, form, XmNtitleString,
                    xmstring, XmNorientation, XmVERTICAL, XmNsliderMark, XmNONE,
                    XmNsliderSize, 10, XmNminimum, 0, XmNmaximum, 100, XmNvalue,
                    config.volume, XmNdecimalPoints, 0, XmNshowValue, false,
                    XmNwidth, 25, XmNheight, 50, XmNleftAttachment,
                    XmATTACH_FORM, XmNtopAttachment, XmATTACH_WIDGET,
                    XmNtopWidget, sounddirtb, NULL);

                XmStringFree(xmstring);

                XtAddCallback(volumesc, XmNvalueChangedCallback,
                              configGUIvolume, NULL);

                XtSetArg(arglist[4], XmNwidth, 345);
                XtSetArg(arglist[6], XmNleftAttachment, XmATTACH_WIDGET);
                XtSetArg(arglist[7], XmNleftWidget, volumesc);
                XtSetArg(arglist[8], XmNtopAttachment, XmATTACH_WIDGET);
                XtSetArg(arglist[9], XmNtopWidget, sounddirtb);
                soundlist =
                    XmCreateScrolledList(form, "SoundList", arglist, 10);

                XtAddCallback(soundlist, XmNbrowseSelectionCallback,
                              configGUIsound, &config);

                raisealertrb = XtVaCreateManagedWidget(
                    "Raise window on alert", xmToggleButtonWidgetClass, form,
                    XmNleftAttachment, XmATTACH_FORM, XmNtopAttachment,
                    XmATTACH_WIDGET, XmNtopWidget, soundlist, NULL);

                XmToggleButtonSetState(raisealertrb, config.raisealert, 0);

                XtAddCallback(raisealertrb, XmNvalueChangedCallback,
                              configGUIraisealert, &config);

                raisechatrb = XtVaCreateManagedWidget(
                    "Raise window on chat", xmToggleButtonWidgetClass, form,
                    XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget,
                    raisealertrb, XmNtopAttachment, XmATTACH_WIDGET,
                    XmNtopWidget, soundlist, NULL);

                XmToggleButtonSetState(raisechatrb, config.raisechat, 0);

                XtAddCallback(raisechatrb, XmNvalueChangedCallback,
                              configGUIraisechat, &config);

                playsoundsrb = XtVaCreateManagedWidget(
                    "Play sounds", xmToggleButtonWidgetClass, form,
                    XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget,
                    raisechatrb, XmNtopAttachment, XmATTACH_WIDGET,
                    XmNtopWidget, soundlist, NULL);

                XmToggleButtonSetState(playsoundsrb, config.playsounds, 0);

                XtAddCallback(playsoundsrb, XmNvalueChangedCallback,
                              configGUIplaysounds, &config);

                xmstring = XmStringCreateLocalized("Disclaimer");

                disclaimertext = XtVaCreateManagedWidget(
                    "DisclaimerText", xmLabelWidgetClass, form, XmNlabelString,
                    xmstring, XmNleftAttachment, XmATTACH_FORM,
                    XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget,
                    raisealertrb, NULL);

                XmStringFree(xmstring);

                XtSetArg(arglist[0], XmNrows, 10);
                XtSetArg(arglist[1], XmNcolumns, 58);
                XtSetArg(arglist[2], XmNeditable, false);
                XtSetArg(arglist[3], XmNscrollHorizontal, false);
                XtSetArg(arglist[4], XmNeditMode, XmMULTI_LINE_EDIT);
                XtSetArg(arglist[5], XmNleftAttachment, XmATTACH_FORM);
                XtSetArg(arglist[6], XmNtopAttachment, XmATTACH_WIDGET);
                XtSetArg(arglist[7], XmNtopWidget, disclaimertext);
                XtSetArg(arglist[8], XmNtoolTipString, NULL);

                disclaimer =
                    XmCreateScrolledText(form, "disclaimer", arglist, 9);

                XmTextSetString(
                    disclaimer,
                    "                                   Alert version 4.1.0\n\n"
                    "Copyright (C) 2007-2021 Christiaan Boersma\n\n"
                    "E-mail: christiaanboersma@hotmail.com\n"
                    "This program is free software; you can redistribute it\n"
                    "and/or modify it under the terms of the GNU General\n"
                    "Public License as published by the Free Software\n"
                    "Foundation; either version 1.1, or (at your option)\n"
                    "any later version.\n\n"
                    "This program is distributed in the hope that it will\n"
                    "be useful,but WITHOUT ANY WARRANTY; without even the\n"
                    "implied warranty of MERCHANTABILITY or FITNESS FOR A\n"
                    "PARTICULAR PURPOSE.  See the GNU General Public License\n"
                    "for more details.\n\n"
                    "You should have received a copy of the GNU General\n"
                    "Public License along with this program; if not, write\n"
                    "to the Free Software Foundation, Inc., 675 Mass Ave,\n"
                    "Cambridge, MA 02139, USA.\n\n");

                XtManageChild(disclaimer);
                XtManageChild(themelist);
                XtManageChild(soundlist);

                configGUIinit = false;
              }

              XmString xmstring;

              XmStringTable xmstringtable = NULL;
              int nitems =
                  XmListFiles(xmstringtable, config.themedir, ".theme");
              if (nitems > 0) {
                XtVaSetValues(themelist, XmNitemCount, nitems, XmNitems,
                              xmstringtable, XmNvisibleItemCount, 3, NULL);
                xmstring = XmStringCreateSimple(config.theme);
                XmListSelectItem(themelist, xmstring, false);
                XmStringFree(xmstring);
                for (int i = 0; i < nitems; i++)
                  XmStringFree(xmstringtable[i]);
                XtFree((char *)xmstringtable);
                xmstringtable = NULL;
              } else {
                XmListDeleteAllItems(themelist);
                XtVaSetValues(themelist, XmNheight, 65, NULL);
              }

              nitems = XmListFiles(xmstringtable, config.sounddir, ".raw");
              if (nitems > 0) {
                XtVaSetValues(soundlist, XmNitemCount, nitems, XmNitems,
                              xmstringtable, XmNvisibleItemCount, 3, NULL);
                xmstring = XmStringCreateSimple(config.sound);
                XmListSelectItem(soundlist, xmstring, false);
                XmStringFree(xmstring);
                for (int i = 0; i < nitems; i++)
                  XmStringFree(xmstringtable[i]);
                XtFree((char *)xmstringtable);
                xmstringtable = NULL;
              } else {
                XmListDeleteAllItems(soundlist);
                XtVaSetValues(soundlist, XmNheight, 65, NULL);
              }

              XtVaSetValues(posxsc, XmNvalue, config.xposx, NULL);
              XtVaSetValues(posysc, XmNvalue, config.xposy, NULL);

              XtVaSetValues(configwidget, XtNx, config.xconfigposx, XtNy,
                            config.xconfigposy, NULL);

              XtMapWidget(configwidget);

              XRaiseWindow(xdisplay, XtWindow(configwidget));

              break;
            };

            break;

          case KeyPress:

            xkey = XkbKeycodeToKeysym(xdisplay, xevent.xkey.keycode, 0,
                                      xevent.xkey.state & ShiftMask ? 1 : 0);

            if (xkey == XK_Escape || xkey == XK_q)
              XtAppSetExitFlag(xtappcontext);

            break;

          case ClientMessage:

            if (xevent.xclient.message_type == wmProtocols) {

              if (xevent.xclient.data.l[0] == (int)wmDeleteWindow)
                XtAppSetExitFlag(xtappcontext);
              else if (xevent.xclient.data.l[0] == (int)wmSaveState) {
                XtAppSetExitFlag(xtappcontext);
                XSetCommand(xdisplay, xwindow, argv, argc);
                tfprintf(stderr, "Saved application state\n");
              } else
                tfprintf(stderr, "Unknown ClientMessage\n");
            }

            break;

          default:

            tfprintf(stderr, "XEventType : %s\n", typestr(xevent.type));

            break;
          };
        }
      }
    } else if ((connection == CONNECTION_OPENED) &&
               (FD_ISSET(sockfd, &testfds))) {

      if ((recieved = recv(sockfd, &pckt_recv, sizeof(struct _PACKET), 0)) <=
          0) {

        if (recieved == -1)
          tfprintf(stderr, "recv : %s\n", strerror(errno));

        tfprintf(fp, "LOST connection from %s:%d on socket %d\n",
                 inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), sockfd);

        FD_CLR(sockfd, &readfds);

        close(sockfd);

        if (sockfd == fdmax)
          fdmax--;

        memset(clients, 0, 254);

        if (!messageGUIinit)
          XmTextSetString(clientstext, clients);

        connection = CONNECTION_OPEN;
      } else {

        if (recieved == sizeof(struct _PACKET)) {

          struct tm *tm_struct = NULL;

          if (pckt_recv.type == STANDARD) {

            strtrim(pckt_recv.msg);

            tfprintf(fp, "RECIEVED message : %s from %s:%d on socket %d\n",
                     pckt_recv.msg, inet_ntoa(remote.sin_addr),
                     ntohs(remote.sin_port), sockfd);

            for (size_t i = 0; i < nalerts; i++) {
              if (strcmp(xalerts[i].trigger, pckt_recv.msg) == 0) {

                xalerts[i].active = true;

                size_t text_len = strlen(pckt_recv.msg) + strlen(pckt_recv.nick) + 16;
                char *text = (char *)malloc(
                    text_len *
                    sizeof(char));
                tm_struct = localtime(&(pckt_recv.timestamp));
                snprintf(text, text_len, "%02d:%02d:%02d - %s @ %s", tm_struct->tm_hour,
                        tm_struct->tm_min, tm_struct->tm_sec, pckt_recv.msg,
                        pckt_recv.nick);
                if (NULL != xalerts[i].message)
                  xalerts[i].message->expired = 1;
                messages = AddMessage(messages, text, -1);
                xalerts[i].message = messages;
                free(text);

                frame_timer = select_timer;

                if (config.raisealert) {

                  XRaiseWindow(xdisplay, xwindow);
                  xactivewindowevent.window = xwindow;
                  xactivewindowevent.data.l[1] = CurrentTime;
                  XSendEvent(xdisplay, DefaultRootWindow(xdisplay), false,
                             SubstructureNotifyMask | SubstructureRedirectMask,
                             (XEvent *)&xactivewindowevent);
                }

                if (config.playsounds) {
                  if (audio.OK())
                    audio.playSound(0);
                  else {
                    printf("%c", 7);
                    fflush(stdout);
                  }
                }

                break;
              }
            }
          } else if (pckt_recv.type == IM) {

            if (messageGUIinit) {

              messageGUIstart(xapplicationname, xapplicationclass, xiconpixmap,
                              xiconpixmapmask);
              messageGUIinit = false;
            }

            static int len;

            size_t text_len = strlen(pckt_recv.msg) + 29;
            char *text =
                (char *)malloc(text_len * sizeof(char));

            tm_struct = localtime(&(pckt_recv.timestamp));

            snprintf(text, text_len, "%02d:%02d:%02d %-12s : %s\n", tm_struct->tm_hour,
                    tm_struct->tm_min, tm_struct->tm_sec, pckt_recv.nick,
                    pckt_recv.msg);

            XmTextInsert(messagetext, len, text);

            len += strlen(text);

            free(text);

            XmTextShowPosition(messagetext, len);

            if (config.raisechat) {

              XtMapWidget(messagewidget);
              XRaiseWindow(xdisplay, XtWindow(messagewidget));
              xactivewindowevent.window = XtWindow(messagewidget);
              xactivewindowevent.data.l[1] = CurrentTime;
              XSendEvent(xdisplay, DefaultRootWindow(xdisplay), false,
                         SubstructureNotifyMask | SubstructureRedirectMask,
                         (XEvent *)&xactivewindowevent);
              XSync(xdisplay, false);
            }

            if (config.playsounds) {
              if (audio.OK())
                audio.playSound(0);
              else {
                printf("%c", 7);
                fflush(stdout);
              }
            }
          } else if (pckt_recv.type == SERVER) {

            tm_struct = localtime(&(pckt_recv.timestamp));

            if (strcmp(pckt_recv.msg, "HANDSHAKE") == 0) {

              strncpy(clients, pckt_recv.data, 127);

              if (!messageGUIinit)
                XmTextSetString(clientstext, clients);
            } else {
              size_t text_len = strlen(pckt_recv.nick) + strlen(pckt_recv.msg) + 11;
              text = (char *)realloc(
                  text, text_len * sizeof(char));

              snprintf(text, text_len, "%02d:%02d:%02d %s %s", tm_struct->tm_hour,
                      tm_struct->tm_min, tm_struct->tm_sec, pckt_recv.nick,
                      pckt_recv.msg);

              messages = AddMessage(messages, text, 1);
            }

            frame_timer = select_timer;

            free(text);
          } else
            tfprintf(stderr, "INCOMPATIBLE TYPE: %d\n", pckt_recv.type);
        } else
          tfprintf(stderr, "LOST package : count too low\n");
      }
    }
    testfds = readfds;
  }

  if (sel == -1)
    tfprintf(stderr, "select : %s\n", strerror(errno));

  close(sockfd);

  for (size_t i = 0; i < nalerts; i++)
    XFreeAlert(xdisplay, xalerts[i]);

  if (NULL != xfont)
    XFreeFont(xdisplay, xfont);
  if (NULL != xlargefont)
    XFreeFont(xdisplay, xlargefont);

  XFreeGC(xdisplay, gcdraw);
  XFreeGC(xdisplay, gcclip);
  XFreeGC(xdisplay, gcset);
  XFreeGC(xdisplay, gcclear);

  XFreePixmap(xdisplay, xiconpixmap);
  XFreePixmap(xdisplay, xiconpixmapmask);

  XFreePixmap(xdisplay, textpixmap);
  XFreePixmap(xdisplay, textclippixmap);

  XFreeColormap(xdisplay, xcolormap);

  XdbeDeallocateBackBufferName(xdisplay, xdbebackbuffer);

  XDestroyWindow(xdisplay, xwindow);

  XtDestroyApplicationContext(xtappcontext);

  if (NULL != text)
    free(text);

  messages = FreeMessages(messages);

  FILE *fpc = fopen(configfile, "w");

  if (fpc != NULL) {
    fwrite(&config, sizeof(configStruct), 1, fpc);
    tfprintf(fp, "WRITING config file : %s\n", configfile);
  } else
    tfprintf(stderr, "fopen : %s\n", strerror(errno));

  fclose(fpc);

  tfprintf(fp, "Client had CLEAN exit\n");

  fclose(fp);

  // Normal exit

  return (0);
}

char *strtrim(char *str) {

  size_t len = strlen(str);

  size_t iii = 0;
  for (size_t ii = 0; ii < len; ii++) {

    if (isspace(str[ii]))
      continue;

    str[iii++] = str[ii];
  }

  str[iii] = '\0';

  return (str);
}

void configGUIEventPos(Widget w, XtPointer clientdata, XEvent *xevent,
                       Boolean *continue_to_dispatch) {

  if (xevent->type == ConfigureNotify) {
    ((configStruct *)clientdata)->xconfigposx = xevent->xconfigure.x;
    ((configStruct *)clientdata)->xconfigposy = xevent->xconfigure.y;
  }
}

void configGUIclose(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  XtUnmapWidget(*((Widget *)clientdata));

  FILE *fpc = fopen(configfile, "w");

  if (fpc != NULL) {
    fwrite(&config, sizeof(configStruct), 1, fpc);
    tfprintf(stderr, "WRITING config file : %s\n", configfile);
  } else
    tfprintf(stderr, "fopen : %s\n", strerror(errno));

  fclose(fpc);
}

void configGUInick(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *textfieldstring = XmTextFieldGetString(w);

  strncpy(((configStruct *)clientdata)->nick, textfieldstring, 12);

  XtFree(textfieldstring);
}

void configGUIip(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *textfieldstring = XmTextFieldGetString(w);

  if (strcmp(((configStruct *)clientdata)->ip, textfieldstring) != 0) {

    strncpy(((configStruct *)clientdata)->ip, textfieldstring, 15);

    if (connection == CONNECTION_OPENED)
      connection = CONNECTION_CLOSE;
    else if (connection == CONNECTION_OPEN)
      connection = CONNECTION_NEW;
  }

  XtFree(textfieldstring);
}

void configGUIport(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *textfieldstring = XmTextFieldGetString(w);

  unsigned int port = atoi(textfieldstring);

  if (((configStruct *)clientdata)->port != port) {

    ((configStruct *)clientdata)->port = port;

    if (connection == CONNECTION_OPENED)
      connection = CONNECTION_CLOSE;
    else if (connection == CONNECTION_OPEN)
      connection = CONNECTION_NEW;
  }

  XtFree(textfieldstring);
}

void configGUIposx(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  ((configStruct *)clientdata)->xposx = ((XmScaleCallbackStruct *)cbs)->value;

  XMoveWindow(xdisplay, xwindow, ((configStruct *)clientdata)->xposx,
              ((configStruct *)clientdata)->xposy);
}

void configGUIposy(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  ((configStruct *)clientdata)->xposy = ((XmScaleCallbackStruct *)cbs)->value;

  XMoveWindow(xdisplay, xwindow, ((configStruct *)clientdata)->xposx,
              ((configStruct *)clientdata)->xposy);
}

void configGUIreconnect(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  ((configStruct *)clientdata)->reconnect =
      ((XmScaleCallbackStruct *)cbs)->value;
}

void configGUIthemedir(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *textfieldstring = XmTextFieldGetString(w);

  strncpy(config.themedir, textfieldstring, 63);

  size_t len = strlen(textfieldstring);
  if (config.themedir[len - 1] != '/' && len < 63) {
    config.themedir[len] = '/';
    config.themedir[len + 1] = '\0';
    XmTextFieldSetString(w, config.themedir);
  }

  XtFree(textfieldstring);

  XmStringTable xmstringtable = NULL;
  int nitems = XmListFiles(xmstringtable, config.themedir, ".theme");
  if (nitems > 0) {
    XtVaSetValues(*((Widget *)clientdata), XmNitemCount, nitems, XmNitems,
                  xmstringtable, XmNheight, 65, NULL);
    for (int i = 0; i < nitems; i++)
      XmStringFree(xmstringtable[i]);
    XtFree((char *)xmstringtable);
  } else {
    XmListDeleteAllItems(*((Widget *)clientdata));
    XtVaSetValues(*((Widget *)clientdata), XmNitemCount, 0, XmNheight, 65,
                  NULL);
  }
}

void configGUIsounddir(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *textfieldstring = XmTextFieldGetString(w);

  strncpy(config.sounddir, textfieldstring, 63);

  size_t len = strlen(textfieldstring);
  if (config.sounddir[len - 1] != '/' && len < 63) {
    config.sounddir[len] = '/';
    config.sounddir[len + 1] = '\0';
    XmTextFieldSetString(w, config.sounddir);
  }

  XtFree(textfieldstring);

  XmStringTable xmstringtable = NULL;
  int nitems = XmListFiles(xmstringtable, config.sounddir, ".raw");
  if (nitems > 0) {
    XtVaSetValues(*((Widget *)clientdata), XmNitemCount, nitems, XmNitems,
                  xmstringtable, XmNheight, 65, NULL);
    for (int i = 0; i < nitems; i++)
      XmStringFree(xmstringtable[i]);
    XtFree((char *)xmstringtable);
  } else {
    XmListDeleteAllItems(*((Widget *)clientdata));
    XtVaSetValues(*((Widget *)clientdata), XmNitemCount, 0, XmNheight, 65,
                  NULL);
  }
}

void configGUIsound(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *sound, *path = NULL;

  XmStringGetLtoR(((XmListCallbackStruct *)cbs)->item, XmSTRING_DEFAULT_CHARSET,
                  &sound);

  path = (char *)malloc(
      (strlen(((configStruct *)clientdata)->sounddir) + strlen(sound) + 1) *
      sizeof(char));
  strcpy(path, ((configStruct *)clientdata)->sounddir);
  path = strcat(path, sound);

  strncpy(((configStruct *)clientdata)->sound, sound, 63);

  if (((configStruct *)clientdata)->playsounds) {

    if (audio.OK()) {

      audio.audioClear();

      audio.loadSoundFromFile(path);

      audio.playSound(0);
    }
  }

  free(sound);
  free(path);
}

void configGUIvolume(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  int volume = ((XmScaleCallbackStruct *)cbs)->value;
  audio.setVolume(volume);
  config.volume = volume;
}

void configGUItheme(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *theme, *path = NULL;

  XmStringGetLtoR(((XmListCallbackStruct *)cbs)->item, XmSTRING_DEFAULT_CHARSET,
                  &theme);

  path = (char *)malloc(
      (strlen(((configStruct *)clientdata)->themedir) + strlen(theme) + 1) *
      sizeof(char));
  strcpy(path, ((configStruct *)clientdata)->themedir);
  path = strcat(path, theme);

  strncpy(((configStruct *)clientdata)->theme, theme, 63);

  XLoadTheme(path);

  XResizeWindow(xdisplay, xwindow, xwidth, xheight);

  XShapeCombineMask(xdisplay, xwindow, ShapeBounding, 0, 0,
                    xbackgroundpixmapmask, ShapeSet);
  XShapeCombineMask(xdisplay, xwindow, ShapeClip, 0, 0, xbackgroundpixmapmask,
                    ShapeSet);

  XSetWindowBackgroundPixmap(xdisplay, xwindow, xbackgroundpixmap);

  XdbeSwapBuffers(xdisplay, &xdbeswapinfo, 1);
  frame_timer = select_timer;

  free(theme);
  free(path);
}

void configGUIraisealert(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  ((configStruct *)clientdata)->raisealert =
      ((XmToggleButtonCallbackStruct *)cbs)->set;
}

void configGUIraisechat(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  ((configStruct *)clientdata)->raisechat =
      ((XmToggleButtonCallbackStruct *)cbs)->set;
}

void configGUIplaysounds(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  ((configStruct *)clientdata)->playsounds =
      ((XmToggleButtonCallbackStruct *)cbs)->set;

  audio.audioClear();

  if (((configStruct *)clientdata)->playsounds) {

    char *path = NULL;
    path = (char *)malloc((strlen(((configStruct *)clientdata)->sounddir) +
                           strlen(((configStruct *)clientdata)->sound) + 1) *
                          sizeof(char));
    strcpy(path, ((configStruct *)clientdata)->sounddir);
    path = strcat(path, ((configStruct *)clientdata)->sound);
    audio.loadSoundFromFile(path);
    audio.setVolume(config.volume);
    free(path);
  }
}

void configGUIrefresh(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  ((configStruct *)clientdata)->refresh = ((XmScaleCallbackStruct *)cbs)->value;
}

void XFreeAlert(Display *xdisplay, XAlert &xalert) {

  if (NULL != xalert.trigger)
    free(xalert.trigger);

  XFreePixmap(xdisplay, xalert.sprite.pixmap);
  XFreePixmap(xdisplay, xalert.sprite.mask);

  XDestroyImage(xalert.sprite.hotspot);
}

char *typestr(int type) {

  char *typestrings[] = {"ZERO",
                         "ONE",
                         "KeyPress",
                         "KeyRelease ",
                         "ButtonPress",
                         "ButtonRelease",
                         "MotionNotify",
                         "EnterNotify",
                         "LeaveNotify",
                         "FocusIn",
                         "FocusOut",
                         "KeymapNotify",
                         "Expose",
                         "GraphicsExpose",
                         "NoExpose",
                         "VisibilityNotify",
                         "CreateNotify",
                         "DestroyNotify",
                         "UnmapNotify",
                         "MapNotify",
                         "MapRequest",
                         "ReparentNotify",
                         "ConfigureNotify",
                         "ConfigureRequest",
                         "GravityNotify",
                         "ResizeRequest",
                         "CirculateNotify",
                         "CirculateRequest",
                         "PropertyNotify",
                         "SelectionClear",
                         "SelectionRequest",
                         "SelectionNotify",
                         "ColormapNotify",
                         "ClientMessage",
                         "MappingNotify",
                         "LASTEvent"};

  if (type < LASTEvent)
    return (typestrings[type]);
  else
    return (NULL);
}

int tfprintf(FILE *restrict_stream, const char *restrict_format, ...) {

  time_t t = time(NULL);

  struct tm *tm_struct = localtime(&t);

  va_list args;

  va_start(args, restrict_format);

  fprintf(restrict_stream, "%02d/%02d/%02d %02d:%02d:%02d - ",
          tm_struct->tm_mday, tm_struct->tm_mon + 1, tm_struct->tm_year + 1900,
          tm_struct->tm_hour, tm_struct->tm_min, tm_struct->tm_sec);

  int ret = vfprintf(restrict_stream, restrict_format, args);

  fflush(restrict_stream);

  va_end(args);

  return (ret);
}

int XmListFiles(XmStringTable &xmstringtable, const char *dir,
                const char *filter) {

  struct dirent **listing;
  int nfiles, nitems = 0;
  if ((nfiles = scandir(dir, &listing, NULL, alphasort)) < 0) {
    tfprintf(stderr, "scandir : %s\n", strerror(errno));
    xmstringtable = NULL;
    return (0);
  }

  size_t nfilter = strlen(filter);

  xmstringtable = (XmStringTable)XtRealloc((char *)xmstringtable,
                                           nfiles * sizeof(XmString *));
  for (int file = 0; file < nfiles; file++) {
    if (strlen(listing[file]->d_name) > nfilter) {
      if (strcmp(listing[file]->d_name + strlen(listing[file]->d_name) -
                     nfilter,
                 filter) == 0)
        xmstringtable[nitems++] = XmStringCreateSimple(listing[file]->d_name);
    }
    free(listing[file]);
  }

  xmstringtable = (XmStringTable)XtRealloc((char *)xmstringtable,
                                           nitems * sizeof(XmString *));
  free(listing);

  return (nitems);
}

void XLoadTheme(const char *filename) {

  int nvar;
  Pair *var = ReadThemeVarsFromFile(filename, nvar);
  if (NULL == var)
    tfprintf(stderr, "ReadThemeVarsFromFile : unable to open theme file\n");

  char *themevar = GetThemeVar(var, nvar, "text.color"),
       *base = GetThemeVar(var, nvar, "base"), *path = NULL;

  // text
  textposx = 17;
  textposy = 67;
  textwidth = 158;

  themevar = GetThemeVar(var, nvar, "text.posx");
  if (themevar[0] != '\0')
    textposx = atoi(themevar);

  themevar = GetThemeVar(var, nvar, "text.posy");
  if (themevar[0] != '\0')
    textposy = atoi(themevar);

  themevar = GetThemeVar(var, nvar, "text.width");
  if (themevar[0] != '\0')
    textwidth = atoi(themevar);

  themevar = GetThemeVar(var, nvar, "text.step");
  if (themevar[0] != '\0')
    textstep = atoi(themevar);

  // sprites
  XpmAttributes xpmattributes;
  xpmattributes.valuemask = XpmSize;

  XRGBAPixmap *xrgbapixmap;

  themevar = GetThemeVar(var, nvar, "tea.sprite");
  if (themevar[0] != '\0') {
    path = (char *)realloc(
        path, (strlen(config.themedir) + strlen(base) + strlen(themevar) + 1) *
                  sizeof(char));
    path = strcpy(path, config.themedir);
    path = strcat(path, base);
    path = strcat(path, themevar);
    if ((xrgbapixmap = XReadPNGPixmapFromFile(path)) != NULL) {
      xalerts[0].sprite.xwidth = xrgbapixmap->width;
      xalerts[0].sprite.xheight = xrgbapixmap->height;
      xalerts[0].sprite.pixmap = xrgbapixmap->pixmap;
      xalerts[0].sprite.mask = xrgbapixmap->mask;
      free(xrgbapixmap);
    } else {
      if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay),
                                xpmdefaulttea, &(xalerts[0].sprite.pixmap),
                                &(xalerts[0].sprite.mask),
                                &xpmattributes) != XpmSuccess) {
        free(path);
        exit(1);
      }
      xalerts[0].sprite.xwidth = xpmattributes.width;
      xalerts[0].sprite.xheight = xpmattributes.height;
      free(path);
      path = NULL;
    }
  } else {
    if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay),
                              xpmdefaulttea, &(xalerts[0].sprite.pixmap),
                              &(xalerts[0].sprite.mask),
                              &xpmattributes) != XpmSuccess) {
      tfprintf(stderr, "XCreatePixmapFromData\n");
      exit(1);
    }
    xalerts[0].sprite.xwidth = xpmattributes.width;
    xalerts[0].sprite.xheight = xpmattributes.height;
  }

  xalerts[0].sprite.xframe = 0;

  themevar = GetThemeVar(var, nvar, "tea.width");
  if (themevar[0] != '\0')
    xalerts[0].sprite.xframewidth = atoi(themevar);
  else
    xalerts[0].sprite.xframewidth = 55;

  themevar = GetThemeVar(var, nvar, "tea.height");
  if (themevar[0] != '\0')
    xalerts[0].sprite.xframeheight = atoi(themevar);
  else
    xalerts[0].sprite.xframeheight = 55;

  themevar = GetThemeVar(var, nvar, "tea.posx");
  if (themevar[0] != '\0')
    xalerts[0].sprite.xposx = atoi(themevar);
  else
    xalerts[0].sprite.xposx = 10;

  themevar = GetThemeVar(var, nvar, "tea.posy");
  if (themevar[0] != '\0')
    xalerts[0].sprite.xposy = atoi(themevar);
  else
    xalerts[0].sprite.xposy = 6;

  if (NULL != xalerts[0].sprite.hotspot)
    XDestroyImage(xalerts[0].sprite.hotspot);
  xalerts[0].sprite.hotspot = XGetImage(
      xdisplay, xalerts[0].sprite.mask, 0, 0, xalerts[0].sprite.xframewidth,
      xalerts[0].sprite.xframeheight, 1, ZPixmap);

  //

  themevar = GetThemeVar(var, nvar, "coffee.sprite");
  if (themevar[0] != '\0') {
    path = (char *)realloc(
        path, (strlen(config.themedir) + strlen(base) + strlen(themevar) + 1) *
                  sizeof(char));
    path = strcpy(path, config.themedir);
    path = strcat(path, base);
    path = strcat(path, themevar);
    if ((xrgbapixmap = XReadPNGPixmapFromFile(path)) != NULL) {
      xalerts[1].sprite.xwidth = xrgbapixmap->width;
      xalerts[1].sprite.xheight = xrgbapixmap->height;
      xalerts[1].sprite.pixmap = xrgbapixmap->pixmap;
      xalerts[1].sprite.mask = xrgbapixmap->mask;
      free(xrgbapixmap);
    } else {
      if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay),
                                xpmdefaultcoffee, &(xalerts[1].sprite.pixmap),
                                &(xalerts[1].sprite.mask),
                                &xpmattributes) != XpmSuccess) {
        free(path);
        exit(1);
      }
      xalerts[1].sprite.xwidth = xpmattributes.width;
      xalerts[1].sprite.xheight = xpmattributes.height;
      free(path);
      path = NULL;
    }
  } else {
    if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay),
                              xpmdefaultcoffee, &(xalerts[1].sprite.pixmap),
                              &(xalerts[1].sprite.mask),
                              &xpmattributes) != XpmSuccess) {
      tfprintf(stderr, "XCreatePixmapFromData\n");
      exit(1);
    }
    xalerts[1].sprite.xwidth = xpmattributes.width;
    xalerts[1].sprite.xheight = xpmattributes.height;
  }

  xalerts[1].sprite.xframe = 0;

  themevar = GetThemeVar(var, nvar, "coffee.width");
  if (themevar[0] != '\0')
    xalerts[1].sprite.xframewidth = atoi(themevar);
  else
    xalerts[1].sprite.xframewidth = 55;

  themevar = GetThemeVar(var, nvar, "coffee.height");
  if (themevar[0] != '\0')
    xalerts[1].sprite.xframeheight = atoi(themevar);
  else
    xalerts[1].sprite.xframeheight = 55;

  themevar = GetThemeVar(var, nvar, "coffee.posx");
  if (themevar[0] != '\0')
    xalerts[1].sprite.xposx = atoi(themevar);
  else
    xalerts[1].sprite.xposx = 68;

  themevar = GetThemeVar(var, nvar, "coffee.posy");
  if (themevar[0] != '\0')
    xalerts[1].sprite.xposy = atoi(themevar);
  else
    xalerts[1].sprite.xposy = 6;

  if (NULL != xalerts[1].sprite.hotspot)
    XDestroyImage(xalerts[1].sprite.hotspot);
  xalerts[1].sprite.hotspot = XGetImage(
      xdisplay, xalerts[1].sprite.mask, 0, 0, xalerts[1].sprite.xframewidth,
      xalerts[1].sprite.xframeheight, 1, ZPixmap);

  //

  themevar = GetThemeVar(var, nvar, "lunch.sprite");
  if (themevar[0] != '\0') {
    path = (char *)realloc(
        path, (strlen(config.themedir) + strlen(base) + strlen(themevar) + 1) *
                  sizeof(char));
    path = strcpy(path, config.themedir);
    path = strcat(path, base);
    path = strcat(path, themevar);
    if ((xrgbapixmap = XReadPNGPixmapFromFile(path)) != NULL) {
      xalerts[2].sprite.xwidth = xrgbapixmap->width;
      xalerts[2].sprite.xheight = xrgbapixmap->height;
      xalerts[2].sprite.pixmap = xrgbapixmap->pixmap;
      xalerts[2].sprite.mask = xrgbapixmap->mask;
      free(xrgbapixmap);
    } else {
      if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay),
                                xpmdefaultlunch, &(xalerts[2].sprite.pixmap),
                                &(xalerts[2].sprite.mask),
                                &xpmattributes) != XpmSuccess) {
        free(path);
        exit(1);
      }
      xalerts[2].sprite.xwidth = xpmattributes.width;
      xalerts[2].sprite.xheight = xpmattributes.height;
      free(path);
      path = NULL;
    }
  } else {
    if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay),
                              xpmdefaultlunch, &(xalerts[2].sprite.pixmap),
                              &(xalerts[2].sprite.mask),
                              &xpmattributes) != XpmSuccess) {
      tfprintf(stderr, "XCreatePixmapFromData\n");
      exit(1);
    }
    xalerts[2].sprite.xwidth = xpmattributes.width;
    xalerts[2].sprite.xheight = xpmattributes.height;
  }

  xalerts[2].sprite.xframe = 0;

  themevar = GetThemeVar(var, nvar, "lunch.width");
  if (themevar[0] != '\0')
    xalerts[2].sprite.xframewidth = atoi(themevar);
  else
    xalerts[2].sprite.xframewidth = 55;

  themevar = GetThemeVar(var, nvar, "lunch.height");
  if (themevar[0] != '\0')
    xalerts[2].sprite.xframeheight = atoi(themevar);
  else
    xalerts[2].sprite.xframeheight = 55;

  themevar = GetThemeVar(var, nvar, "lunch.posx");
  if (themevar[0] != '\0')
    xalerts[2].sprite.xposx = atoi(themevar);
  else
    xalerts[2].sprite.xposx = 126;

  themevar = GetThemeVar(var, nvar, "lunch.posy");
  if (themevar[0] != '\0')
    xalerts[2].sprite.xposy = atoi(themevar);
  else
    xalerts[2].sprite.xposy = 6;

  if (NULL != xalerts[2].sprite.hotspot)
    XDestroyImage(xalerts[2].sprite.hotspot);
  xalerts[2].sprite.hotspot = XGetImage(
      xdisplay, xalerts[2].sprite.mask, 0, 0, xalerts[2].sprite.xframewidth,
      xalerts[2].sprite.xframeheight, 1, ZPixmap);

  // skin
  themevar = GetThemeVar(var, nvar, "skin");
  path = (char *)realloc(
      path, (strlen(config.themedir) + strlen(base) + strlen(themevar) + 1) *
                sizeof(char));
  path = strcpy(path, config.themedir);
  path = strcat(path, base);
  path = strcat(path, themevar);

  if ((xrgbapixmap = XReadPNGPixmapFromFile(path)) != NULL) {
    xwidth = xrgbapixmap->width;
    xheight = xrgbapixmap->height;
    xbackgroundpixmap = xrgbapixmap->pixmap;
    xbackgroundpixmapmask = xrgbapixmap->mask;
    free(xrgbapixmap);
  } else {
    if (XCreatePixmapFromData(xdisplay, DefaultRootWindow(xdisplay),
                              xpmdefaultbackground, &xbackgroundpixmap,
                              &xbackgroundpixmapmask, &xpmattributes) == 0) {
      xwidth = xpmattributes.width;
      xheight = xpmattributes.height;
    } else
      tfprintf(stderr,
               "XpmReadFileToPixmap : unable to create pixmap from %s\n",
               themevar);
  }

  // pixmaps
  if (drawclippixmap != None)
    XFreePixmap(xdisplay, drawclippixmap);
  drawclippixmap =
      XCreatePixmap(xdisplay, DefaultRootWindow(xdisplay), xwidth, xheight, 1);

  // graphics context

  XGCValues xgcvalues;
  unsigned long xgcvalues_mask = GCGraphicsExposures;

  xgcvalues.graphics_exposures = false;

  if (NULL == gcset) {
    gcset = XCreateGC(xdisplay, drawclippixmap, xgcvalues_mask, &xgcvalues);
    XSetFunction(xdisplay, gcset, GXset);
  }
  if (NULL == gcclear) {
    gcclear = XCreateGC(xdisplay, drawclippixmap, xgcvalues_mask, &xgcvalues);
    XSetFunction(xdisplay, gcclear, GXclear);
  }
  XFillRectangle(xdisplay, drawclippixmap, gcclear, 0, 0, xwidth, xheight);

  // font
  if (NULL != xfont)
    XFreeFont(xdisplay, xfont);

  themevar = GetThemeVar(var, nvar, "text.font");
  if (themevar[0] == '\0')
    xfont = XLoadQueryFont(xdisplay, "*-12-*");
  else
    xfont = XLoadQueryFont(xdisplay, themevar);

  if (xfont == NULL)
    tfprintf(stderr, "XLoadQueryFont : cannot load font\n");
  else {
    XSetFont(xdisplay, gcdraw, xfont->fid);
    XSetFont(xdisplay, gcset, xfont->fid);
  }

  // foreground color
  XColor xcolor;

  Status xstatus;

  themevar = GetThemeVar(var, nvar, "text.color");
  if (themevar[0] != '\0') {
    if ((xstatus = XParseColor(xdisplay, xcolormap, themevar, &xcolor)) == 0)
      tfprintf(stderr, "XParseColor : unable to parse color %s\n", themevar);
  } else if ((xstatus = XParseColor(xdisplay, xcolormap, "#a1b7d9", &xcolor)) ==
             0)
    tfprintf(stderr, "XParseColor : unable to parse default color\n");

  if (xstatus != 0) {
    if (XAllocColor(xdisplay, xcolormap, &xcolor) == 0)
      tfprintf(stderr, "XParseColor : unable to allocate color %s\n", themevar);
  }

  XSetForeground(xdisplay, gcdraw, xcolor.pixel);

  FreeThemeVars(var, nvar);

  free(path);
}

Pair *ReadThemeVarsFromFile(const char *file, int &nvar) {

  nvar = 0;

  FILE *fp = fopen(file, "r");

  if (NULL == fp) {
    tfprintf(stderr, "fopen : %s\n", strerror(errno));
    return (NULL);
  }

  int c, ic, nc = 0, ivar = 10;

  char *p = NULL;

  Pair *var = (Pair *)malloc(ivar * sizeof(Pair));
  while ((c = fgetc(fp)) != EOF) {
    if (c == '#') {
      while ((c = fgetc(fp)) != EOF) {
        if (c == '\n')
          break;
      }
    } else if (c == '$') {

      if (++nvar == ivar) {
        ivar += 5;
        var = (Pair *)realloc(var, ivar * sizeof(Pair));
      }

      nc = 0;
      ic = 32;

      p = (char *)malloc(ic * sizeof(char));
      while (true) {
        if ((c = fgetc(fp)) == EOF || c == '\n') {
          p[nc] = '\0';
          var[nvar - 1].val = (char *)realloc(p, (nc + 1) * sizeof(char));
          break;
        } else if (c == '=') {
          p[nc] = '\0';
          var[nvar - 1].key = (char *)realloc(p, (nc + 1) * sizeof(char));
          nc = 0;
          ic = 32;
          p = (char *)malloc(ic * sizeof(char));
        } else if (c == '\\') {
          while ((c = fgetc(fp)) != EOF)
            if (c == '\n')
              break;
        } else if (c != ' ' && c != '\"') {
          if (++nc > ic) {
            ic += 16;
            p = (char *)realloc(p, ic * sizeof(char));
          }
          p[nc - 1] = c;
        }
      }
    }
  }
  fclose(fp);

  var = (Pair *)realloc(var, nvar * sizeof(Pair));

  qsort(var, nvar, sizeof(Pair), PairCmp);

  return (var);
}

char *GetThemeVar(Pair *var, int nvar, const char *key) {

  Pair needle, *result;
  needle.key = (char *)key;
  needle.val = NULL;

  result = (Pair *)bsearch(&needle, var, nvar, sizeof(Pair), PairCmp);

  if (NULL == result)
    return ("");

  return (result->val);
}

void FreeThemeVars(Pair *var, int nvar) {

  for (int i = 0; i < nvar; i++) {
    free(var[i].key);
    free(var[i].val);
  }
  free(var);

  var = NULL;
}

int PairCmp(const void *c1, const void *c2) {
  return (strcmp(((Pair *)c1)->key, ((Pair *)c2)->key));
}

Message *AddMessage(Message *messages, const char *text, int expired) {

  Message *message = (Message *)malloc(sizeof(_Message));

  if (NULL != messages)
    messages->previous = message;

  message->previous = NULL;
  message->next = messages;

  message->text = (char *)malloc((strlen(text) + 1) * sizeof(char));
  message->text = strcpy(message->text, text);

  message->expired = expired;

  messages = message;

  return (messages);
}

Message *DelMessage(Message *messages, Message *message) {

  if (message->text != NULL)
    free(message->text);

  if (NULL != message->next)
    message->next->previous = message->previous;
  if (NULL != message->previous)
    message->previous->next = message->next;

  if (messages == message)
    messages = message->next;

  free(message);

  return (messages);
}

Message *FreeMessages(Message *messages) {

  Message *message;
  while (messages != NULL) {
    if (messages->text != NULL)
      free(messages->text);
    message = messages;
    messages = messages->next;
    free(message);
  }
  return (NULL);
}

void messageGUIstart(char *xapplicationname, char *xapplicationclass,
                     Pixmap xiconpixmap, Pixmap xiconpixmapmask) {

  Widget form, messagetb, closebutton, sendbutton;

  messagewidget = XtVaAppCreateShell(
      xapplicationname, xapplicationclass, topLevelShellWidgetClass, xdisplay,
      XmNdeleteResponse, XmUNMAP, XmNmappedWhenManaged, false, NULL);

  XtVaSetValues(messagewidget, XmNiconPixmap, xiconpixmap, XmNiconMask,
                xiconpixmapmask, NULL);

  form = XtVaCreateManagedWidget("FormMessage", xmFormWidgetClass,
                                 messagewidget, XmNwidth, 450, XmNheight, 400,
                                 XmNshadowType, XmSHADOW_IN, NULL);

  XtAddEventHandler(messagewidget, StructureNotifyMask, false,
                    messageGUIEventPos, &config);

  messagetb = XtVaCreateManagedWidget("Message", xmTextFieldWidgetClass, form,
                                      XmNwidth, 350, XmNmaxLength, 127,
                                      XmNleftAttachment, XmATTACH_FORM, NULL);

  XtAddCallback(messagetb, XmNactivateCallback, messageGUIsend, NULL);

  sendbutton = XtVaCreateManagedWidget(
      "Send", xmPushButtonWidgetClass, form, XmNwidth, 50, XmNtopAttachment,
      XmATTACH_FORM, XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget,
      messagetb, NULL);

  XtAddCallback(sendbutton, XmNactivateCallback, messageGUIsend, messagetb);

  closebutton = XtVaCreateManagedWidget(
      "Close", xmPushButtonWidgetClass, form, XmNwidth, 50, XmNtopAttachment,
      XmATTACH_FORM, XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget,
      sendbutton, NULL);

  XtAddCallback(closebutton, XmNactivateCallback, messageGUIclose,
                &messagewidget);

  Arg arglist[7];

  XtSetArg(arglist[0], XmNwidth, 350);
  XtSetArg(arglist[1], XmNheight, 365);
  XtSetArg(arglist[2], XmNeditable, false);
  XtSetArg(arglist[3], XmNeditMode, XmMULTI_LINE_EDIT);
  XtSetArg(arglist[4], XmNcursorPositionVisible, false);
  XtSetArg(arglist[5], XmNwordWrap, true);
  XtSetArg(arglist[6], XmNscrollHorizontal, false);

  messagetext = XmCreateScrolledText(form, "MessageText", arglist, 7);

  XtVaSetValues(XtParent(messagetext), XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, messagetb, XmNbottomAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM, NULL);

  XtManageChild(messagetext);

  XtSetArg(arglist[0], XmNwidth, 100);

  clientstext = XmCreateScrolledText(form, "ClientsText", arglist, 7);

  XtVaSetValues(XtParent(clientstext), XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, messagetb, XmNbottomAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget, messagetext,
                NULL);

  XmTextSetString(clientstext, clients);

  XtManageChild(clientstext);

  XtRealizeWidget(messagewidget);
}

void messageGUIsend(_WidgetRec *w, XtPointer clientdata, void *cbs) {

  char *textfieldstring = NULL;
  if (NULL == clientdata) {
    textfieldstring = XmTextFieldGetString(w);
    XmTextFieldSetString(w, "");
  } else {
    textfieldstring = XmTextFieldGetString((Widget)clientdata);
    XmTextFieldSetString((Widget)clientdata, "");
  }

  if (connection == CONNECTION_OPENED) {
    pckt_send.type = IM;
    pckt_send.timestamp = time(NULL);
    strcpy(pckt_send.nick, config.nick);
    strncpy(pckt_send.msg, textfieldstring, 127);
    if (send(sockfd, &pckt_send, sizeof(struct _PACKET), 0) == -1)
      tfprintf(stderr, "send : %s\n", strerror(errno));
  }

  XtFree(textfieldstring);
}

void messageGUIclose(_WidgetRec *w, XtPointer clientdata, void *cbs) {
  XtUnmapWidget(*((Widget *)clientdata));
}

void messageGUIEventPos(Widget w, XtPointer clientdata, XEvent *xevent,
                        Boolean *continue_to_dispatch) {

  if (xevent->type == ConfigureNotify) {
    ((configStruct *)clientdata)->xmessageposx = xevent->xconfigure.x;
    ((configStruct *)clientdata)->xmessageposy = xevent->xconfigure.y;
  }
}

XRGBAPixmap *XReadPNGPixmapFromFile(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    tfprintf(stderr, "fopen : %s\n", strerror(errno));
    return (NULL);
  }

  png_byte header[8];
  fread(header, sizeof(png_byte), 8, fp);
  if (!png_check_sig(header, 8)) {
    fclose(fp);
    tfprintf(stderr, "png_check_sig : not a PNG file\n");
    return (NULL);
  }

  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    tfprintf(stderr, "png_create_read_struct\n");
    return (NULL);
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    tfprintf(stderr, "png_create_info_struct\n");
    fclose(fp);
    return (NULL);
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    tfprintf(stderr, "png_create_info_struct\n");
    fclose(fp);
    return (NULL);
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    tfprintf(stderr, "setjmp\n");
    fclose(fp);
    return (NULL);
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               &interlace_type, int_p_NULL, int_p_NULL);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand(png_ptr);
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_expand(png_ptr);

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);
  png_set_invert_alpha(png_ptr);

  double gamma, display_exponent = 2.2 * 1.0;

  if (png_get_gAMA(png_ptr, info_ptr, &gamma))
    png_set_gamma(png_ptr, display_exponent, gamma);

  if ((color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
      DefaultDepth(xdisplay, DefaultScreen(xdisplay)))
    png_set_bgr(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  unsigned char *image_data;
  if ((image_data = (unsigned char *)malloc(rowbytes * height *
                                            sizeof(unsigned char))) == NULL) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    tfprintf(stderr, "malloc : %s\n", strerror(errno));
    fclose(fp);
    return (NULL);
  }

  png_bytep *row_pointers;

  if ((row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep))) ==
      NULL) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return (NULL);
  }

  for (png_uint_32 i = 0; i < height; ++i)
    row_pointers[i] = image_data + i * rowbytes;

  png_read_image(png_ptr, row_pointers);

  png_read_end(png_ptr, end_info);

  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  delete row_pointers;

  fclose(fp);

  XImage *ximage =
      XCreateImage(xdisplay, DefaultVisual(xdisplay, DefaultScreen(xdisplay)),
                   DefaultDepth(xdisplay, DefaultScreen(xdisplay)), ZPixmap, 0,
                   (char *)image_data, width, height, bit_depth, rowbytes);

  if (ImageByteOrder(xdisplay) != LSBFirst)
    ximage->byte_order = LSBFirst;

  XRGBAPixmap *xrgbapixmap = (XRGBAPixmap *)malloc(sizeof(struct _XRGBAPixmap));
  xrgbapixmap->width = ximage->width;
  xrgbapixmap->height = ximage->height;
  xrgbapixmap->pixmap = XCreatePixmap(
      xdisplay, DefaultRootWindow(xdisplay), ximage->width, ximage->height,
      DefaultDepth(xdisplay, DefaultScreen(xdisplay)));
  xrgbapixmap->mask = XCreatePixmap(xdisplay, DefaultRootWindow(xdisplay),
                                    ximage->width, ximage->height, 1);
  XPutImage(xdisplay, xrgbapixmap->pixmap,
            DefaultGC(xdisplay, DefaultScreen(xdisplay)), ximage, 0, 0, 0, 0,
            ximage->width, ximage->height);

  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {

    XGCValues xgcvalues;
    xgcvalues.function = GXcopy;
    xgcvalues.graphics_exposures = false;
    xgcvalues.foreground = 1;
    xgcvalues.plane_mask = AllPlanes;
    unsigned long xgcvalues_mask =
        GCFunction | GCGraphicsExposures | GCForeground | GCPlaneMask;

    GC alphaGC =
        XCreateGC(xdisplay, xrgbapixmap->mask, xgcvalues_mask, &xgcvalues);

    XFillRectangle(xdisplay, xrgbapixmap->mask, alphaGC, 0, 0, ximage->width,
                   ximage->height);
    XSetForeground(xdisplay, alphaGC, 0);

    char alpha;
    for (int y = 0; y < ximage->height; y++) {
      for (int x = 0; x < ximage->width; x++) {
        alpha = ximage->data[3 + (y * ximage->width + x) * 4];
        if (alpha < 0)
          XDrawPoint(xdisplay, xrgbapixmap->mask, alphaGC, x, y);
      }
    }
    XFreeGC(xdisplay, alphaGC);
  } else
    xrgbapixmap->mask = None;

  XDestroyImage(ximage);

  return (xrgbapixmap);
}

void sighandler(int sig) {

  switch (sig) {

  case SIGHUP:
    tfprintf(stderr, "Caught SIGHUP (%d) : terminating\n", sig);
    break;
  case SIGINT:
    tfprintf(stderr, "Caught SIGINT (%d) : terminating\n", sig);
    break;
  case SIGQUIT:
    tfprintf(stderr, "Caught SIGQUIT(%d) : terminating\n", sig);
    break;
  case SIGTERM:
    tfprintf(stderr, "Caught SIGTERM(%d) : terminating\n", sig);
    break;
  case SIGABRT:
    tfprintf(stderr, "Caught SIGABRT(%d) : terminating\n", sig);
    break;
  case SIGSEGV:
    tfprintf(stderr, "Caught SIGSEGV(%d) : terminating\n", sig);
    break;
  case SIGPIPE:
    tfprintf(stderr, "Caught SIGPIPE(%d) : terminating\n", sig);
    break;
  case SIGBUS:
    tfprintf(stderr, "Caught SIGBUS(%d) : terminating\n", sig);
    break;
  default:
    tfprintf(stderr, "Caught undefined signal %d: terminating\n", sig);
  };

  FILE *fpc = fopen(configfile, "w");

  if (fpc != NULL) {
    fwrite(&config, sizeof(configStruct), 1, fpc);
    tfprintf(stderr, "WRITING SECURE config file : %s\n", configfile);
  } else
    tfprintf(stderr, "fopen : %s\n", strerror(errno));

  fclose(fpc);

  tfprintf(stderr, "Client has DIRTY exit\n");

  exit(sig);
}

char *retrieveURL(const char *url, int &total_bytes_recv, short port) {

  total_bytes_recv = 0;

  struct protoent *protocol = getprotobyname("tcp");
  if (protocol == NULL)
    return (NULL);

  int sockfd = socket(AF_INET, SOCK_STREAM, protocol->p_proto);
  if (sockfd == -1)
    return (NULL);

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    return (NULL);

  int opt;
  if ((opt = fcntl(sockfd, F_GETFL, NULL)) < 0)
    return (NULL);

  if (fcntl(sockfd, F_SETFL, opt | O_NONBLOCK) < 0)
    return (NULL);

  struct sockaddr_in local, remote;

  local.sin_family = AF_INET;
  local.sin_port = htons(0);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&local.sin_zero, '\0', 8);
  if (bind(sockfd, (struct sockaddr *)&local, sizeof(struct sockaddr)) == -1)
    return (NULL);

  remote.sin_family = AF_INET;
  remote.sin_port = htons(port);

  const char *file = strchr(url, '/');
  char *host = (char *)malloc(sizeof(char) * (strlen(url) - strlen(file) + 1));

  if (host == NULL) {
    free(host);
    return (NULL);
  }

  host[strlen(url) - strlen(file)] = '\0';
  strncpy(host, url, strlen(url) - strlen(file));
  struct hostent *remote_host = gethostbyname(host);
  if (remote_host == NULL) {
    free(host);
    herror("gethostbyname");
    return (NULL);
  }
  remote.sin_addr = *((struct in_addr *)remote_host->h_addr);
  memset(&remote.sin_zero, '\0', 8);

  if (connect(sockfd, (struct sockaddr *)&remote, sizeof(struct sockaddr)) ==
      -1) {
    if (errno == EINPROGRESS) {
      struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
      fd_set fdset;
      FD_ZERO(&fdset);
      FD_SET(sockfd, &fdset);
      if (select(sockfd + 1, NULL, &fdset, NULL, &timeout) <= 0) {
        if (fcntl(sockfd, F_SETFL, opt) < 0)
          return (NULL);
      }
    }
  }

  if (fcntl(sockfd, F_SETFL, opt) < 0)
    return (NULL);

  char *method = "GET ", *version = " \r\nHTTP/1.1\r\n\r\n",
       *msg = (char *)malloc(
           sizeof(char) * (strlen(method) + strlen(file) + strlen(version)) +
           1);
  if (msg == NULL)
    return (NULL);
  strcpy(msg, method);
  strcat(msg, file);
  strcat(msg, version);

  int bytes_to_send = strlen(msg), bytes_send, total_bytes_send = 0;

  while (total_bytes_send < bytes_to_send) {
    bytes_send = send(sockfd, msg + total_bytes_send,
                      bytes_to_send - total_bytes_send, 0);
    if (bytes_send == -1)
      return (NULL);
    total_bytes_send += bytes_send;
  }

  char *target, buff[BUFFLEN];

  int bytes_recv;
  total_bytes_recv = 0;

  target = (char *)malloc(sizeof(char));
  if (target == NULL)
    return (NULL);

  *target = '\0';
  while ((bytes_recv = recv(sockfd, buff, BUFFLEN - 1, 0))) {
    if (bytes_recv == -1)
      return (NULL);

    total_bytes_recv += bytes_recv;
    buff[bytes_recv] = '\0';

    target = (char *)realloc(target, sizeof(char) * (total_bytes_recv + 1));
    if (target == NULL)
      return (NULL);
    strcat(target, buff);
  }

  free(host);
  free(msg);

  close(sockfd);

  return (target);
}
