/* npos2.cpp */

//\\// INCLUDE
#include <os2.h>
#define DWORD ULONG
#define LPVOID PVOID

// netscape
#ifndef _NPAPI_H_
#include "npapi.h"
#endif
#ifndef _NPUPP_H_
#include "npupp.h"
#endif

//\\// DEFINE
#ifndef __GNUC__
#define NP_EXPORT _Export
#else
#define NP_EXPORT
#endif

//\\// GLOBAL DATA
NPNetscapeFuncs* g_pNavigatorFuncs = NULL;
/*
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\.
////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//.
// DllEntryPoint
//
//
BOOL DLLInitTerm( HMODULE hmod, int Flag)
{
   return 0;
}

*/
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\.
////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//.
//                                              PLUGIN DLL entry points
//
// These are the OS/2 specific DLL entry points. They must be exoprted
//

// we need these to be global since we have to fill one of its field
// with a data (class) which requires knowlwdge of the navigator
// jump-table. This jump table is known at Initialize time (NP_Initialize)
// which is called after NP_GetEntryPoint
static NPPluginFuncs* g_pluginFuncs;

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\.
////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//.
// NP_GetEntryPoints
//
//      fills in the func table used by Navigator to call entry points in
//  plugin DLL.  Note that these entry points ensure that DS is loaded
//  by using the NP_LOADDS macro, when compiling for Win16
//
extern "C" NPError OSCALL NP_EXPORT NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
    // trap a NULL ptr
    if(pFuncs == NULL)
        return NPERR_INVALID_FUNCTABLE_ERROR;

    // if the plugin's function table is smaller than the plugin expects,
    // then they are incompatible, and should return an error
    if(pFuncs->size < sizeof( NPPluginFuncs))
        return NPERR_INVALID_FUNCTABLE_ERROR;

    pFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    pFuncs->newp          = NPP_New;
    pFuncs->destroy       = NPP_Destroy;
    pFuncs->setwindow     = NPP_SetWindow;
    pFuncs->newstream     = NPP_NewStream;
    pFuncs->destroystream = NPP_DestroyStream;
    pFuncs->asfile        = NPP_StreamAsFile;
    pFuncs->writeready    = NPP_WriteReady;
    pFuncs->write         = NPP_Write;
    pFuncs->print         = NPP_Print;
    pFuncs->event         = NULL;       /// reserved

        g_pluginFuncs             = pFuncs;

    return NPERR_NO_ERROR;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\.
////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//.
// NP_Initialize
//
//      called immediately after the plugin DLL is loaded
//
extern "C" NPError OSCALL NP_EXPORT NP_Initialize(NPNetscapeFuncs* pFuncs)
{
    // trap a NULL ptr
    if(pFuncs == NULL)
        return NPERR_INVALID_FUNCTABLE_ERROR;

    g_pNavigatorFuncs = pFuncs; // save it for future reference

    // if the plugin's major ver level is lower than the Navigator's,
    // then they are incompatible, and should return an error
    if(HIBYTE(pFuncs->version) > NP_VERSION_MAJOR)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    // if the Navigator's function table is smaller than the plugin expects,
    // then they are incompatible, and should return an error
    if(pFuncs->size < sizeof (NPNetscapeFuncs))
        return NPERR_INVALID_FUNCTABLE_ERROR;

    return NPP_Initialize();
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\.
////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//.
// NP_Shutdown
//
//      called immediately before the plugin DLL is unloaded.
//      This function should check for some ref count on the dll to see if it is
//      unloadable or it needs to stay in memory.
//
extern "C" NPError OSCALL NP_EXPORT NP_Shutdown()
{
    NPP_Shutdown();

    g_pNavigatorFuncs = NULL;

    return NPERR_NO_ERROR;
}

//                                              END - PLUGIN DLL entry points
////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//.
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\.

/*    NAVIGATOR Entry points    */

/* These entry points expect to be called from within the plugin.  The
   noteworthy assumption is that DS has already been set to point to the
   plugin's DLL data segment.  Don't call these functions from outside
   the plugin without ensuring DS is set to the DLLs data segment first,
   typically using the NP_LOADDS macro
*/

/* returns the major/minor version numbers of the Plugin API for the plugin
   and the Navigator
*/
void NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
    *plugin_major   = NP_VERSION_MAJOR;
    *plugin_minor   = NP_VERSION_MINOR;
    *netscape_major = HIBYTE(g_pNavigatorFuncs->version);
    *netscape_minor = LOBYTE(g_pNavigatorFuncs->version);
}

/* causes the specified URL to be fetched and streamed in
*/
NPError NPN_GetURL(NPP instance, const char *url, const char *window)
{
    return g_pNavigatorFuncs->geturl(instance, url, window);
}

NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32 len, const char* buf, NPBool file)
{
    return g_pNavigatorFuncs->posturl(instance, url, window, len, buf, file);
}

/* Requests that a number of bytes be provided on a stream.  Typically
   this would be used if a stream was in "pull" mode.  An optional
   position can be provided for streams which are seekable.
*/
NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
    return g_pNavigatorFuncs->requestread(stream, rangeList);
}

/* Creates a new stream of data from the plug-in to be interpreted
   by Netscape in the current window.
*/
NPError NPN_NewStream(NPP instance, NPMIMEType type, NPStream* stream)
{
//  return g_pNavigatorFuncs->newstream(instance, type, target, stream);
    return g_pNavigatorFuncs->newstream(instance, type, stream);
}

/* Provides len bytes of data.
*/
int32 NPN_Write(NPP instance, NPStream *stream,
                int32 len, void *buffer)
{
    return g_pNavigatorFuncs->write(instance, stream, len, buffer);
}

/* Closes a stream object.
reason indicates why the stream was closed.
*/
NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
    return g_pNavigatorFuncs->destroystream(instance, stream, reason);
}

/* Provides a text status message in the Netscape client user interface
*/
void NPN_Status(NPP instance, const char *message)
{
    g_pNavigatorFuncs->status(instance, message);
}

/* returns the user agent string of Navigator, which contains version info
*/
const char* NPN_UserAgent(NPP instance)
{
    return g_pNavigatorFuncs->uagent(instance);
}

/* allocates memory from the Navigator's memory space.  Necessary so that
   saved instance data may be freed by Navigator when exiting.
*/
void* NPN_MemAlloc(uint32 size)
{
    return g_pNavigatorFuncs->memalloc(size);
}

/* reciprocal of MemAlloc() above
*/
void NPN_MemFree(void* ptr)
{
    g_pNavigatorFuncs->memfree(ptr);
}

/* private function to Netscape.  do not use!
*/
void NPN_ReloadPlugins(NPBool reloadPages)
{
    g_pNavigatorFuncs->reloadplugins(reloadPages);
}

