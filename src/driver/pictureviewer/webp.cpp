/*
 * WebP pictures for the picture viewer, decoded with libwebp
 * (still images; of an animation the first frame)
 *
 * The handler works like the PNG one: fh_webp_load gives RGB, for the
 * viewer and every other caller; webp_load_ext gives RGBA where the picture
 * carries an alpha channel, for getImage/getIcon, which blend it.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "pv_config.h"

#ifdef FBV_SUPPORT_WEBP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webp/decode.h>

#include "pictureviewer.h"

/* the container: "RIFF" <size> "WEBP" */
int fh_webp_id(const char *name)
{
	unsigned char id[12];
	FILE *fh = fopen(name, "rb");
	if (!fh)
		return 0;
	size_t n = fread(id, 1, sizeof(id), fh);
	fclose(fh);
	return n == sizeof(id) && !memcmp(id, "RIFF", 4) && !memcmp(id + 8, "WEBP", 4);
}

/* the whole file, which libwebp decodes from memory */
static uint8_t *webp_read(const char *name, size_t *size)
{
	FILE *fh = fopen(name, "rb");
	if (!fh)
		return NULL;
	uint8_t *data = NULL;
	long len = -1;
	if (fseek(fh, 0, SEEK_END) == 0)
		len = ftell(fh);
	if (len > 0 && len < 64 * 1024 * 1024 && fseek(fh, 0, SEEK_SET) == 0) {
		data = (uint8_t *) malloc(len);
		if (data && fread(data, 1, len, fh) != (size_t) len) {
			free(data);
			data = NULL;
		}
	}
	fclose(fh);
	*size = data ? (size_t) len : 0;
	return data;
}

/* decodes into *buffer, which the caller made for the size fh_webp_getsize
 * gave (at least x * y * 3, or x * y * 4 with alpha); bpp is 4 when the
 * picture has alpha and it was asked for, otherwise 3 */
static int int_webp_load(const char *name, unsigned char **buffer, int *xp, int *yp, int *bpp, bool alpha)
{
	size_t size = 0;
	uint8_t *data = webp_read(name, &size);
	if (!data)
		return FH_ERROR_FILE;

	WebPBitstreamFeatures f;
	if (WebPGetFeatures(data, size, &f) != VP8_STATUS_OK || f.width < 1 || f.height < 1) {
		free(data);
		return FH_ERROR_FORMAT;
	}

	const bool rgba = alpha && f.has_alpha;
	const int stride = f.width * (rgba ? 4 : 3);
	const size_t outsize = (size_t) stride * f.height;
	uint8_t *out = rgba ? WebPDecodeRGBAInto(data, size, *buffer, outsize, stride)
			    : WebPDecodeRGBInto(data, size, *buffer, outsize, stride);
	free(data);
	if (!out)
		return FH_ERROR_FORMAT;

	*xp = f.width;
	*yp = f.height;
	if (bpp)
		*bpp = rgba ? 4 : 3;
	return FH_ERROR_OK;
}

int webp_load_ext(const char *name, unsigned char **buffer, int *xp, int *yp, int *bpp)
{
	return int_webp_load(name, buffer, xp, yp, bpp, true);
}

int fh_webp_load(const char *name, unsigned char **buffer, int *xp, int *yp)
{
	return int_webp_load(name, buffer, xp, yp, NULL, false);
}

int fh_webp_getsize(const char *name, int *x, int *y, int /*wanted_width*/, int /*wanted_height*/)
{
	/* the size is in the first chunk header: 64 bytes are plenty for
	 * WebPGetInfo (VP8, VP8L and the extended VP8X header) */
	uint8_t head[64];
	FILE *fh = fopen(name, "rb");
	if (!fh)
		return FH_ERROR_FILE;
	size_t n = fread(head, 1, sizeof(head), fh);
	fclose(fh);
	int w = 0, h = 0;
	if (!WebPGetInfo(head, n, &w, &h) || w < 1 || h < 1)
		return FH_ERROR_FORMAT;
	*x = w;
	*y = h;
	return FH_ERROR_OK;
}

#endif /* FBV_SUPPORT_WEBP */
