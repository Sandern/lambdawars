/*
 *
 * Compiz mouse position polling plugin
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _COMPIZ_MOUSEPOLL_H
#define _COMPIZ_MOUSEPOLL_H

#define MOUSEPOLL_ABIVERSION 20080116

typedef int PositionPollingHandle;

typedef void (*PositionUpdateProc) (CompScreen *s,
				    int        x,
				    int        y);

typedef PositionPollingHandle
(*AddPositionPollingProc) (CompScreen         *s,
			   PositionUpdateProc update);

typedef void
(*RemovePositionPollingProc) (CompScreen            *s,
			      PositionPollingHandle id);

typedef void
(*GetCurrentPositionProc) (CompScreen *s,
			   int        *x,
			   int        *y);

typedef struct _MousePollFunc {
   AddPositionPollingProc    addPositionPolling;
   RemovePositionPollingProc removePositionPolling;
   GetCurrentPositionProc    getCurrentPosition;
} MousePollFunc;

#endif
