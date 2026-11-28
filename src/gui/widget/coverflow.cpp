/*
	Based up Neutrino-GUI - Tuxbox-Project
	Copyright (C) 2001 by Steffen Hehn 'McClean'

	Classes for generic GUI-related components.
	Copyright (C) 2012-2017, Thilo Graf 'dbt'

	CoverFlow widget based up CComponentsInfoBox classes.

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino.h>
#include "coverflow.h"
#include <gui/widget/textbox.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>
#include <driver/pictureviewer/pictureviewer.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <list>
#include <map>
#include <vector>
#include <sys/stat.h>

extern CPictureViewer *g_PicViewer;

/*
 * The pictures of the cards. Decoding a JPEG of a few hundred pixels costs
 * around 100 ms on the box, and every move of a coverflow shows each picture
 * at another size (beside the card in the middle at 3/4, in the middle at
 * full size). A picture is therefore decoded once, somewhat larger than the
 * card that asks for it, and every card scales its own size down from that
 * in memory. Sizes and decoded pictures are kept by file, the key holding
 * inode, size and time, so that a file written anew under the same name
 * (a script reusing its names for the next list) is read anew.
 */
namespace
{
	struct cf_source_t
	{
		std::string key;	/* file key, see cf_fileKey() */
		fb_pixel_t *buf;	/* decoded picture, w x h */
		int w, h;
	};

	std::map<std::string, std::pair<int, int> > cf_sizes;	/* file key -> size of the picture */
	std::list<cf_source_t> cf_sources;			/* decoded pictures, the latest first */
	const size_t CF_SOURCES_MAX = 6;			/* about the three cards on screen and the next ones */
}

static std::string cf_fileKey(const std::string &path)
{
	struct stat st;
	if (path.empty() || stat(path.c_str(), &st) != 0)
		return "";
	char k[80];
	snprintf(k, sizeof(k), "|%lu|%lld|%ld", (unsigned long)st.st_ino, (long long)st.st_size, (long)st.st_mtime);
	return path + k;
}

/* the size of a picture, read from its file once */
static void cf_imageSize(const std::string &path, const std::string &key, int &w, int &h)
{
	std::map<std::string, std::pair<int, int> >::const_iterator it = cf_sizes.find(key);
	if (it != cf_sizes.end()) {
		w = it->second.first;
		h = it->second.second;
		return;
	}
	w = h = 0;
	if (g_PicViewer)
		g_PicViewer->getSize(path.c_str(), &w, &h);
	if (w > 0 && h > 0 && !key.empty())	/* a file that cannot be read yet is asked again next time */
		cf_sizes[key] = std::make_pair(w, h);
}

/* scales a picture down by averaging the pixels each new one covers */
static fb_pixel_t *cf_scale(const fb_pixel_t *src, int sw, int sh, int dw, int dh)
{
	if (!src || sw < 1 || sh < 1 || dw < 1 || dh < 1)
		return NULL;
	fb_pixel_t *dst = (fb_pixel_t *) malloc((size_t)dw * dh * sizeof(fb_pixel_t));
	if (!dst)
		return NULL;
	if (dw == sw && dh == sh) {
		memcpy(dst, src, (size_t)dw * dh * sizeof(fb_pixel_t));
		return dst;
	}
	for (int y = 0; y < dh; y++) {
		const int y0 = (int)((long long)y * sh / dh);
		const int y1 = std::min(sh, std::max(y0 + 1, (int)((long long)(y + 1) * sh / dh)));
		for (int x = 0; x < dw; x++) {
			const int x0 = (int)((long long)x * sw / dw);
			const int x1 = std::min(sw, std::max(x0 + 1, (int)((long long)(x + 1) * sw / dw)));
			uint32_t c0 = 0, c1 = 0, c2 = 0, c3 = 0, n = 0;
			for (int yy = y0; yy < y1; yy++) {
				const fb_pixel_t *p = src + (size_t)yy * sw + x0;
				for (int xx = x0; xx < x1; xx++, p++) {
					const uint32_t v = *p;
					c0 += v >> 24;
					c1 += (v >> 16) & 0xff;
					c2 += (v >> 8) & 0xff;
					c3 += v & 0xff;
					n++;
				}
			}
			dst[(size_t)y * dw + x] = ((c0 / n) << 24) | ((c1 / n) << 16) | ((c2 / n) << 8) | (c3 / n);
		}
	}
	return dst;
}

/* the decoded picture of a file, at least need_w x need_h, decoded when there is none */
static const cf_source_t *cf_getSource(const std::string &path, int need_w, int need_h)
{
	const std::string key = cf_fileKey(path);
	if (key.empty() || !g_PicViewer)
		return NULL;
	for (std::list<cf_source_t>::iterator it = cf_sources.begin(); it != cf_sources.end(); ++it) {
		if (it->key == key && it->w >= need_w && it->h >= need_h) {
			cf_sources.splice(cf_sources.begin(), cf_sources, it);	/* the latest first */
			return &cf_sources.front();
		}
	}

	/* somewhat more than 4/3 of what is asked: the card beside the middle one
	   is 3/4 of it, so the picture it decodes is large enough for the middle
	   one a move later (more than 4/3, as the margins of a card do not scale
	   and sizes are rounded); never larger than the picture is, unless the
	   card itself is */
	int ow = 0, oh = 0;
	cf_imageSize(path, key, ow, oh);
	if (ow < 1 || oh < 1)
		return NULL;
	float f = 1.4f;
	f = std::min(f, std::max(1.0f, std::min(static_cast<float>(ow) / need_w, static_cast<float>(oh) / need_h)));
	const int dw = std::max(need_w, static_cast<int>(need_w * f + 0.5f));
	const int dh = std::max(need_h, static_cast<int>(need_h * f + 0.5f));

	/* a cover is opaque where it has no alpha channel: black in it would
	   otherwise become transparent (TM_BLACK) and leave holes in a poster
	   wherever it is black; pictures with alpha keep theirs */
	CFrameBuffer *fb = CFrameBuffer::getInstance();
	fb->SetTransparent(CFrameBuffer::TM_NONE);
	fb_pixel_t *buf = g_PicViewer->getImage(path, dw, dh);
	fb->SetTransparentDefault();
	if (!buf)
		return NULL;

	for (std::list<cf_source_t>::iterator it = cf_sources.begin(); it != cf_sources.end(); ) {
		if (it->key == key) {	/* a smaller one of the same picture */
			free(it->buf);
			it = cf_sources.erase(it);
		} else
			++it;
	}
	cf_source_t s;
	s.key = key;
	s.buf = buf;
	s.w = dw;
	s.h = dh;
	cf_sources.push_front(s);
	while (cf_sources.size() > CF_SOURCES_MAX) {
		free(cf_sources.back().buf);
		cf_sources.pop_back();
	}
	return &cf_sources.front();
}

void CCoverFlow::clearImageCache()
{
	for (std::list<cf_source_t>::iterator it = cf_sources.begin(); it != cf_sources.end(); ++it)
		free(it->buf);
	cf_sources.clear();
	cf_sizes.clear();
}


/*!
 * Truncates a UTF-8 text to max_lines wrapped lines for the given width,
 * using the same line breaking rules and word widths as CTextBox
 * (see refreshTextLineArray() in gui/widget/textbox.cpp: words are split
 * at "\n/-. " including of the delimiter char).
 * If the text does not fit into max_lines, it is cut at a word border of
 * the last allowed line and an ellipsis (U+2026) is appended if there is
 * room for it. The result contains '\n' separated lines, like CTextBox.
 */
static std::string coverflow_truncateLines(	const std::string& text,
						Font* font,
						int max_width,
						int max_lines)
{
	if (text.empty() || max_lines < 1 || max_width < 1)
		return "";

	const std::string ellipsis("\xE2\x80\xA6"); /* U+2026 horizontal ellipsis */
	int w_ellipsis = font ? font->getRenderWidth(ellipsis, true) : 3;

	std::vector<std::vector<std::string> > lines;	/*wrapped tokens per line, in order of appearance*/
	bool done = false;
	size_t pos_prev = 0;

	while (!done && (pos_prev < text.size())) {
		/*one paragraph: up to the next explicit line break*/
		const size_t para_end = text.find('\n', pos_prev);
		std::string paragraph = (para_end == std::string::npos) ? text.substr(pos_prev) : text.substr(pos_prev, para_end - pos_prev);

		if (!paragraph.empty()) {
			std::vector<std::string> aktLine;	/*tokens of the line currently filled*/
			int aktWidth = 0;
			size_t wstart = 0;
			bool pdone = false;

			while (!pdone) {
				size_t pos = paragraph.find_first_of("/-. ", wstart);
				const bool at_end = (pos == std::string::npos);
				if (at_end)
					pos = paragraph.size();

				std::string word = paragraph.substr(wstart, pos - wstart + 1);	/*includes trailing delimiter char*/
				if (word.empty()) { pdone = true; break; }
				const int w_word = font ? font->getRenderWidth(word, true) : static_cast<int>(word.size());

				/*close the current line if the word does not fit anymore*/
				if (!aktLine.empty() && aktWidth + w_word > max_width) {
					lines.push_back(aktLine);
					aktLine.clear();
					aktWidth = 0;
				}

				aktLine.push_back(word);
				aktWidth += w_word;

				wstart = pos + 1;
				pdone = at_end;
			}

			if (!aktLine.empty())
				lines.push_back(aktLine);
		} else if (para_end != std::string::npos) {
			lines.push_back(std::vector<std::string>());	/*explicit blank line, like CTextBox*/
		}

		pos_prev = (para_end == std::string::npos) ? text.size() : para_end + 1;
		done = (pos_prev >= text.size());
	}

	const bool truncated = (lines.size() > static_cast<size_t>(max_lines));
	if (truncated)
		lines.resize(max_lines);

	std::string result;
	for (size_t i = 0; i < lines.size(); i++) {
		const bool is_last 	= ((i + 1) == lines.size());

		std::string line;
		for (size_t j = 0; j < lines[i].size(); j++)
			line += lines[i][j];

		if (is_last && truncated) {
			/*last visible line: drop trailing words until there is room for the ellipsis, then append it*/
			while (!line.empty() && isspace(static_cast<unsigned char>(line.back())))
				line.erase(line.size() - 1);

			int w_line = font ? font->getRenderWidth(line, true) : static_cast<int>(line.size());
			while (w_line + w_ellipsis > max_width && !lines[i].empty()) {
				lines[i].pop_back();
				line.clear();
				for (size_t j = 0; j < lines[i].size(); j++)
					line += lines[i][j];
				w_line = font ? font->getRenderWidth(line, true) : static_cast<int>(line.size());
			}

			if (w_line + w_ellipsis <= max_width) {
				while (!line.empty() && isspace(static_cast<unsigned char>(line.back())))
					line.erase(line.size() - 1);
				line += ellipsis;
			}
		}

		if (!result.empty())
			result += "\n";
		result += line;
	}

	return result;
}


CCoverFlow::CCoverFlow(	const std::string& image_name,
				const std::string& info_text,
				const int w,
				Font* font_text,
				CComponentsForm *parent,
				int shadow_mode,
				fb_pixel_t color_text,
				fb_pixel_t color_frame,
				fb_pixel_t color_body,
				fb_pixel_t color_shadow)
/* the singleton, not the member frameBuffer: the base class that sets the
   member is not built yet while its own arguments are worked out */
: CComponentsInfoBox(0, 0, CFrameBuffer::getInstance()->scale2Res(w), 10, info_text, CTextBox::AUTO_WIDTH, font_text, parent, shadow_mode, color_text, color_frame, color_body, color_shadow)
{
	cc_item_type.name = "wg.coverflow";

	/*cover image on top and text below it (own members, the base class keeps its pic/pic_name private)*/
	cf_image 	= image_name;
	cf_margin 	= OFFSET_INNER_MID;

	cf_max_lines 	= COVERFLOW_MAX_LINES;
	cf_pic_max_h 	= COVERFLOW_PIC_MAX_HEIGHT;
	cf_pic_buf 	= NULL;
	cf_pic_w 		= cf_pic_h = 0;
	cf_text_w 	= cf_text_lines = 0;
	cf_text_font 	= NULL;

	/*pre-calculate truncated text, image size and box height, so paint() can be called directly*/
	layout();
}


CCoverFlow::~CCoverFlow()
{
	if (cf_pic_buf) { free(cf_pic_buf); cf_pic_buf = NULL; }
}


void CCoverFlow::layout()
{
	Font* font = ct_font ? ct_font : g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO];

	int margin 	= fr_thickness + cf_margin;	/*indent from box edge to content*/
	int content_w = std::max(1, width - 2 * margin);

	/*truncate the text to at most cf_max_lines lines (with ellipsis if cut), again only when what it depends on changed*/
	if (ct_text != cf_text_src || content_w != cf_text_w || cf_max_lines != cf_text_lines || font != cf_text_font) {
		cf_text 	= coverflow_truncateLines(ct_text, font, content_w, cf_max_lines);
		cf_text_src 	= ct_text;
		cf_text_w 	= content_w;
		cf_text_lines 	= cf_max_lines;
		cf_text_font 	= font;
	}

	/*visible line count and line height for the box height calculation*/
	const int n_lines = CTextBox::getLines(cf_text);	/*0 for empty text*/
	int line_h 	= (font ? font->getHeight() : 16);

	/*cover image: fit into content_w x pic_max_h keeping aspect ratio, no stretching.
	   Same name resolution as CCPicture via CCDraw::setBodyBGImageName(). */
	const int old_w = cf_pic_w, old_h = cf_pic_h;
	const std::string old_path = cf_image_path;
	cf_pic_w = cf_pic_h = 0;
	cf_image_path.clear();
	if (!cf_image.empty()) {
		const std::string img = frameBuffer->getIconPath(cf_image);
		if (!img.empty() && g_PicViewer) {
			int ow = 0, oh = 0;
			cf_imageSize(img, cf_fileKey(img), ow, oh);
			if (ow > 0 && oh > 0) {
				cf_image_path = img;
				const int pic_max_h = frameBuffer->scale2Res(cf_pic_max_h);
				float rx = static_cast<float>(content_w) / static_cast<float>(ow);
				float ry = static_cast<float>(pic_max_h) / static_cast<float>(oh);
				float ratio = (rx < ry) ? rx : ry;
				cf_pic_w 	= std::max(1, static_cast<int>(static_cast<float>(ow) * ratio + 0.5f));
				cf_pic_h 	= std::max(1, static_cast<int>(static_cast<float>(oh) * ratio + 0.5f));
			}
		}
	}

	/*a scaled picture of another size or file is made anew on the next paint*/
	if (cf_pic_buf && (cf_pic_w != old_w || cf_pic_h != old_h || cf_image_path != old_path)) {
		free(cf_pic_buf);
		cf_pic_buf = NULL;
	}

	/*required box height: top+bottom margins, image block with spacing to text and the text lines*/
	height = 2 * margin
		+ (cf_pic_h > 0 ? cf_pic_h + cf_margin : 0)
		+ n_lines * line_h;
}


void CCoverFlow::preload()
{
	layout();
	if (!cf_image_path.empty() && cf_pic_w > 0)
		cf_getSource(cf_image_path, cf_pic_w, cf_pic_h);
}


void CCoverFlow::paint(const bool &do_save_bg)
{
	layout();			/*recalculate truncated text, image size and height before the fb init*/
	paintInit(do_save_bg);	/*shadow + body + frame, see CComponentsItem::paintInit()*/

	Font* font = ct_font ? ct_font : g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO];

	/*positions of the content area (real screen values required)*/
	int margin 	= fr_thickness + cf_margin;
	int cx 		= x + margin;	/*left border of the content*/
	int cy 		= y + margin;	/*top border of the content*/
	int cw 		= std::max(1, width - 2 * margin);

	/*---- cover image on top, horizontally centered ------------------------*/
	if (!cf_image_path.empty() && cf_pic_w > 0) {
		/*scaled from the decoded picture all cards share, and kept for the next paint*/
		if (!cf_pic_buf) {
			const cf_source_t *src = cf_getSource(cf_image_path, cf_pic_w, cf_pic_h);
			if (src)
				cf_pic_buf = cf_scale(src->buf, src->w, src->h, cf_pic_w, cf_pic_h);
		}
		/*blended over the body, so pictures with an alpha channel keep it*/
		if (cf_pic_buf && cc_allow_paint)
			frameBuffer->blit2FB(cf_pic_buf, cf_pic_w, cf_pic_h, cx + (cw - cf_pic_w) / 2, cy, 0, 0, false);

		cy += cf_pic_h + cf_margin;	/*text starts below the image with spacing*/
	}

	/*---- text below, max. cf_max_lines lines ------------------------------*/
	if (!cf_text.empty()) {
		if (cctext)
			delete cctext;
		cctext = NULL;
	}

	const int n_lines 	= CTextBox::getLines(cf_text);	/*0 for empty text*/
	const int th 		= std::max(1, n_lines) * font->getHeight();

	if (cctext == NULL && !cf_text.empty())
		cctext = new CComponentsText(cx, cy, cw, th);

	if (cctext) {
		cctext->setText(cf_text, ct_text_mode, font);
		cctext->doPaintTextBoxBg(ct_paint_textbg);
		cctext->doPaintBg(false);
		cctext->setTextColor(ct_col_text);
		cctext->enableTboxSaveScreen(cc_txt_save_screen);
		cctext->setDimensionsAll(cx, cy, cw, th);

		/*paint, but set visibility mode*/
		cctext->allowPaint(cc_allow_paint);
		cctext->paint(CC_SAVE_SCREEN_NO);
	}

	/*An infobox without own bg, shadow and frame paints no fb layer at all, so
	  paintFbItems() never marks it as painted; do it here like CComponentsInfoBox::paint(),
	  otherwise parent forms skip the hide()/repaint cycle for this item forever. */
	if (cc_allow_paint)
		is_painted = true;

	OnAfterPaintInfo();
}
