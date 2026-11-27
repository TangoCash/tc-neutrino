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


#ifndef __C_COVERFLOW__
#define __C_COVERFLOW__

#include <gui/components/cc.h>
#include <string>

/*logical values as used by CHintBox, scaled in init (frameBuffer->scale2Res) */
//!default width of the card box
#define COVERFLOW_DEFAULT_WIDTH	480
//!default maximum height for the cover image on top
#define COVERFLOW_PIC_MAX_HEIGHT	300
//!default maximum count of visible text lines below the image
#define COVERFLOW_MAX_LINES	5

//! Sub class of CComponentsInfoBox. Shows a "coverflow" card: an image on top and below it up to max_lines lines of text.
/*!
CCoverFlow provides a small box with one picture (the cover) painted on top,
horizontally centered and scaled into the box width without stretching,
and below of it a multi line text which is truncated at the given maximum
count of lines (default COVERFLOW_MAX_LINES = 5), an ellipsis marks cut texts.

	+---------------------------------------------------------------+
	| +-----------------------------------------------------------+ |
	| |                                                           | |
	 | |            [ cover image, max height 300 ]               | |  cf_pic_max_h
	 | |                                                           | |
	 | +-----------------------------------------------------------+ |
	 | | line 1                                                    | |  text area, up to
	 | | line 2                                                    | |  COVERFLOW_MAX_LINES
	 | | ...                                                       | |  lines (ellipsis if cut)
	 | | line N                                                    | |
	 | +-----------------------------------------------------------+ |
	+---------------------------------------------------------------+

*/

class CCoverFlow : public CComponentsInfoBox
{
	private:
		///property: maximum count of visible text lines, see setTextMaxLines()
		int cf_max_lines;

		///property: maximum height (logical px) the cover image is scaled to, width follows from aspect ratio
		int cf_pic_max_h;

		///property: indent from box edge to content and spacing between image and text
		int cf_margin;

		///property: path or icon name of the cover image on top
		std::string cf_image;

		///object: picture object for the cover image, see paint()
		CComponentsPicture *cf_pic;

		///status: display text truncated to cf_max_lines lines by layout(), used in paint()
		std::string cf_text;

		///status: calculated size of the scaled cover image, see layout()
		int cf_pic_w, cf_pic_h;

		///layout handler: truncates text, calculates image size and required box height, must be called before paintInit()
		void layout();

	public:
		/**CCoverFlow Constructor
		 * @param[in]	image_name
		 * 	@li 	optional: expects type std::string, path or icon name of the cover image on top, default = "" (no image)
		 * @param[in]	info_text
		 * 	@li 	optional: expects type std::string, UTF-8 encoded description text below the image, max. COVERFLOW_MAX_LINES lines visible
		 * @param[in]	w
		 * 	@li 	optional: expects type int, defines box width (logical px), default = COVERFLOW_DEFAULT_WIDTH
		 * @param[in]	font_text
		 * 	@li 	optional: expects type Font*, defines the text font type, default = NULL for system preset FONT_TYPE_MENU_INFO
		 * @param[in]	parent
		 * 	@li 	optional: expects CComponentsForm* parent in which this item is embedded, default = NULL (stand alone)
		 * @param[in]	shadow_mode
		 * 	@li 	optional: defines shadow mode, default = CC_SHADOW_ON
		 * @param[in]	color_text / color_frame / color_body / color_shadow
		 * 	@li 	optional: colors for text, frame, body and shadow, see CComponentsInfoBox()
		 */
		CCoverFlow(	const std::string& image_name = "",
					const std::string& info_text = "",
					const int w = COVERFLOW_DEFAULT_WIDTH,
					Font* font_text = NULL,
					CComponentsForm *parent = NULL,
					int shadow_mode = CC_SHADOW_ON,
					fb_pixel_t color_text = COL_MENUCONTENT_TEXT,
					fb_pixel_t color_frame = COL_FRAME_PLUS_0,
					fb_pixel_t color_body = COL_MENUCONTENT_PLUS_0,
					fb_pixel_t color_shadow = COL_SHADOW_PLUS_0);

		virtual ~CCoverFlow();

		/**
		 * Set the cover image on top.
		 * @param[in]	image_name
		 * 	@li 	expects type std::string, path or icon name of the image to display
		 */
		void setImage(const std::string& image_name)
		{
			cf_image = image_name;
			layout();
		}

		/**
		 * Set the description text below the image.
		 * @param[in]	text
		 * 	@li 	expects type std::string, UTF-8 encoded, more than cf_max_lines lines are truncated with an ellipsis
		 */
		void setInfoText(const std::string& text)
		{
			ct_text = text;
			layout();
		}

		/**
		 * Set maximum count of visible text lines.
		 * @param[in]	n_lines
		 * 	@li 	expects type int, value > 0, default = COVERFLOW_MAX_LINES (5)
		 */
		void setTextMaxLines(const int& n_lines)
		{
			if (n_lines > 0)
				cf_max_lines = n_lines;
			layout();
		}

		/**
		 * Set maximum height for the cover image, width follows from aspect ratio.
		 * @param[in]	h
		 * 	@li 	expects type int (logical px), value > 0, default = COVERFLOW_PIC_MAX_HEIGHT (300)
		 */
		void setPicMaxHeight(const int& h)
		{
			if (h > 0)
				cf_pic_max_h = h;
			layout();
		}

		/**
		 * Paint the coverflow card on screen, see CComponentsInfoBox::paint()
		 */
		virtual void paint(const bool &do_save_bg = CC_SAVE_SCREEN_YES);
};

#endif
