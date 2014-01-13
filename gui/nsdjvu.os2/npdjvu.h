/*
** Module   :NPDJVU.H
** Abstract :Main header file for the plugin
**
** Copyright (C) Sergey I. Yevtushenko
**
** This software is subject to, and may be distributed under, the
** GNU General Public License, Version 2. The license should have
** accompanied the software or you may obtain a copy of the license
** from the Free Software Foundation at http://www.fsf.org .
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**----------------------------------------------------------------
**
** Log: Sun  30/11/2003 Created
**      Sat  13/12/2003 Added frame subclassing
**      Sat  13/12/2003 Added download progress indication
**
*/

#ifndef __NPDJVU_H
#define __NPDJVU_H

//-----------------------------------------
// Version constants (used by MKVER.CMD)
//-----------------------------------------

#define VER_MAJOR       0
#define VER_MINOR       9
#define VER_BUILD       3
#define VER_PREFIX      "@#ES:"
#define VER_SUFFIX      "#@ NPDJVU Mozilla/Netscape plugin"

#define MK_S(a)         #a
#define MK_STR(a,b,c)   MK_S(a)"."MK_S(b)"."MK_S(c)
#define VERSION         "Version "MK_STR(VER_MAJOR,VER_MINOR,VER_BUILD)" ("__DATE__" "__TIME__")"

//-----------------------------------------
// Constants
//-----------------------------------------

#define SCROLL_DELTA_X  16  	/* step for vertical scrolling */
#define SCROLL_DELTA_Y  16      /* step for horizontal scrolling */
#define KBD_ZOOM_DELTA  5       /* step for zoom control */
#define SCROLL_DELTA_Y  16      /* step for horizontal scrolling */
#define MIN_ZOOM        10      /* zoom limits */
#define MAX_ZOOM        200
#define WIN_ZOOM_SZ     70
#define WIN_PAGE_SZ     70
#define TOOLBAR_SZ      22      /* should match icon Y size */

//-----------------------------------------
// Window IDs
//-----------------------------------------

#define FID_PANE        255
#define FID_CTXMENU     260
#define FID_CZOOM       2048
#define FID_CPAGE       2049

//-----------------------------------------
// Bitmap IDs
//-----------------------------------------

#define BID_FIRST       1001
#define BID_BCK10       1002
#define BID_BCK01       1003
#define BID_FWD01       1004
#define BID_FWD10       1005
#define BID_LAST        1006

//-----------------------------------------
// Command IDs
//-----------------------------------------

#define CMD_OK          1001

#define CMD_FIRST       100
#define CMD_BCK10       101
#define CMD_BCK01       102
#define CMD_FWD01       103
#define CMD_FWD10       104
#define CMD_LAST        105

#define CMD_SAVE_ZOOM   120
#define CMD_BW_MODE     121
#define CMD_SAVEAS      122
#define CMD_ABOUT       123

#ifndef RC_INVOKED

//----------------------------------------------------------------------------
// Document-specific data
//
// Note: they are not stored in PluginInstance (see below)
//       just because this structure must be initialized/destroyed
//       properly from the point of view of C++
//----------------------------------------------------------------------------

class DocData
{
        int ready;
        HWND hWnd;

        int page;

        double scale;

        int aspect;
        int subsample;

    public:

        int bug;
        int start_x;
        int start_y;

        int pr_pos;
        int pr_size;

        GP<DjVuDocument> doc;
        GP<DjVuImage>    image;

        GRect segmentrect;
        GRect fullrect;

        DocData():ready(0),hWnd(0)                  { clear();}
        ~DocData()                                  { clear();}

        int isReady()                               { return ready;}

        void clear();
        void resetVars();
        int initDoc(char*);
        void setHWND(HWND wnd)                      { hWnd = wnd;}
        HWND getHWND()                              { return hWnd;}

        int setPage(int page_num, int reset_y = 1);
        int getPage()                               { return page;}
        int getNumPages()                           { return (ready) ?
                                                            (doc->get_pages_num() - 1):0;}
        void drawPicture(HPS hBigPS, POINTL& endPoint, BOOL isScreen);
        void setScrollbars();
        void setPageNum();
        void setZoom(int);
        void setProgressInfo(int32 pos, int32 size);
};

//----------------------------------------------------------------------------
// Instance state information about the plugin.
//----------------------------------------------------------------------------

typedef struct _PluginInstance PluginInstance;
typedef struct _PluginInstance
{
    NPWindow*       fWindow;
    HWND            hWnd;
    HWND            hWndZoom;
    HWND            hWndPage;
    uint16          fMode;
    PFNWP           pOldProc;
    NPSavedData*    pSavedInstanceData;
    PluginInstance* pNext;
    DocData*        Doc;
} PluginInstance;

//----------------------------------------------------------------------------
// Menu definition
//----------------------------------------------------------------------------

typedef struct
{
    int id;
    char* text;
}
USERMENU;
typedef USERMENU* PUSERMENU;

//----------------------------------------------------------------------------
// Function prototypes
//----------------------------------------------------------------------------

MRESULT APIENTRY FrameProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT APIENTRY ClientProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT APIENTRY PageProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2);

int SetScrollBar(HWND hBar, int curr, int port, int max);
void FillMenu(HWND hMenu, PUSERMENU pList, int start);
void SetupWindow(PluginInstance* This);

#endif

#endif  /*__NPDJVU_H*/

