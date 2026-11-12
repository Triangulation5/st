#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <X11/Xft/Xft.h>
#include <X11/cursorfont.h>
#include <hb.h>
#include <hb-ft.h>

#include "st.h"

#define FEATURE(c1,c2,c3,c4) { .tag = HB_TAG(c1,c2,c3,c4), .value = 1, .start = HB_FEATURE_GLOBAL_START, .end = HB_FEATURE_GLOBAL_END }

/*
 * Replace 0 with a list of font features, wrapped in FEATURE macro, e.g.
 * FEATURE('c', 'a', 'l', 't'), FEATURE('d', 'l', 'i', 'g')
 * 
 * Uncomment either one of the 2 lines below. Uncomment the prior to disable (any) font features. Uncomment the 
 * latter to enable the (selected) font features.
 */

hb_feature_t features[] = { 0 };
//hb_feature_t features[] = { FEATURE('s','s','0','1'), FEATURE('s','s','0','2'), FEATURE('s','s','0','3'), FEATURE('s','s','0','5'), FEATURE('s','s','0','6'), FEATURE('s','s','0','7'), FEATURE('s','s','0','8'), FEATURE('z','e','r','o') };

void hbtransformsegment(XftFont *xfont, const Glyph *string, hb_codepoint_t *codepoints, int start, int length);
hb_font_t *hbfindfont(XftFont *match);

typedef struct {
	XftFont *match;
	hb_font_t *font;
} HbFontMatch;

static int hbfontslen = 0;
static HbFontMatch *hbfontcache = NULL;

void
hbunloadfonts()
{
	for (int i = 0; i < hbfontslen; i++) {
		hb_font_destroy(hbfontcache[i].font);
		XftUnlockFace(hbfontcache[i].match);
	}

	if (hbfontcache != NULL) {
		free(hbfontcache);
		hbfontcache = NULL;
	}
	hbfontslen = 0;
}

hb_font_t *
hbfindfont(XftFont *match)
{
	for (int i = 0; i < hbfontslen; i++) {
		if (hbfontcache[i].match == match)
			return hbfontcache[i].font;
	}

	/* Font not found in cache, caching it now. */
	hbfontcache = realloc(hbfontcache, sizeof(HbFontMatch) * (hbfontslen + 1));
	FT_Face face = XftLockFace(match);
	hb_font_t *font = hb_ft_font_create(face, NULL);
	if (font == NULL)
		die("Failed to load Harfbuzz font.");

	hbfontcache[hbfontslen].match = match;
	hbfontcache[hbfontslen].font = font;
	hbfontslen += 1;

	return font;
}

void
hbtransform(XftGlyphFontSpec *specs, const Glyph *glyphs, size_t len, int x, int y)
{
	int start = 0, length = 1, gstart = 0;
	hb_codepoint_t *codepoints = calloc((unsigned int)len, sizeof(hb_codepoint_t));
	int specidx = 0;

	if (codepoints == NULL)
		die("Failed to allocate Harfbuzz codepoint buffer.");

	for (int i = 0; i < len; i++) {
		if (glyphs[i].mode & ATTR_WDUMMY)
			continue;
		codepoints[i] = specs[specidx++].glyph;
	}

	for (int idx = 1, specidx = 1; idx < len; idx++) {
		if (glyphs[idx].mode & ATTR_WDUMMY) {
			length += 1;
			continue;
		}

		if (specs[specidx].font != specs[start].font || ATTRCMP(glyphs[gstart], glyphs[idx]) || selected(x + idx, y) != selected(x + gstart, y)) {
			hbtransformsegment(specs[start].font, glyphs, codepoints, gstart, length);

			/* Reset the sequence. */
			length = 1;
			start = specidx;
			gstart = idx;
		} else {
			length += 1;
		}

		specidx++;
	}

	/* EOL. */
	hbtransformsegment(specs[start].font, glyphs, codepoints, gstart, length);

	/* Apply the transformation to glyph specs. */
	for (int i = 0, specidx = 0; i < len; i++) {
		if (glyphs[i].mode & ATTR_WDUMMY)
			continue;
		if (glyphs[i].mode & ATTR_BOXDRAW) {
			specidx++;
			continue;
		}

		if (codepoints[i] != specs[specidx].glyph)
			((Glyph *)glyphs)[i].mode |= ATTR_LIGA;

		specs[specidx++].glyph = codepoints[i];
	}

	free(codepoints);
}

void
hbtransformsegment(XftFont *xfont, const Glyph *string, hb_codepoint_t *codepoints, int start, int length)
{
	hb_font_t *font = hbfindfont(xfont);
	if (font == NULL)
		return;

	hb_codepoint_t *runes;
	hb_codepoint_t space;
	unsigned int infolen, i;
	unsigned int lastcluster = 0;
	int valid = 1;
	hb_buffer_t *buffer = hb_buffer_create();
	hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);

	runes = calloc((unsigned int)length, sizeof(hb_codepoint_t));
	if (runes == NULL)
		die("Failed to allocate Harfbuzz input buffer.");
	for (i = 0; i < (unsigned int)length; i++) {
		runes[i] = string[start + i].mode & ATTR_WDUMMY
			? 0x0020 : string[start + i].u;
	}
	hb_buffer_add_codepoints(buffer, runes, length, 0, length);
	free(runes);
	hb_buffer_guess_segment_properties(buffer);

	/* Shape the segment. */
	hb_shape(font, buffer, features, sizeof(features) / sizeof(features[0]));

	/* Map shaped glyphs back to terminal cells without reading past the
	 * shaped output when a ligature consumes multiple input characters. */
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer, &infolen);
	for (i = 0; i < infolen; i++) {
		if (info[i].cluster >= (unsigned int)length ||
		    (i > 0 && info[i].cluster <= lastcluster)) {
			valid = 0;
			break;
		}
		lastcluster = info[i].cluster;
	}
	if (infolen > (unsigned int)length)
		valid = 0;

	if (valid) {
		if (!hb_font_get_nominal_glyph(font, 0x0020, &space))
			space = 0;
		for (i = 0; i < (unsigned int)length; i++)
			codepoints[start + i] = space;
		for (i = 0; i < infolen; i++)
			codepoints[start + info[i].cluster] = info[i].codepoint;
	}

	/* Cleanup. */
	hb_buffer_destroy(buffer);
}
