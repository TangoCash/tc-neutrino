/*
 * lua coverflow
 *
 * The menu a script made, held as the shared ScriptCoverFlow: what is done
 * with it is the same for every interpreter, and this binding only fills its
 * parameters out of a lua table.
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

#ifndef _LUACOVERFLOW_H
#define _LUACOVERFLOW_H

#include <scriptapi/script_coverflow.h>

class CLuaCoverFlow : public ScriptCoverFlow
{
	public:
		CLuaCoverFlow(const params_t &pars) : ScriptCoverFlow(pars) {}
};

class CLuaInstCoverFlow
{
	public:
		CLuaInstCoverFlow() {};
		~CLuaInstCoverFlow() {};
		static CLuaInstCoverFlow* getInstance();
		static void CoverFlowRegister(lua_State *L);

	private:
		static CLuaCoverFlow *CoverFlowCheck(lua_State *L, int n);
		static int CoverFlowNew(lua_State *L);
		static int CoverFlowDelete(lua_State *L);
		static int CoverFlowExec(lua_State *L);
		static int CoverFlowActive(lua_State *L);
		static int CoverFlowPaint(lua_State *L);
		static int CoverFlowHide(lua_State *L);
};

#endif //_LUACOVERFLOW_H
