;/*
;** Module   :NPDJVU.DEF
;** Abstract :Exported entry points/module definition file
;**
;** Copyright (C) Sergey I. Yevtushenko
;**
;** This software is subject to, and may be distributed under, the
;** GNU General Public License, Version 2. The license should have
;** accompanied the software or you may obtain a copy of the license
;** from the Free Software Foundation at http://www.fsf.org .
;**
;** This program is distributed in the hope that it will be useful,
;** but WITHOUT ANY WARRANTY; without even the implied warranty of
;** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;** GNU General Public License for more details.
;**
;**----------------------------------------------------------------
;**
;** Log: Tue  02/12/2003 Created
;**
;*/

LIBRARY   NPDJVU INITINSTANCE TERMINSTANCE
PROTMODE
CODE      MOVEABLE DISCARDABLE LOADONCALL
DATA      MULTIPLE NONSHARED READWRITE LOADONCALL

DESCRIPTION @version@

EXPORTS
        NP_GetEntryPoints   @1
        NP_Initialize       @2
        NP_Shutdown         @3

