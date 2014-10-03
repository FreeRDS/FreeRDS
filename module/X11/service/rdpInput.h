/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2005-2013 Jay Sorg
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

#ifndef FREERDS_X11RDP_INPUT_H
#define FREERDS_X11RDP_INPUT_H

int rdpKeybdProc(DeviceIntPtr pDevice, int onoff);
int rdpMouseProc(DeviceIntPtr pDevice, int onoff);
Bool rdpCursorOffScreen(ScreenPtr* ppScreen, int* x, int* y);
void rdpCrossScreen(ScreenPtr pScreen, Bool entering);
void rdpPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScr, int x, int y);
void rdpPointerEnqueueEvent(DeviceIntPtr pDev, InternalEvent* event);
void rdpPointerNewEventScreen(DeviceIntPtr pDev, ScreenPtr pScr, Bool fromDIX);
Bool rdpSpriteRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScr, CursorPtr pCurs);
Bool rdpSpriteUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScr, CursorPtr pCurs);
void rdpSpriteSetCursor(DeviceIntPtr pDev, ScreenPtr pScr, CursorPtr pCurs, int x, int y);
void rdpSpriteMoveCursor(DeviceIntPtr pDev, ScreenPtr pScr, int x, int y);
Bool rdpSpriteDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScr);
void rdpSpriteDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScr);
void PtrAddMotionEvent(int x, int y);
void PtrAddButtonEvent(int buttonMask);
void KbdAddScancodeEvent(DWORD flags, DWORD scancode, DWORD keyboardType);
void KbdAddVirtualKeyCodeEvent(DWORD flags, DWORD vkcode);
void KbdAddUnicodeEvent(DWORD flags, DWORD code);
void KbdAddSyncEvent(DWORD flags);

#endif /* FREERDS_X11RDP_INPUT_H */
