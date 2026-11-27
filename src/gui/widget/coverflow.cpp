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
#include <vector>

extern CPictureViewer *g_PicViewer;


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
	cf_pic 		= NULL;
	cf_pic_w 		= cf_pic_h = 0;

	/*pre-calculate truncated text, image size and box height, so paint() can be called directly*/
	layout();
}


CCoverFlow::~CCoverFlow()
{
	if (cf_pic) { delete cf_pic; cf_pic = NULL; }
}


void CCoverFlow::layout()
{
	Font* font = ct_font ? ct_font : g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO];

	int margin 	= fr_thickness + cf_margin;	/*indent from box edge to content*/
	int content_w = std::max(1, width - 2 * margin);

	/*truncate the text to at most cf_max_lines lines (with ellipsis if cut)*/
	cf_text = coverflow_truncateLines(ct_text, font, content_w, cf_max_lines);

	/*visible line count and line height for the box height calculation*/
	const int n_lines = CTextBox::getLines(cf_text);	/*0 for empty text*/
	int line_h 	= (font ? font->getHeight() : 16);

	/*cover image: fit into content_w x pic_max_h keeping aspect ratio, no stretching.
	   Same name resolution as CCPicture via CCDraw::setBodyBGImageName(). */
	cf_pic_w = cf_pic_h = 0;
	if (!cf_image.empty()) {
		const std::string img = frameBuffer->getIconPath(cf_image);
		if (!img.empty() && g_PicViewer) {
			int ow = 0, oh = 0;
			g_PicViewer->getSize(img.c_str(), &ow, &oh);
			if (ow > 0 && oh > 0) {
				const int pic_max_h = frameBuffer->scale2Res(cf_pic_max_h);
				float rx = static_cast<float>(content_w) / static_cast<float>(ow);
				float ry = static_cast<float>(pic_max_h) / static_cast<float>(oh);
				float ratio = (rx < ry) ? rx : ry;
				cf_pic_w 	= std::max(1, static_cast<int>(static_cast<float>(ow) * ratio + 0.5f));
				cf_pic_h 	= std::max(1, static_cast<int>(static_cast<float>(oh) * ratio + 0.5f));
			}
		}
	}

	/*required box height: top+bottom margins, image block with spacing to text and the text lines*/
	height = 2 * margin
		+ (cf_pic_h > 0 ? cf_pic_h + cf_margin : 0)
		+ n_lines * line_h;
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
	if (cf_pic) { delete cf_pic; cf_pic = NULL; }

	if (!cf_image.empty() && cf_pic_w > 0) {
		cf_pic = new CComponentsPicture(cx + (cw - cf_pic_w) / 2, cy, cf_pic_w, cf_pic_h, cf_image);
		cf_pic->setColorBody(col_body_std);
		cf_pic->allowPaint(cc_allow_paint);
		cf_pic->paint(CC_SAVE_SCREEN_NO);

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
