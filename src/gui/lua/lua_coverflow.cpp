/*
 * lua coverflow
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

#include <config.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <global.h>
#include <system/debug.h>
#include <neutrino.h>

#include "luainstance.h"
#include "lua_coverflow.h"
#include <scriptapi/script_ui.h>

CLuaInstCoverFlow* CLuaInstCoverFlow::getInstance()
{
	static CLuaInstCoverFlow* LuaInstCoverFlow = NULL;

	if(!LuaInstCoverFlow)
		LuaInstCoverFlow = new CLuaInstCoverFlow();
	return LuaInstCoverFlow;
}

void CLuaInstCoverFlow::CoverFlowRegister(lua_State *L)
{
	luaL_Reg meth[] = {
		{ "new", CLuaInstCoverFlow::CoverFlowNew },
		{ "exec", CLuaInstCoverFlow::CoverFlowExec },
		{ "active", CLuaInstCoverFlow::CoverFlowActive },
		{ "paint", CLuaInstCoverFlow::CoverFlowPaint },
		{ "hide", CLuaInstCoverFlow::CoverFlowHide },
		{ "__gc", CLuaInstCoverFlow::CoverFlowDelete },
		{ NULL, NULL }
	};

	luaL_newmetatable(L, "coverflow");
	luaL_setfuncs(L, meth, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	lua_setglobal(L, "coverflow");
}

CLuaCoverFlow *CLuaInstCoverFlow::CoverFlowCheck(lua_State *L, int n)
{
	return *(CLuaCoverFlow **) luaL_checkudata(L, n, "coverflow");
}

int CLuaInstCoverFlow::CoverFlowNew(lua_State *L)
{
	lua_assert(lua_istable(L,1));

	/* The parameters a script names in its table. What is done with them is
	 * done by the script API, so that both bindings ask for one menu and
	 * cannot come to differ. */
	ScriptCoverFlow::params_t pars;
	pars.width     = COVERFLOW_DEFAULT_WIDTH;
	pars.max_lines = COVERFLOW_MAX_LINES;
	pars.pic_max_h = COVERFLOW_PIC_MAX_HEIGHT;

	lua_Integer width = (lua_Integer)pars.width;
	tableLookup(L, "width", width);
	pars.width = (int)width;

	lua_Integer max_lines = (lua_Integer)pars.max_lines;
	tableLookup(L, "max_lines", max_lines);
	pars.max_lines = (int)max_lines;

	lua_Integer pic_max_h = (lua_Integer)pars.pic_max_h;
	tableLookup(L, "pic_max_h", pic_max_h);
	pars.pic_max_h = (int)pic_max_h;

	tableLookup(L, "title", pars.title);   /* the bar above the cards, optional */

	/* the cards of the menu: a list of tables with image and text */
	lua_getfield(L, 1, "cards");
	if (!lua_istable(L, -1)) {
		luaL_error(L, "coverflow.new(): 'cards' must be a list of tables with image and text\n");
	}

	int n = 0;
	for (int i = 1; ; i++) {
		lua_pushinteger(L, i);        /* index into cards */
		lua_gettable(L, -2);          /* cards[i] on top */
		if (!lua_istable(L, -1)) {   /* the end of the list: nil or a non-table entry */
			lua_pop(L, 1);
			break;
		}

		ScriptCoverFlow::card_t c;    /* the card is below its fields while they are read */
		lua_pushstring(L, "image");
		lua_gettable(L, -2);
		if (lua_isstring(L, -1))
			c.image = lua_tostring(L, -1);
		lua_pop(L, 1);

		lua_pushstring(L, "text");
		lua_gettable(L, -2);
		if (lua_isstring(L, -1))
			c.text = lua_tostring(L, -1);
		lua_pop(L, 1);

		pars.cards.push_back(c);
		lua_pop(L, 1);                /* drop the card: lua_gettable took the index already */
		n++;
	}
	if (n == 0) {
		luaL_error(L, "coverflow.new(): 'cards' must hold at least one card\n");
	}

	CLuaCoverFlow **udata = (CLuaCoverFlow **) lua_newuserdata(L, sizeof(CLuaCoverFlow *));
	*udata = new CLuaCoverFlow(pars);
	luaL_getmetatable(L, "coverflow");
	lua_setmetatable(L, -2);
	return 1;
}

int CLuaInstCoverFlow::CoverFlowDelete(lua_State *L)
{
	CLuaCoverFlow *D = CoverFlowCheck(L, 1);
	if (!D) return 0;
	delete D;
	return 0;
}

int CLuaInstCoverFlow::CoverFlowPaint(lua_State *L)
{
	CLuaCoverFlow *D = CoverFlowCheck(L, 1);
	if (!D) return 0;
	D->paint();
	return 0;
}

int CLuaInstCoverFlow::CoverFlowHide(lua_State *L)
{
	CLuaCoverFlow *D = CoverFlowCheck(L, 1);
	if (!D) return 0;
	D->hide();
	return 0;
}

/* b:exec([timeout]): runs the menu and hands back the picked entry (1..n),
 * or 0 when nothing was picked. */
int CLuaInstCoverFlow::CoverFlowExec(lua_State *L)
{
	CLuaCoverFlow *D = CoverFlowCheck(L, 1);
	if (!D) return 0;

	int timeout = -1;
	if (lua_isnumber(L, -1))
		timeout = (int) lua_tonumber(L, -1);

	ScriptUi::open();
	lua_pushinteger(L, D->exec(timeout));
	ScriptUi::close();
	return 1;
}

/* b:active(): the entry in the middle now, 1..n */
int CLuaInstCoverFlow::CoverFlowActive(lua_State *L)
{
	CLuaCoverFlow *D = CoverFlowCheck(L, 1);
	if (!D) return 0;
	lua_pushinteger(L, D->getActive());
	return 1;
}
