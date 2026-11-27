/*
 * script coverflow
 *
 * The coverflow menu a script shows, in the language the scripts are written
 * in: neither this nor anything it calls knows which interpreter asked for
 * it. A binding reads its own table, fills a ScriptCoverFlow::params and
 * hands it over.
 *
 * What the menu does while it is shown - left/right between the cards, ok to
 * pick one of them - is what a box of the program would do when a menu shows
 * one, and is one implementation here.
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

#ifndef _SCRIPTCOVERFLOW_H
#define _SCRIPTCOVERFLOW_H

#include <string>
#include <vector>

#include <gui/widget/coverflow.h>

/* The menu a script shows: one coverflow card per entry, the active card in
 * the middle of the screen and its neighbours beside it. Left and right move
 * between the cards, ok selects the one in the middle, back leaves without a
 * selection. */
class ScriptCoverFlow
{
	public:
		/* One entry of the menu: what its card shows. */
		typedef struct card {
			std::string	image;
			std::string	text;
		} card_t;

		/* What a script asked for when it made the menu. A binding fills
		 * this from its own table, so the field names are the ones the
		 * bindings' parameters are. */
		typedef struct params {
			std::vector<card_t>	cards;   /* the entries of the menu, in order */
			int	width;       /* width (logical px) of the card in the middle */
			int	max_lines;   /* most lines of text shown per card */
			int	pic_max_h;   /* maximum height (logical px) of a cover image */
		} params_t;

		explicit ScriptCoverFlow(const params_t &pars);
		virtual ~ScriptCoverFlow();

		/* Draw the menu at its current position: the active card in the
		 * middle, its neighbours beside it. */
		void	paint(void);
		/* Remove everything the menu drew and restore the screen behind it. */
		void	hide(void);
		/* Run the selection loop: left/right between the cards, ok picks the
		 * active one, back leaves. Returns the picked entry (1..n) or 0 when
		 * nothing was picked; a positive timeout of idle seconds closes the
		 * menu like leaving it does, without picking anything. */
		int	exec(int timeout);
		/* The active entry now, 1..n, for a binding that wants to show it. */
		int	getActive();

	private:
		std::vector<card_t>	cards;
		int	width, max_lines, pic_max_h;
		int	active_idx;   /* the card in the middle, from zero */
		int	last_dir;     /* the direction of the last move: +1 right, -1 left */
		std::vector<CCoverFlow*>	shown;  /* the cards on screen now */

		void	render(int active);
};

#endif /* _SCRIPTCOVERFLOW_H */
