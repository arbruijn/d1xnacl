/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Graphical routines for manipulating grs_bitmaps.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"


#include "gr.h"
#include "grdef.h"
#include "u_dpmi.h"
#include "bitmap.h"
#include "error.h"

#ifdef OGL
#include "ogl_init.h"
#endif

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq );

void gr_set_bitmap_data (grs_bitmap *bm, unsigned char *data)
{
#ifdef OGL
//	if (bm->bm_data!=data)
		ogl_freebmtexture(bm);
#endif
	bm->bm_data = data;
#ifdef D1XD3D
	Assert (bm->iMagic == BM_MAGIC_NUMBER);
	Win32_SetTextureBits (bm, data, bm->bm_flags & BM_FLAG_RLE);
#endif
}

void gr_init_bitmap( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline, unsigned char * data ) // TODO: virtualize
{
#ifdef D1XD3D
	Assert (bm->iMagic != BM_MAGIC_NUMBER || bm->pvSurface == NULL);
#endif

	bm->bm_x = x;
	bm->bm_y = y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = 0;
	bm->bm_type = mode;
	bm->bm_rowsize = bytesperline;

	bm->bm_data = NULL;
#ifdef D1XD3D
	bm->iMagic = BM_MAGIC_NUMBER;
	bm->pvSurface = NULL;
#endif

#ifdef D1XD3D
	Win32_CreateTexture (bm);
#endif
#ifdef OGL
	bm->bm_parent=NULL;bm->gltexture=NULL;
#endif

//	if (data != 0)
		gr_set_bitmap_data (bm, data);
/*
	else
		gr_set_bitmap_data (bm, malloc (w * h));
*/

#ifdef BITMAP_SELECTOR
	bm->bm_selector = 0;
#endif
}

void gr_init_bitmap_alloc( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline)
{
	gr_init_bitmap(bm, mode, x, y, w, h, bytesperline, 0);
	gr_set_bitmap_data(bm, d_malloc(MAX_BMP_SIZE(w, h)));
}

void gr_init_bitmap_data (grs_bitmap *bm) // TODO: virtulize
{
	bm->bm_data = NULL;
#ifdef D1XD3D
	Assert (bm->iMagic != BM_MAGIC_NUMBER);
	bm->iMagic = BM_MAGIC_NUMBER;
	bm->pvSurface = NULL;
#endif
#ifdef OGL
//	ogl_freebmtexture(bm);//not what we want here.
	bm->bm_parent=NULL;bm->gltexture=NULL;
#endif
}

void gr_free_bitmap(grs_bitmap *bm )
{
	gr_free_bitmap_data (bm);
	if (bm!=NULL)
		d_free(bm);
}

void gr_free_bitmap_data (grs_bitmap *bm) // TODO: virtulize
{
#ifdef D1XD3D
	Assert (bm->iMagic == BM_MAGIC_NUMBER);

	Win32_FreeTexture (bm);
	bm->iMagic = 0;
	if (bm->bm_data == BM_D3D_RENDER)
		bm->bm_data = NULL;
#endif
#ifdef OGL
	ogl_freebmtexture(bm);
#endif
	if (bm->bm_data != NULL)
		d_free (bm->bm_data);
	bm->bm_data = NULL;
}

void gr_init_sub_bitmap (grs_bitmap *bm, grs_bitmap *bmParent, int x, int y, int w, int h )	// TODO: virtualize
{
	bm->bm_x = x + bmParent->bm_x;
	bm->bm_y = y + bmParent->bm_y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = bmParent->bm_flags;
	bm->bm_type = bmParent->bm_type;
	bm->bm_rowsize = bmParent->bm_rowsize;

#ifdef OGL
	bm->gltexture=bmParent->gltexture;
	bm->bm_parent=bmParent;
#endif
#ifdef D1XD3D
	Assert (bmParent->iMagic == BM_MAGIC_NUMBER);
	bm->iMagic = BM_MAGIC_NUMBER;
	bm->pvSurface = bmParent->pvSurface;
	if (bm->bm_type == BM_DIRECTX)
	{
		bm->bm_data = bmParent->bm_data;
	}
	else
#endif
	{
		bm->bm_data = bmParent->bm_data+(unsigned int)((y*bmParent->bm_rowsize)+x);
	}

}

void gr_free_sub_bitmap(grs_bitmap *bm )
{
	if (bm!=NULL)
	{
#ifdef D1XD3D
		bm->iMagic = 0;
#endif
		d_free(bm);
	}
}


grs_bitmap *gr_create_bitmap(int w, int h )
{
	return gr_create_bitmap_raw (w, h, d_malloc(MAX_BMP_SIZE(w, h)));
}

grs_bitmap *gr_create_bitmap_raw(int w, int h, unsigned char * raw_data )
{
    grs_bitmap *new;

    new = (grs_bitmap *)d_malloc( sizeof(grs_bitmap) );
	gr_init_bitmap (new, 0, 0, 0, w, h, w, raw_data);

    return new;
}


grs_bitmap *gr_create_sub_bitmap(grs_bitmap *bm, int x, int y, int w, int h )
{
    grs_bitmap *new;

    new = (grs_bitmap *)d_malloc( sizeof(grs_bitmap) );
	gr_init_sub_bitmap (new, bm, x, y, w, h);

	return new;
}

void gr_set_bitmap_flags (grs_bitmap *pbm, int flags)
{
#ifdef D1XD3D
	Assert (pbm->iMagic == BM_MAGIC_NUMBER);

	if (pbm->pvSurface)
	{
		if ((flags & BM_FLAG_TRANSPARENT) != (pbm->bm_flags & BM_FLAG_TRANSPARENT))
		{
			Win32_SetTransparent (pbm->pvSurface, flags & BM_FLAG_TRANSPARENT);
		}
	}
#endif
	pbm->bm_flags = flags;
}

void gr_set_transparent (grs_bitmap *pbm, int bTransparent)
{
	if (bTransparent)
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags | BM_FLAG_TRANSPARENT);
	}
	else
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags & ~BM_FLAG_TRANSPARENT);
	}
}

void gr_set_super_transparent (grs_bitmap *pbm, int bTransparent)
{
	if (bTransparent)
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags & ~BM_FLAG_SUPER_TRANSPARENT);
	}
	else
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags | BM_FLAG_SUPER_TRANSPARENT);
	}
}

void gr_remap_bitmap( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];

	// This should be build_colormap_asm, but we're not using invert table, so...
	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = 255;

	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		gr_set_transparent (bmp, 1);

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		gr_set_super_transparent (bmp, 0);
}

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq )
{
	int i, r, g, b;

	for (i=0; i<256; i++ )	{
		r = *palette++;		
		g = *palette++;		
		b = *palette++;		
 		*colormap++ = gr_find_closest_color( r, g, b );
		*freq++ = 0;
	}
}


void gr_remap_bitmap_good( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];
   
	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = 255;

	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		gr_set_transparent (bmp, 1);

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		gr_set_super_transparent (bmp, 1);
}

#ifdef BITMAP_SELECTOR
int gr_bitmap_assign_selector( grs_bitmap * bmp )
{
	if (!dpmi_allocate_selector( bmp->bm_data, bmp->bm_w*bmp->bm_h, &bmp->bm_selector )) {
		bmp->bm_selector = 0;
		return 1;
	}
	return 0;
}
#endif

void gr_bitmap_check_transparency( grs_bitmap * bmp )
{
	int x, y;
	ubyte * data;

	data = bmp->bm_data;
	
	for (y=0; y<bmp->bm_h; y++ )	{
		for (x=0; x<bmp->bm_w; x++ )	{
			if (*data++ == 255 )	{
				gr_set_transparent (bmp, 1);
				return;
			}
		}
		data += bmp->bm_rowsize - bmp->bm_w;
	}

	bmp->bm_flags = 0;

}
