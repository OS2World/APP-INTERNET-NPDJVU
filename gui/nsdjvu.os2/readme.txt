
                              NPDJVU for OS/2

                                v0.9.3 beta

This Mozilla plugin enables it to view DjVu files.

Detailed information about DjVu you can find here http://www.djvuzone.org or
at http://www.lizardtech.com.

DjVuLibre project which is the base of this plugin, located at
http://sourceforge.net/projects/djvu/

The DjVuLibre and the NPDJVU for OS/2 are covered by GPL license (see
included copy of license). Sources of the library are available at the page
specified above. OS/2 port of the library (including sources) is available at
http://hobbes.nmsu.edu/pub/os2/apps/graphics/viewer/djvulibre-3.5.12.zip

Sources of the NPDJVU for OS/2, are included in this package and submitted
for inclusion in the main DjVuLibre source tree.

-------------------------------------------------------------------------------
Installation:

Just copy NPDJVU.DLL into 'plugins' subdirectory of the Mozilla.
Put LIBC05.DLL somewhere in the LIBPATH. If you already have one, do not
install one included in the package.

-------------------------------------------------------------------------------
Building from sources:

Unpack OS/2 port of the DjVuLibre sources. Unpack this package over
the DjVuLibre sources. Build library then change directory to gui\nsdjvu.os2
and run gmake. Environment requirements are the same as for OS/2 port
of DjVuLibre.

-------------------------------------------------------------------------------
Recognized keys:

    +/-                                     - Zoom control
    Up/Down/Left/Right/PgDn/PgUp/Home/End   - Scroll page
    Ctrl+PgUp/Ctrl+PgDn                     - Prev/Next page
    Ctrl+Home/Ctrl+End                      - First/Last page

-------------------------------------------------------------------------------
History:

September 2, 2004. V 0.9.3
	* Context menu
	* Some traps probably fixed

December 6, 2003. V 0.9.2
    * More changes in look and feel (two spinbuttons instead of
      ugly menus)
    + Added handling of the keyboard
    + Added load progress indicator
    * Some traps probably fixed

December 6, 2003. V 0.9.1
    * Workaround for some traps
    * Changes in look and feel

December 1, 2003.
    Initial release 0.9 beta.

-------------------------------------------------------------------------------
Known problems:

1. Image may be displayed incorrectly after resizing browser window.
2. May trap while opening local file if path contains NLS characters.
3. Zooming does not retain relative scrolling position.
4. Printing does not work.

-------------------------------------------------------------------------------
Copyrights:

NPDJVU for OS/2
Copyright (c) 2003, 2004 Sergey I. Yevtushenko <es@os2.ru>

DjVuLibre-3.5
Copyright (c) 2002  Leon Bottou and Yann Le Cun.
Copyright (c) 2001  AT&T
DjVu (r) Reference Library (v. 3.5)
Copyright (c) 1999-2001 LizardTech, Inc. All Rights Reserved.
The DjVu Reference Library is protected by U.S. Pat. No.
6,058,214 and patents pending.

-------------------------------------------------------------------------------

Contact information:

Sergey I. Yevtushenko <es@os2.ru> http://es.os2.ru/

