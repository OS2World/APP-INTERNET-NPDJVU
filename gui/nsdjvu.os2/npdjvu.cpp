/*
** Module   :NPDJVU.CPP
** Abstract :NPDJVU Mozilla plugin for OS/2 main module
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
** Log: Tue  02/12/2003 Created
**      Sat  13/12/2003 Added frame subclassing
**      Sat  13/12/2003 Added download progress indication
**      Sat  13/12/2003 Added keyboard processing
*/

#define INCL_WIN
#define INCL_DOSPROCESS
#define INCL_GPI
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSMODULEMGR
#include <os2.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#ifndef _NPAPI_H_
#include "npapi.h"
#endif

#ifdef __GNUG__
#pragma implementation
#endif
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "GException.h"
#include "GSmartPointer.h"
#include "GRect.h"
#include "GPixmap.h"
#include "GBitmap.h"
#include "DjVuImage.h"
#include "DjVuDocument.h"
#include "DjVuPalette.h"
#include "GOS.h"
#include "ByteStream.h"
#include "DjVuMessage.h"

#include <locale.h>

#include "npdjvu.h"

#define __TR__  fprintf(stderr, "%s %d\n", __FUNCTION__, __LINE__);fflush(stderr);

#define min(a,b)    ((a < b) ? (a):(b))
#define max(a,b)    ((a > b) ? (a):(b))

//----------------------------------------------------------------------------
// Static data
//----------------------------------------------------------------------------

static HMODULE hMod = 0;
static int BWmode   = 0;
static int SaveZoom = 0;
static int Zoom     = 100;

USERMENU mCtl[] =
{
    {CMD_FIRST, "#1001"},
//    {CMD_BCK10, "#1002"},
    {CMD_BCK01, "#1003"},
    {CMD_FWD01, "#1004"},
//    {CMD_FWD10, "#1005"},
    {CMD_LAST , "#1006"},
    {0}
};

//----------------------------------------------------------------------------
// DocData methods
//----------------------------------------------------------------------------

void DocData::resetVars()
{
    aspect  = subsample = -1;
    page    = start_x = start_y = 0;
    scale   = 0;
}

void DocData::clear()
{
    doc     = 0;
    image   = 0;
    ready   = 0;
    bug     = 0;
    pr_pos  = 0;
    pr_size = 0;
    resetVars();
}

void DocData::setScrollbars()
{
    HWND hBar;

    hBar = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), FID_VERTSCROLL);

    int pos_y = (fullrect.height() - segmentrect.height()) - start_y;

    if(pos_y < 0)
        pos_y = 0;

    SetScrollBar(hBar, pos_y, segmentrect.height(), fullrect.height());

    hBar = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), FID_HORZSCROLL);
    SetScrollBar(hBar, start_x, segmentrect.width(), fullrect.width());
}

void DocData::setPageNum()
{
    int curr  = 0;
    int total = 0;

    if(ready)
    {
        curr  = page + 1;
        total = doc->get_pages_num();
    }

    HWND hPage = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), FID_CPAGE);

    WinSendMsg(hPage, SPBM_SETLIMITS, (MPARAM)total, (MPARAM)1);
    WinSendMsg(hPage, SPBM_SETCURRENTVALUE, (MPARAM)curr, 0);

    HWND hZoom = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), FID_CZOOM);
    WinSendMsg(hZoom, SPBM_SETCURRENTVALUE, (MPARAM)int(scale), 0);
}

void DocData::setZoom(int percent)
{
    if(!doc || !ready)
        return;

    if(percent < MIN_ZOOM)
        percent = MIN_ZOOM;
    if(percent > MAX_ZOOM)
        percent = MAX_ZOOM;

    scale = percent;

    start_x = 0;

    setPage(page, 1);
}

int DocData::setPage(int page_num, int reset_y)
{
    if(!doc || !ready)
        return 0;

    if(page_num < 0)
        page_num = 0;

    if(page_num >= doc->get_pages_num())
        page_num = doc->get_pages_num() - 1;

    try
    {
        image = doc->get_page(page_num);
    }
    catch(...)
    {
fprintf(stderr, "unknown excepion during get_page\n");
//        clear();
//        bug = 1;
        return 0;
    }

    int rc = 0;

    do
    {
        if(!image || !image->wait_for_complete_decode())
        {
            rc = -2;
            break;
        }

        page = page_num;

        // Setup rectangles

        DjVuInfo *info = image->get_info();

        if(scale <= 0 && subsample < 0)
            scale = 100;

        if(subsample > 0)
            scale = (double) info->dpi / subsample;

        if(scale > 0)
        {
            int w = (int)(image->get_width() * scale / info->dpi);
            int h = (int)(image->get_height() * scale / info->dpi);

            if(w < 1)
                w = 1;
            if(h < 1)
                h = 1;

            fullrect = GRect(0,0, w, h);
        }

        if(aspect <= 0)
        {
            int w = fullrect.width();
            int h = fullrect.height();

            double dw = (double)image->get_width()/ w;
            double dh = (double)image->get_height()/h;

            if(dw > dh)
                h = (int)(image->get_height()/dw);
        	else
                w = (int)(image->get_width()/dh);

            fullrect = GRect(0,0, w, h);
        }

        if(reset_y)
        {
        	int pos_y = (fullrect.height() - segmentrect.height());

        	if(pos_y < 0)
            	pos_y = 0;

            start_y = pos_y;
        }

        setScrollbars();
        setPageNum();
        WinInvalidateRect(hWnd, 0, TRUE);
    }
    while(0);

    return rc;
}

int DocData::initDoc(char* cName)
{
    resetVars();

    if(SaveZoom)
        scale = Zoom;

    int rc = 0;

    do
    {
        try
        {
            doc = DjVuDocument::create_wait(GURL::Filename::UTF8(cName));

	        if(!doc->wait_for_complete_init())
            {
                rc = NPERR_GENERIC_ERROR;
    	        break;
            }
        }
	    catch(const GException &msg)
        {
fprintf(stderr, "excepion (create_wait): %s\n", msg.get_cause());
            rc = NPERR_GENERIC_ERROR;
            break;
        }
        catch(...)
        {
fprintf(stderr, "unknown excepion during create_wait\n");
            rc = NPERR_GENERIC_ERROR;
            break;
        }

        ready = 1;

        rc = setPage(0);

        WinPostMsg(hWnd, WM_BUTTON1DOWN, 0, 0);
    }
    while(0);

    if(rc)
        clear();

    return ready ? NPERR_NO_ERROR:NPERR_GENERIC_ERROR;
}

void DocData::drawPicture(HPS hBigPS, POINTL& endPoint, BOOL isScreen)
{
    //Ignore shifts for the printing
    int s_x = (isScreen) ? start_x:0;
    int s_y = (isScreen) ? start_y:0;

    GP<GPixmap> pm;

	if((s_x + min(segmentrect.width(), fullrect.width())) > fullrect.width())
		s_x = 0;

	if((s_y + min(segmentrect.height(),fullrect.height())) > fullrect.height())
		s_y = 0;

    GRect seg(s_x, s_y,
              min(segmentrect.width(), fullrect.width()),
              min(segmentrect.height(),fullrect.height()));

#if 0
fprintf(stderr, "seg(%d, %d, %d, %d), s_x + w = %d (%d), s_y + h = %d (%d)\n",
					s_x, s_y, seg.width(), seg.height(),
					s_x + seg.width(), fullrect.width(),
					s_y + seg.height(), fullrect.height());
fprintf(stderr, "full(%d, %d, %d, %d)\n", 0, 0, fullrect.width(), fullrect.height());
#endif

    try
    {
        //Don't create image for BW mode and let next block to handle this
        if(!BWmode)
        {
            pm = image->get_pixmap(seg, fullrect);
        }

        //Something wrong: either we're unable to create color image
        //or we just in BW mode. Handle this case
        if(!pm)
        {
            GP<GBitmap> bm;

            bm = image->get_bitmap(seg, fullrect);

            if(!bm) //Error
                return;

            pm = GPixmap::create(*bm);
        }
    }
	catch(const GException &msg)
    {
fprintf(stderr, "excepion (draw_page): %s\n", msg.get_cause());
        return;
    }
	catch(...)
    {
fprintf(stderr, "unknown excepion during create_wait\n");
        return;
    }

    static DEVOPENSTRUC dop = {0L, (PSZ)"DISPLAY", NULL,
                               0L, 0L, 0L, 0L, 0L, 0L};

    HDC hdc = 0;
    HPS hps = 0;
    HBITMAP hbm = 0;

    do
    {
        hdc = DevOpenDC(0, OD_MEMORY, (PSZ)"*", 5L, (PDEVOPENDATA)&dop, 0);

        if(!hdc)
            break;

        SIZEL sizl = {0};

        hps = GpiCreatePS(0, hdc, &sizl, PU_PELS | GPIT_MICRO | GPIA_ASSOC);

        if(!hps)
            break;

        BITMAPINFOHEADER2 bmhdr = {0};
        BITMAPINFO2       dhdr  = {0};

        bmhdr.cbFix     = sizeof(BITMAPINFOHEADER2);
        bmhdr.cx        = max(segmentrect.width(), fullrect.width()) + 16;
        bmhdr.cy        = max(segmentrect.height(), fullrect.height()) + 16;
        bmhdr.cPlanes   = 1;
        bmhdr.cBitCount = 24;

    	dhdr.cbFix     = 16;
        dhdr.cx        = seg.width();
        dhdr.cy        = 1;
        dhdr.cPlanes   = 1;
        dhdr.cBitCount = 24;

        hbm = GpiCreateBitmap(hps, &bmhdr, 0, 0, 0);

        if(!hbm)
            break;

        GpiSetBitmap(hps, hbm);

        POINTL aptl[4];

        aptl[0].x = 0;
        aptl[0].y = 0;
        aptl[1].x = segmentrect.width();
        aptl[1].y = segmentrect.height();
        aptl[2].x = 0;
        aptl[2].y = 0;
        aptl[3].x = bmhdr.cx;
        aptl[3].y = bmhdr.cy;

        GpiMove(hps, aptl);
        GpiSetColor(hps, CLR_PALEGRAY);
        GpiBox(hps, DRO_OUTLINEFILL, &aptl[3], 0L, 0L);

        for(int i = 0; i < seg.height(); i++)
            GpiSetBitmapBits(hps, i, 1, (PBYTE)pm->operator[](i), &dhdr);

        GpiBitBlt(hBigPS, hps, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE);
    }
    while(0);

    if(hps)
    {
        GpiSetBitmap(hps, 0);
        GpiDestroyPS(hps);
    }

    if(hbm)
        GpiDeleteBitmap(hbm);

    if(hdc)
        DevCloseDC(hdc);
}

void DocData::setProgressInfo(int32 pos, int32 size)
{
    if(pr_size > 0 && pr_size == size)
    {
        //Avoid redundant repainting
        if(int(double(pr_pos)/double(pr_size)*100) ==
           int(double(pos)/double(size)*100))
        {
            return;
        }
    }

    pr_pos  = pos;
    pr_size = size;

    WinInvalidateRect(hWnd, 0, FALSE);
}

//----------------------------------------------------------------------------
// Memory management
//----------------------------------------------------------------------------

void* operator new(size_t size)
{
    return NPN_MemAlloc(size);
}

void operator delete(void* p)
{
    NPN_MemFree(p);
}

//----------------------------------------------------------------------------
// NPP_Initialize:
//----------------------------------------------------------------------------

NPError NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

//----------------------------------------------------------------------------
// NPP_Shutdown:
//----------------------------------------------------------------------------
void NPP_Shutdown(void)
{
}

//----------------------------------------------------------------------------
// NPP_New:
//----------------------------------------------------------------------------
NPError NP_LOADDS NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode,
                          int16 argc, char* argn[], char* argv[], NPSavedData* saved)
{
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));
    PluginInstance* This = (PluginInstance*) instance->pdata;

    if (This == NULL)
        return NPERR_OUT_OF_MEMORY_ERROR;

    This->fWindow = 0;
    // mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h)
    This->fMode = mode;
    This->hWnd = 0;
    This->pSavedInstanceData = saved;
    This->pNext = 0;
    This->Doc   = new DocData;

    return NPERR_NO_ERROR;
}


//-----------------------------------------------------------------------------
// NPP_Destroy:
//-----------------------------------------------------------------------------
NPError NP_LOADDS NPP_Destroy(NPP instance, NPSavedData** save)
{
    if (instance == 0)
        return NPERR_INVALID_INSTANCE_ERROR;

    PluginInstance* This = (PluginInstance*) instance->pdata;

    if(!This)
        return NPERR_NO_ERROR;

    // Remove the subclass for the frame window

    if(This->Doc && This->Doc->getHWND())
    {
        HWND hFrame = WinQueryWindow(This->Doc->getHWND(), QW_PARENT);

        if(hFrame)
            WinSetWindowULong(hFrame, QWL_USER, 0);
    }

    delete This->Doc;
    This->Doc = 0;

    // make some saved instance data if necessary
    if(This->pSavedInstanceData == 0)
    {
        // make a struct header for the data
        This->pSavedInstanceData =
            (NPSavedData*)NPN_MemAlloc(sizeof (struct _NPSavedData));

        // fill in the struct
        if(This->pSavedInstanceData != 0)
        {
            This->pSavedInstanceData->len = 0;
            This->pSavedInstanceData->buf = 0;

            // replace the def below and references to it with your data
            #define SIDATA "aSavedInstanceDataBlock"

            // the data
            This->pSavedInstanceData->buf = NPN_MemAlloc(sizeof SIDATA);

            if(This->pSavedInstanceData->buf != 0)
            {
                strcpy((char*)This->pSavedInstanceData->buf, SIDATA);
                This->pSavedInstanceData->len = sizeof SIDATA;
            }
        }
    }

    // save some instance data
    *save = This->pSavedInstanceData;

    NPN_MemFree(instance->pdata);

    instance->pdata = 0;

    return NPERR_NO_ERROR;
}

//----------------------------------------------------------------------------
// NPP_SetWindow:
//----------------------------------------------------------------------------

NPError NP_LOADDS NPP_SetWindow(NPP instance, NPWindow* window)
{
    if (instance == 0)
        return NPERR_INVALID_INSTANCE_ERROR;

    PluginInstance* This = (PluginInstance*) instance->pdata;

    if(!This)
        return NPERR_NO_ERROR;

    if(window->window && !This->hWnd)
    {
        This->fWindow = window;
        This->hWnd    = (HWND)This->fWindow->window;
        SetupWindow(This);
    }
    else
    {
        if(This->hWnd != (HWND)window->window)
        {
            This->fWindow = window;
            This->hWnd = (HWND)This->fWindow->window;

            if(This->hWnd)
            {
                SetupWindow(This);
            }
        }
    }

    if(This->hWnd)
    {
        SWP swp;

        WinQueryWindowPos(This->hWnd, &swp);

        WinSetWindowPos(WinQueryWindow(This->Doc->getHWND(), QW_PARENT),
                        0, 0, 0, swp.cx, swp.cy, SWP_MOVE | SWP_SIZE);

        WinQueryWindowPos(This->Doc->getHWND(), &swp);

        This->Doc->segmentrect = GRect(0,0, swp.cx, swp.cy);

        if(This->Doc->isReady())
            This->Doc->setPage(This->Doc->getPage());

        This->Doc->setScrollbars();
        WinInvalidateRect(This->Doc->getHWND(), 0, TRUE);

        WinSetFocus(HWND_DESKTOP, This->Doc->getHWND());
    }

    return NPERR_NO_ERROR;
}

//----------------------------------------------------------------------------
// NPP_NewStream:
//----------------------------------------------------------------------------

NPError NP_LOADDS NPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream,
                                NPBool seekable, uint16 *stype)
{
    if (instance == 0)
        return NPERR_INVALID_INSTANCE_ERROR;

    *stype = NP_ASFILE;

    PluginInstance* This = (PluginInstance*) instance->pdata;
    This->Doc->setProgressInfo(0, stream->end);

    return 0;
}


int32 STREAMBUFSIZE = 0X07FFF;      // If we are reading from a file in
                                    // NP_ASFILE mode, we can take any size
                                    // stream in our write call (since we
                                    // ignore it)

//----------------------------------------------------------------------------
// NPP_WriteReady:
//----------------------------------------------------------------------------
int32 NP_LOADDS NPP_WriteReady(NPP instance, NPStream *stream)
{
    return STREAMBUFSIZE;   // Number of bytes ready to accept in NPP_Write()
}

//----------------------------------------------------------------------------
// NPP_Write:
//----------------------------------------------------------------------------
int32 NP_LOADDS NPP_Write(NPP instance, NPStream *stream,
                          int32 offset, int32 len, void *buffer)
{
    if (instance == 0)
        return NPERR_INVALID_INSTANCE_ERROR;

//fprintf(stderr, "%s %d offset = %d, len = %d, stream->end = %d\n", __FUNCTION__, __LINE__,
//        offset, len, stream->end);
//fflush(stderr);

    PluginInstance* This = (PluginInstance*) instance->pdata;

    This->Doc->setProgressInfo(offset + len, stream->end);

    return len;
}


//----------------------------------------------------------------------------
// NPP_DestroyStream:
//----------------------------------------------------------------------------
NPError NP_LOADDS NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
    if (instance == 0)
        return NPERR_INVALID_INSTANCE_ERROR;

    return NPERR_NO_ERROR;
}


//----------------------------------------------------------------------------
// NPP_StreamAsFile:
//----------------------------------------------------------------------------
void NP_LOADDS NPP_StreamAsFile(NPP instance, NPStream *stream, char* fname)
{
    if (instance == 0)
    	return;

    PluginInstance* This = (PluginInstance*) instance->pdata;

    if(!This)
        return;

    This->Doc->initDoc(fname);

   	// invalidate window to ensure a redraw
    WinInvalidateRect(This->hWnd, 0, TRUE);
}


//----------------------------------------------------------------------------
// NPP_Print:
//----------------------------------------------------------------------------
void NP_LOADDS NPP_Print(NPP instance, NPPrint* printInfo)
{
    if(printInfo == 0)   // trap invalid parm
        return;

    if (instance != 0)
    {
        PluginInstance* This = (PluginInstance*) instance->pdata;

	    if(!This)
            return;

        if(!This->Doc->isReady())
            return;

        if (printInfo->mode == NP_FULL)
        {
            //
            // *Developers*: If your plugin would like to take over
            // printing completely when it is in full-screen mode,
            // set printInfo->pluginPrinted to TRUE and print your
            // plugin as you see fit.  If your plugin wants Netscape
            // to handle printing in this case, set printInfo->pluginPrinted
            // to FALSE (the default) and do nothing.  If you do want
            // to handle printing yourself, printOne is true if the
            // print button (as opposed to the print menu) was clicked.
            // On the Macintosh, platformPrint is a THPrint; on Windows,
            // platformPrint is a structure (defined in npapi.h) containing
            // the printer name, port, etc.
            //
            //void* platformPrint = printInfo->print.fullPrint.platformPrint;
            //NPBool printOne = printInfo->print.fullPrint.printOne;

            printInfo->print.fullPrint.pluginPrinted = FALSE; // Do the default

        }
        else    // If not fullscreen, we must be embedded
        {
            //
            // *Developers*: If your plugin is embedded, or is full-screen
            // but you returned false in pluginPrinted above, NPP_Print
            // will be called with mode == NP_EMBED.  The NPWindow
            // in the printInfo gives the location and dimensions of
            // the embedded plugin on the printed page.  On the Macintosh,
            // platformPrint is the printer port; on Windows, platformPrint
            // is the handle to the printing device context.
            //
            NPWindow* printWindow = &(printInfo->print.embedPrint.window);
            void* platformPrint = printInfo->print.embedPrint.platformPrint;

            /* get Presentation Space and save it */
            HPS hps = (HPS)platformPrint;
            LONG saveID = GpiSavePS(hps);

            /* create GPI various data structures about the drawing area */
            POINTL offWindow = { -(int)printWindow->x, -(int)printWindow->y };
            POINTL endPoint = { (int)printWindow->width, (int)printWindow->height };
            RECTL rect = { (int)printWindow->x,
                           (int)printWindow->y,
                           (int)printWindow->x + (int)printWindow->width,
                           (int)printWindow->y + (int)printWindow->height };

            /* get model transform so origin is 0,0 */
            MATRIXLF matModel;
            GpiQueryModelTransformMatrix(hps, 9L, &matModel);
            GpiTranslate(hps, &matModel, TRANSFORM_ADD, &offWindow);
            GpiSetModelTransformMatrix(hps, 9L, &matModel, TRANSFORM_REPLACE);

            /* set clipping region so we don't accidently draw outside our rectangle */
            HRGN hrgn = 0;
            HRGN hrgnOld = 0;

            GpiCreateRegion(hps, 1, &rect);
            GpiSetClipRegion(hps, hrgn, &hrgnOld);

            /* draw using common drawing routine */
            //Draw(This, hps, &endPoint, FALSE);
            This->Doc->drawPicture(hps, endPoint, FALSE);

            /* restore PS after drawing and delete created objects */
            GpiDestroyRegion(hps, hrgn);
            GpiDestroyRegion(hps, hrgnOld);
            GpiRestorePS(hps, saveID);
        }
    }
}

//----------------------------------------------------------------------------
// NPP_HandleEvent:
// Mac-only.
//----------------------------------------------------------------------------
int16 NP_LOADDS NPP_HandleEvent(NPP instance, void* event)
{
    return FALSE;
}

//----------------------------------------------------------------------------
// Helper routines
//----------------------------------------------------------------------------

//----------------------------------------
// Set scrollbar position
//----------------------------------------

int SetScrollBar(HWND hBar, int curr, int port, int max)
{
    int max1 = max - port;

    if(max1 < 0)
        max1 = 0;

    WinSendMsg(hBar, SBM_SETSCROLLBAR, MPFROMSHORT(curr), MPFROM2SHORT(0, max1));
    WinSendMsg(hBar, SBM_SETTHUMBSIZE, MPFROM2SHORT(port, max), 0);
}

//----------------------------------------
// Fill menu items from the item list
//----------------------------------------

void FillMenu(HWND hMenu, PUSERMENU pList, int start)
{
    if(!hMenu || !pList)
        return;

    for(int i = 0; pList[i].text; i++)
    {
        MENUITEM mi = {0};

        mi.iPosition = start + i;

        if(pList[i].text[0] == '#')
            mi.afStyle = MIS_BITMAP;

        else if(pList[i].text[0] == '-')
            mi.afStyle = MIS_SEPARATOR;

        else

            mi.afStyle = MIS_TEXT;

        if(mi.afStyle == MIS_BITMAP)
        {
            ULONG ulID = atoi(pList[i].text + 1);

            mi.hItem = GpiLoadBitmap(WinGetPS(hMenu), hMod, ulID, 0, 0);
        }

        mi.id = pList[i].id;

        WinSendMsg(hMenu, MM_INSERTITEM, (MPARAM) &mi,
                   (MPARAM)pList[i].text);
    }
}

//----------------------------------------
// Prepare plugin window at the place
// specified by browser
//----------------------------------------

void SetupWindow(PluginInstance* This)
{
    static int once = 0;

    if(!once)
        WinRegisterClass(0, (PSZ)"DjVu.Client", ClientProc, 0L, 4);

    ULONG ulFlags = FCF_NOBYTEALIGN | FCF_VERTSCROLL | FCF_HORZSCROLL;

    HWND hClient = 0;

    if(!hMod)
    {
        static char cPath[CCHMAXPATH] = {0};
        ULONG Obj;
        ULONG Off;
        ULONG eip = (ULONG)&SetupWindow;
        HMODULE hMod2 = 0;

        int rc = DosQueryModFromEIP(&hMod, &Obj, CCHMAXPATH, cPath, &Off, eip);

        rc = DosQueryModuleName(hMod, CCHMAXPATH, cPath);

        rc = DosLoadModule(0, 0, (PSZ)cPath, &hMod2);
    }

    HWND hWnd = WinCreateStdWindow(This->hWnd, WS_VISIBLE, &ulFlags,
                                   (PSZ)"DjVu.Client", (PSZ)"", 0L, hMod,
                                   FID_PANE, &hClient);

    //If to try to use FID_MENU style, frame does not load
    //properly, so add menu in a bit different way

    HWND hMenu = WinLoadMenu(hWnd, hMod, FID_PANE);

    WinSetWindowUShort(hMenu, QWS_ID, FID_MENU);
    FillMenu(hMenu, mCtl, 0);
    WinSendMsg(hWnd, WM_UPDATEFRAME, MPFROMLONG(FCF_MENU), 0);

    //Create two spin buttons

    ULONG ulCommon = SPBS_MASTER |
                     SPBS_NUMERICONLY |
                     SPBS_JUSTRIGHT |
                     SPBS_FASTSPIN |
                     WS_VISIBLE |
                     0;

    ULONG ulPage = ulCommon;
    ULONG ulZoom = ulCommon;

    This->hWndPage = WinCreateWindow(hWnd, WC_SPINBUTTON, 0, ulPage,
                                     0, 0, WIN_PAGE_SZ, TOOLBAR_SZ,
//                                     hClient, HWND_TOP, FID_CPAGE, 0, 0);
                                     hWnd, HWND_TOP, FID_CPAGE, 0, 0);

    This->hWndZoom = WinCreateWindow(hWnd, WC_SPINBUTTON, 0, ulZoom,
                                     WIN_PAGE_SZ, 0, WIN_ZOOM_SZ, TOOLBAR_SZ,
//                                     hClient, HWND_TOP, FID_CZOOM, 0, 0);
                                     hWnd, HWND_TOP, FID_CZOOM, 0, 0);

    WinSendMsg(This->hWndZoom, SPBM_SETLIMITS, (MPARAM)MAX_ZOOM, (MPARAM)MIN_ZOOM);
    WinSendMsg(This->hWndZoom, SPBM_SETCURRENTVALUE, (MPARAM)100, 0);

    //Subclass frame
    This->pOldProc = WinSubclassWindow(hWnd, FrameProc);

    //Set user info for client and frame proc
    WinSetWindowULong(hWnd, QWL_USER, (ULONG)This);
    WinSetWindowULong(hClient, QWL_USER, (ULONG)This);

    This->Doc->setHWND(hClient);
}

//----------------------------------------
// Mark specified menu item as "checked"
//----------------------------------------

void CheckMenuItem(HWND hwndMenu, USHORT ulID)
{
    WinSendMsg(hwndMenu,
               MM_SETITEMATTR,
               MPFROMSHORT(ulID),
               MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
}

//----------------------------------------
// Mark specified menu item as "disabled"
//----------------------------------------

void DisableMenuItem(HWND hwndMenu, USHORT ulID)
{
    WinSendMsg(hwndMenu,
               MM_SETITEMATTR,
               MPFROMSHORT(ulID),
               MPFROM2SHORT(MIA_DISABLED, MIA_DISABLED));
}


//----------------------------------------------------------------------------
// Window Procedures
//----------------------------------------------------------------------------

MRESULT APIENTRY FrameProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    PluginInstance *This = (PluginInstance*)WinQueryWindowULong(hWnd, QWL_USER);

    if(!This || !This->pOldProc)
        return WinDefWindowProc(hWnd, msg, mp1, mp2);

    switch(msg)
    {
        case WM_DESTROY:
            WinSetWindowULong(hWnd, QWL_USER, 0);
            WinSubclassWindow(hWnd, This->pOldProc);
            break;

        case WM_QUERYFRAMECTLCOUNT:
            {
                ULONG itemCount = (ULONG)(This->pOldProc(hWnd, msg, mp1, mp2)) + 2;
                return MRFROMLONG(itemCount);
            }

        case WM_FORMATFRAME:
            {
                USHORT usMenu    = 0;
                ULONG  itemCount = (ULONG)(This->pOldProc(hWnd, msg, mp1, mp2));
                PSWP   pSWP      = (PSWP)PVOIDFROMMP(mp1);
                HWND hMenu       = WinWindowFromID(hWnd, FID_MENU);

                while (pSWP[usMenu].hwnd != hMenu)
                    usMenu++;

                ULONG usPanel1 = itemCount;
                ULONG usPanel2 = itemCount + 1;

                pSWP[usPanel1] = pSWP[usMenu];
                pSWP[usPanel2] = pSWP[usMenu];

                pSWP[usPanel1].fl |= SWP_MOVE | SWP_SIZE;
                pSWP[usPanel2].fl |= SWP_MOVE | SWP_SIZE;

                pSWP[usPanel1].hwndInsertBehind = HWND_TOP;
                pSWP[usPanel2].hwndInsertBehind = HWND_TOP;

                pSWP[usPanel1].hwnd = This->hWndPage;
                pSWP[usPanel2].hwnd = This->hWndZoom;

                int nMenus = (int)WinSendMsg(hMenu, MM_QUERYITEMCOUNT, 0, 0);

                for(int i = 0; i < nMenus; i++)
                {
                    RECTL rcl = {0};
                    int usID = (int)WinSendMsg(hMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(i), 0);

                    WinSendMsg(hMenu, MM_QUERYITEMRECT, MPFROM2SHORT(usID, 0), (MPARAM)&rcl);

                    pSWP[usPanel1].x += rcl.xRight - rcl.xLeft;
                }

                pSWP[usPanel1].cx = WIN_PAGE_SZ;
                pSWP[usPanel1].y  += 3;

                pSWP[usPanel2].x  = pSWP[usPanel1].x + pSWP[usPanel1].cx;
                pSWP[usPanel2].cx = WIN_ZOOM_SZ;
                pSWP[usPanel2].y  += 3;

                itemCount += 2;
                return MRFROMSHORT(itemCount);
            }
    }

    return This->pOldProc(hWnd, msg, mp1, mp2);
}

MRESULT APIENTRY ClientProc(HWND hWnd, ULONG Message, MPARAM mp1, MPARAM mp2)
{
    PluginInstance *This = (PluginInstance*)WinQueryWindowULong(hWnd, QWL_USER);

    if(!This || !This->Doc)
        return WinDefWindowProc(hWnd, Message, mp1, mp2);

    switch(Message)
    {
        case WM_DESTROY:
            This->Doc->clear();
            WinSetWindowULong(hWnd, QWL_USER, 0);
            This->Doc->setHWND(0);
            break;

        case WM_CONTROL:
            if(This->Doc->isReady())
            {
                switch(SHORT1FROMMP(mp1))
                {
                    case FID_CZOOM:
                        if(SHORT2FROMMP(mp1) == SPBN_CHANGE)
                        {
                            ULONG ulValue = 0;
                            HWND hCtl = (HWND)mp2;

                            WinSendMsg(hCtl, SPBM_QUERYVALUE, MPFROMP(&ulValue),
                                       MPFROM2SHORT(0, SPBQ_DONOTUPDATE));

                            This->Doc->setZoom(ulValue);

                            if(SaveZoom)
                                Zoom = ulValue;
                        }
                        break;

                    case FID_CPAGE:
                        if(SHORT2FROMMP(mp1) == SPBN_CHANGE)
                        {
                            ULONG ulValue = 0;
                            HWND hCtl = (HWND)mp2;

                            WinSendMsg(hCtl, SPBM_QUERYVALUE, MPFROMP(&ulValue),
                                       MPFROM2SHORT(0, SPBQ_DONOTUPDATE));

                            This->Doc->setPage(ulValue - 1);
                        }
                        break;
                }
                break;
            }
            break;

        case WM_COMMAND:
            if(This->Doc->isReady())
            {
                switch(SHORT1FROMMP(mp1))
                {
                    case CMD_FIRST: This->Doc->setPage(0); break;
                    case CMD_LAST : This->Doc->setPage(This->Doc->getNumPages()) ; break;
                    case CMD_BCK10: This->Doc->setPage(This->Doc->getPage() - 10); break;
                    case CMD_BCK01: This->Doc->setPage(This->Doc->getPage() -  1); break;
                    case CMD_FWD01: This->Doc->setPage(This->Doc->getPage() +  1); break;
                    case CMD_FWD10: This->Doc->setPage(This->Doc->getPage() + 10); break;

                    case CMD_ABOUT:
                    	WinMessageBox(HWND_DESKTOP, hWnd,
                    	(PSZ)"NPDJVU Plugin for OS/2 \n"
                    		 VERSION"\n"
						    "Copyright (c) 2003,2004\n"
						    "Sergey I. Yevtushenko\n"
						    "\n"
							"DjVuLibre-3.5\n"
							"Copyright (c) 2002\n"
							"Leon Bottou and Yann Le Cun.\n"
							"Copyright (c) 2001\n"
							"AT&T\n"
							"DjVu (r) Reference Library (v. 3.5)\n"
							"Copyright (c) 1999-2001\n"
							"LizardTech, Inc. All Rights Reserved.\n",
                    	(PSZ)"About", 256, MB_OK | MB_MOVEABLE);
                        break;
                }
            }
            switch(SHORT1FROMMP(mp1))
            {
                case CMD_SAVE_ZOOM: SaveZoom = 1 - SaveZoom; break;
                case CMD_BW_MODE  :
                    BWmode = 1 - BWmode;

                    if(This->Doc->isReady())
                    	This->Doc->setPage(This->Doc->getPage());
                    break;
            }
            break;

        case WM_CONTEXTMENU:
            {
                HWND hFrame = WinQueryWindow(hWnd, QW_PARENT);
                HWND hMenu;
                POINTL ptl;

                WinQueryPointerPos(HWND_DESKTOP, &ptl);

                hMenu = WinLoadMenu(hFrame, hMod, FID_CTXMENU);

                if(SaveZoom)
                    CheckMenuItem(hMenu, CMD_SAVE_ZOOM);
                if(BWmode)
                    CheckMenuItem(hMenu, CMD_BW_MODE);

                if(!This->Doc->isReady())
                	DisableMenuItem(hMenu, CMD_SAVEAS);

                WinPopupMenu(HWND_DESKTOP, hFrame, hMenu, ptl.x, ptl.y, 0,
                             PU_NONE | PU_KEYBOARD | PU_MOUSEBUTTON1 |
                             PU_MOUSEBUTTON2 | PU_HCONSTRAIN | PU_VCONSTRAIN);
            }
            return 0;

        case WM_BUTTON1DOWN:
        case WM_BUTTON2DOWN:
        case WM_BUTTON3DOWN:
            if(WinQueryFocus(HWND_DESKTOP) != hWnd)
                WinSetFocus(HWND_DESKTOP, hWnd);
            break;

        case WM_CHAR:
            if(This->Doc->isReady())
            {
                if(SHORT1FROMMP(mp1) & KC_KEYUP)    //Filter out KEYUP events
                    break;

                if(SHORT1FROMMP(mp1) & KC_CHAR)
                {
                	ULONG ulValue = 0;

                    switch(SHORT1FROMMP(mp2))
                    {
                        case '+':
                            ulValue += KBD_ZOOM_DELTA;
                            break;

                        case '-':
                            ulValue -= KBD_ZOOM_DELTA;
                            break;
                    }

                    if(ulValue)
                    {
                        ULONG ulValue2 = ulValue;
                        HWND hCtl = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT), FID_CZOOM);

        	            WinSendMsg(hCtl, SPBM_QUERYVALUE, MPFROMP(&ulValue),
            	                   MPFROM2SHORT(0, SPBQ_DONOTUPDATE));

                        ulValue += ulValue2;

                        WinSendMsg(hCtl, SPBM_SETCURRENTVALUE, (MPARAM)ulValue, 0);
                    }
                }

                if(!(SHORT1FROMMP(mp1) & KC_VIRTUALKEY))
                    break;

                int isCtrl = SHORT1FROMMP(mp1) & KC_CTRL;

                switch(SHORT2FROMMP(mp2))
                {
                    case VK_LEFT : WinPostMsg(hWnd, WM_HSCROLL, 0, MPFROM2SHORT(0, SB_LINEUP));   break;
                    case VK_RIGHT: WinPostMsg(hWnd, WM_HSCROLL, 0, MPFROM2SHORT(0, SB_LINEDOWN)); break;

                    case VK_UP   : WinPostMsg(hWnd, WM_VSCROLL, 0, MPFROM2SHORT(0, SB_LINEUP));   break;
                    case VK_DOWN : WinPostMsg(hWnd, WM_VSCROLL, 0, MPFROM2SHORT(0, SB_LINEDOWN)); break;

                    case VK_HOME:
                        if(isCtrl)
                            WinPostMsg(hWnd, WM_COMMAND, MPFROM2SHORT(CMD_FIRST,0), 0);
                        else
                            WinPostMsg(hWnd, WM_HSCROLL, 0, MPFROM2SHORT(0, SB_PAGEUP));
                        break;

                    case VK_END:
                        if(isCtrl)
                            WinPostMsg(hWnd, WM_COMMAND, MPFROM2SHORT(CMD_LAST,0), 0);
                        else
                            WinPostMsg(hWnd, WM_HSCROLL, 0, MPFROM2SHORT(0, SB_PAGEDOWN));
                        break;

                    case VK_PAGEUP  :
                        if(isCtrl)
                            WinPostMsg(hWnd, WM_COMMAND, MPFROM2SHORT(CMD_BCK01,0), 0);
                        else
                            WinPostMsg(hWnd, WM_VSCROLL, 0, MPFROM2SHORT(0, SB_PAGEUP));
                        break;

                    case VK_PAGEDOWN:
                        if(isCtrl)
                            WinPostMsg(hWnd, WM_COMMAND, MPFROM2SHORT(CMD_FWD01,0), 0);
                        else
                            WinPostMsg(hWnd, WM_VSCROLL, 0, MPFROM2SHORT(0, SB_PAGEDOWN));
                        break;
                }
            }
            break;

        case WM_VSCROLL:
            if(This->Doc->isReady())
            {
                int page_y  = This->Doc->segmentrect.height()/2;
                int start_y = This->Doc->start_y;

                switch(SHORT2FROMMP(mp2))
                {
                    case SB_LINEUP   : start_y += SCROLL_DELTA_Y; break;
                    case SB_LINEDOWN : start_y -= SCROLL_DELTA_Y; break;
                    case SB_PAGEUP   : start_y += page_y; break;
                    case SB_PAGEDOWN : start_y -= page_y; break;
                    case SB_SLIDERPOSITION :
                        {
                            start_y = This->Doc->fullrect.height() -
                                      This->Doc->segmentrect.height() -
                                      SHORT1FROMMP(mp2);
                        }
                        break;
                }

                if((start_y + This->Doc->segmentrect.height()) > This->Doc->fullrect.height())
                    start_y = This->Doc->fullrect.height() - This->Doc->segmentrect.height();

                if(start_y < 0)
                    start_y = 0;

                if(This->Doc->start_y != start_y)
                {
                    This->Doc->start_y = start_y;
                    This->Doc->setScrollbars();
                    WinInvalidateRect(hWnd, 0, TRUE);
                }
            }
            break;

        case WM_HSCROLL:
            if(This->Doc->isReady())
            {
                int page_x  = This->Doc->segmentrect.width()/2;
                int start_x = This->Doc->start_x;

                switch(SHORT2FROMMP(mp2))
                {
                    case SB_LINELEFT : start_x -= SCROLL_DELTA_X; break;
                    case SB_LINERIGHT: start_x += SCROLL_DELTA_X; break;
                    case SB_PAGELEFT : start_x -= page_x; break;
                    case SB_PAGERIGHT: start_x += page_x; break;
                    case SB_SLIDERPOSITION : start_x = SHORT1FROMMP(mp2); break;
                }

                if((start_x + This->Doc->segmentrect.width()) > This->Doc->fullrect.width())
                    start_x = This->Doc->fullrect.width() - This->Doc->segmentrect.width();

                if(start_x < 0)
                    start_x = 0;

                if(This->Doc->start_x != start_x)
                {
                    This->Doc->start_x = start_x;
                    This->Doc->setScrollbars();
                    WinInvalidateRect(hWnd, 0, TRUE);
                }
            }
            break;

    	case WM_REALIZEPALETTE:
            WinInvalidateRect(hWnd, 0, TRUE);
            WinUpdateWindow(hWnd);
            return 0;

        case WM_PAINT:
            {
                /* get PS associated with window  and set to PU_TWIPS coordinates */
            	RECTL invalidRect;
                HPS hps = WinBeginPaint(hWnd, NULL, &invalidRect);
                SIZEL siz = { 0,0 };
                GpiSetPS(hps, &siz, PU_TWIPS);

                /* get window size and convert to world coordinates */

                RECTL rect;
                WinQueryWindowRect(hWnd, &rect);

                POINTL pts[2] = { { 0L, 0L },
                              { rect.xRight, rect.yTop }
                            };
                GpiConvert(hps, CVTC_DEVICE, CVTC_WORLD, 2L, pts);

            	/* draw using common drawing routine */

                if(!This->Doc->isReady())
                {
                    POINTL ptl = { 0, 0 };
                    char cText[64];

                    GpiMove(hps, &ptl);
                    GpiSetColor(hps, CLR_PALEGRAY);
                    GpiBox(hps, DRO_OUTLINEFILL, &pts[1], 0L, 0L);

                    RECTL rcl = {0};

                    rcl.xRight = pts[1].x;
                    rcl.yTop   = pts[1].y;

                    if(!This->Doc->bug)
                    {
                        if(This->Doc->pr_size)
                        {
                            char* p = cText;

                            p += sprintf(p, "%.0f%% of ",
                                         double(This->Doc->pr_pos)/double(This->Doc->pr_size)*100);

                            if(This->Doc->pr_size > (1024*1024))
                                p += sprintf(p, "%.1fM bytes", double(This->Doc->pr_size)/(1024.0*1024.0));
                            else
                                if(This->Doc->pr_size > 1024)
                                    p += sprintf(p, "%dK bytes", This->Doc->pr_size/1024);
                                else
                                    p += sprintf(p, "%d bytes", This->Doc->pr_size);

                            p += sprintf(p, " done");
                        }
                        else
                            strcpy(cText, "Loading document...");
                    }
                    else
                        strcpy(cText, "Error occured");

                    WinDrawText(hps, -1, (PSZ)cText,
                                &rcl, CLR_BLACK, CLR_PALEGRAY, DT_CENTER | DT_VCENTER);
                }
                else
                {
                    This->Doc->drawPicture(hps, pts[1], TRUE);
                }

                WinEndPaint(hps);
            }
            return (MRESULT)0;
    }

    return WinDefWindowProc(hWnd, Message, mp1, mp2);
}

