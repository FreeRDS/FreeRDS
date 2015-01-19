/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 *
 * Copyright 2013-2015 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include "rdp.h"
#include "rdpGCFuncs.h"

GCFuncs g_rdpGCFuncs;
extern GCOps g_rdpGCOps;
extern DevPrivateKeyRec g_rdpGCIndex;

#define GC_FUNC_PROLOGUE(_pGC) \
{ \
	priv = (rdpGCPtr)(dixGetPrivateAddr(&(_pGC->devPrivates), &g_rdpGCIndex)); \
	(_pGC)->funcs = (GCFUNCS_TYPE*) priv->funcs; \
	if (priv->ops != 0) \
	{ \
		(_pGC)->ops = (GCOPS_TYPE*) priv->ops; \
	} \
}

#define GC_FUNC_EPILOGUE(_pGC) \
{ \
	priv->funcs = (_pGC)->funcs; \
	(_pGC)->funcs = (GCFUNCS_TYPE*) &g_rdpGCFuncs; \
	if (priv->ops != 0) \
	{ \
		priv->ops = (_pGC)->ops; \
		(_pGC)->ops = (GCOPS_TYPE*) &g_rdpGCOps; \
	} \
}

static void rdpValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr d)
{
	rdpGCRec* priv;

	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->ValidateGC(pGC, changes, d);
	priv->ops = pGC->ops;
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpChangeGC(GCPtr pGC, unsigned long mask)
{
	rdpGCRec* priv;

	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->ChangeGC(pGC, mask);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpCopyGC(GCPtr src, unsigned long mask, GCPtr dst)
{
	rdpGCRec* priv;

	GC_FUNC_PROLOGUE(dst);
	dst->funcs->CopyGC(src, mask, dst);
	GC_FUNC_EPILOGUE(dst);
}

static void rdpDestroyGC(GCPtr pGC)
{
	rdpGCRec* priv;

	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->DestroyGC(pGC);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpChangeClip(GCPtr pGC, int type, pointer pValue, int nrects)
{
	rdpGCRec* priv;

	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->ChangeClip(pGC, type, pValue, nrects);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpDestroyClip(GCPtr pGC)
{
	rdpGCRec* priv;

	GC_FUNC_PROLOGUE(pGC);
	pGC->funcs->DestroyClip(pGC);
	GC_FUNC_EPILOGUE(pGC);
}

static void rdpCopyClip(GCPtr dst, GCPtr src)
{
	rdpGCRec* priv;

	GC_FUNC_PROLOGUE(dst);
	dst->funcs->CopyClip(dst, src);
	GC_FUNC_EPILOGUE(dst);
}

GCFuncs g_rdpGCFuncs =
{
	rdpValidateGC,
	rdpChangeGC,
	rdpCopyGC,
	rdpDestroyGC,
	rdpChangeClip,
	rdpDestroyClip,
	rdpCopyClip
};
