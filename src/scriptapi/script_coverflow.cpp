/*
 * script coverflow
 *
 * See script_coverflow.h for what this is for.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "script_coverflow.h"

#include <algorithm>

#include <global.h>
#include <neutrino.h>
#include <driver/framebuffer.h>
#include <driver/neutrinofonts.h>
#include <driver/fontrenderer.h>
#include <system/debug.h>

ScriptCoverFlow::ScriptCoverFlow(const params_t &pars)
{
	cards = pars.cards;
	width     = pars.width > 0 ? pars.width : COVERFLOW_DEFAULT_WIDTH;
	max_lines = pars.max_lines > 0 ? pars.max_lines : COVERFLOW_MAX_LINES;
	pic_max_h = pars.pic_max_h > 0 ? pars.pic_max_h : COVERFLOW_PIC_MAX_HEIGHT;
	active_idx = 0;
	last_dir = 1;
}

ScriptCoverFlow::~ScriptCoverFlow()
{
	hide();
	CCoverFlow::clearImageCache();   /* the decoded pictures are needed no longer */
}

void ScriptCoverFlow::paint(void)
{
	render(active_idx);
}

void ScriptCoverFlow::hide(void)
{
	/* in the reverse order of painting: a card painted later saved a
	 * background that may hold parts of the ones before it (overlap, shadow),
	 * so it has to put that back first */
	for (size_t i = shown.size(); i-- > 0; ) {
		shown[i]->hide();   /* restore the screen behind it, and let go of its saved buffer */
		delete shown[i];
	}
	shown.clear();
}

int ScriptCoverFlow::getActive()
{
	return cards.empty() ? 0 : active_idx + 1;
}

/* One position of the menu: the card in the middle and its neighbours. What
 * was on screen before comes away first, so that only the new position is
 * left when it is drawn over what the old one saved behind it. */
void ScriptCoverFlow::render(int active)
{
	hide();

	if (cards.empty())
		return;

	if (active < 0 || active >= (int)cards.size())
		active = 0;
	if (active != active_idx)
		last_dir = (active > active_idx) ? 1 : -1;
	active_idx = active;

	/* The visible area of the screen, in real pixels. The card scales its
	 * logical width to the OSD resolution (scale2Res), so the positions are
	 * worked out from the widths it really has, not from the logical ones. */
	CFrameBuffer *fb = CFrameBuffer::getInstance();
	const int screen_x = fb->getScreenX();
	const int screen_y = fb->getScreenY();
	const int screen_w = fb->getScreenWidth();
	const int screen_h = fb->getScreenHeight();
	const int gap      = fb->scale2Res(8);

	/* a card wider than the screen would be drawn beyond it: the logical
	 * width is cut back until the card in the middle fits */
	int w_mid = width;
	if (fb->scale2Res(w_mid) > screen_w && screen_w > 0)
		w_mid = std::max(1, w_mid * screen_w / fb->scale2Res(w_mid));
	/* the neighbours sit smaller beside it: three quarters of the card in
	 * the middle in width, image height and font */
	const int w_side = w_mid * 3 / 4;
	const int pic_side = std::max(1, pic_max_h * 3 / 4);
	Font *font_side = NULL;
	Font *font_mid = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO];
	if (font_mid) {
		int fdx = 0, fdy = std::max(1, font_mid->getHeight() * 3 / 4);
		Font **f = CNeutrinoFonts::getInstance()->getDynFont(fdx, fdy);   /* shared, kept by CNeutrinoFonts */
		if (f)
			font_side = *f;
	}

	const int real_mid = fb->scale2Res(w_mid);
	const int center_x = screen_x + (screen_w - real_mid) / 2;

	/* the neighbours first, and the card in the middle last */
	const int offs[3] = { -1, +1, 0 };

	for (int i = 0; i < 3; i++) {
		int off = offs[i];
		int idx = active + off;
		if (idx < 0 || idx >= (int)cards.size())
			continue;

		CCoverFlow *cf = new CCoverFlow(cards[idx].image, cards[idx].text, off ? w_side : w_mid, off ? font_side : NULL);
		cf->setTextMaxLines(max_lines);
		cf->setPicMaxHeight(off ? pic_side : pic_max_h);

		int x = center_x;
		if (off != 0) {
			const int real_side = cf->getWidth();
			x = (off < 0) ? center_x - gap - real_side : center_x + real_mid + gap;
			/* a neighbour may reach into the border around the visible
			 * area, but one that would stand beyond the screen itself is
			 * left out */
			if (x < 0 || x + real_side > (int)fb->getScreenWidth(true)) {
				delete cf;
				continue;
			}
		}

		/* the cards are centred on each other, and none above the screen */
		int y = std::max(screen_y, screen_y + (screen_h - cf->getHeight()) / 2);
		cf->setXPos(x);
		cf->setYPos(y);
		cf->paint();
		shown.push_back(cf);
	}

	/* the picture that comes in on the next move the same way is decoded
	 * now, after this position is on screen, so that move only scales it */
	const int next = active + 2 * last_dir;
	if (next >= 0 && next < (int)cards.size() && !cards[next].image.empty()) {
		CCoverFlow pre(cards[next].image, cards[next].text, w_side, font_side);
		pre.setTextMaxLines(max_lines);
		pre.setPicMaxHeight(pic_side);
		pre.preload();
	}
}

/* The loop is the one gui/widget/hintbox.cpp runs when a menu shows a box of
 * its own: left and right move between the cards, ok picks the one in the
 * middle, back leaves it without picking anything. */
int ScriptCoverFlow::exec(int timeout)
{
	if (cards.empty())
		return 0;

	render(active_idx);

	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	/* zero is no timeout at all: the menu waits until a key decides it */
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout < 0 ? 0 : timeout);

	int selected = 0;   /* nothing picked yet */

	while (true)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		if (msg == CRCInput::RC_timeout) {
			break;   /* left alone too long: leave it without a pick */
		} else if (msg == CRCInput::RC_ok) {
			selected = active_idx + 1;
			break;
		} else if (CNeutrinoApp::getInstance()->backKey(msg)) {
			break;   /* back leaves the menu, whatever it was */
		} else if (msg == CRCInput::RC_mode) {
			break;
		} else if (msg == CRCInput::RC_left || msg == CRCInput::RC_right) {
			int next = active_idx + (msg == CRCInput::RC_left ? -1 : 1);
			if (next >= 0 && next < (int)cards.size())
				render(next);
			/* the timeout counts idle time: a key starts it again */
			if (timeout > 0)
				timeoutEnd = CRCInput::calcTimeoutEnd(timeout);
		} else if ((msg == CRCInput::RC_next) || (msg == CRCInput::RC_prev)) {
			g_RCInput->postMsg(msg, data);   /* a menu below decides with them */
			break;
		}
		else {
			/* as gui/widget/hintbox.cpp: what neutrino handles (volume, the
			 * clock, events of the EPG or the timers) leaves the menu open;
			 * only what nobody takes, or a cancel, ends it */
			int res = CNeutrinoApp::getInstance()->handleMsg(msg, data);
			if (res & messages_return::cancel_all)
				break;
			if (res & messages_return::unhandled) {
				/* leave it for the caller to handle above... */
				g_RCInput->postMsg(msg, data);
				break;
			}
		}
	}

	hide();
	return selected;
}
