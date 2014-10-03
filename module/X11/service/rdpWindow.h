/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013-2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef FREERDS_X11RDP_WINDOW_H
#define FREERDS_X11RDP_WINDOW_H

/* Window Procedures */
Bool rdpCreateWindow(WindowPtr pWindow);
Bool rdpDestroyWindow(WindowPtr pWindow);
Bool rdpPositionWindow(WindowPtr pWindow, int x, int y);
Bool rdpChangeWindowAttributes(WindowPtr pWindow, unsigned long mask);
Bool rdpRealizeWindow(WindowPtr pWindow);
Bool rdpUnrealizeWindow(WindowPtr pWindow);
int rdpValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind);
void rdpPostValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind);
void rdpWindowExposures(WindowPtr pWindow, RegionPtr prgn, RegionPtr other_exposed);
void rdpCopyWindow(WindowPtr pWindow, DDXPointRec ptOldOrg, RegionPtr prgnSrc);
void rdpClearToBackground(WindowPtr pWin, int x, int y, int w, int h, Bool generateExposures);
void rdpClipNotify(WindowPtr pWindow, int dx, int dy);
void rdpRestackWindow(WindowPtr pWindow, WindowPtr pOldNextSib);

#endif /* FREERDS_X11RDP_WINDOW_H */
