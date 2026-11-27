/*
 * python coverflow
 *
 * The menu a script shows, the counterpart of the Lua binding in
 * lua_coverflow.cpp, read as
 *
 *	from neutrino import coverflow
 *	b = coverflow.new(cards=[{"image": "logo", "text": "text"}])
 *	sel = b.exec()      # the picked entry (1..n), or 0 when nothing was picked
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

#ifndef _PYCOVERFLOW_H
#define _PYCOVERFLOW_H

#include "pyinstance_helpers.h"

#include <gui/widget/coverflow.h>

#include <scriptapi/script_coverflow.h>

/* The menu a script made, held as the shared ScriptCoverFlow: what is done
 * with it is the same for every interpreter, and this binding only fills its
 * parameters out of its call. */
class CPyCoverFlow : public ScriptCoverFlow
{
	public:
		CPyCoverFlow(const params_t &pars) : ScriptCoverFlow(pars) {}
};

class CPythonInstCoverFlow
{
	public:
		CPythonInstCoverFlow() {};
		~CPythonInstCoverFlow() {};
		static CPythonInstCoverFlow *getInstance();
		static bool CoverFlowRegister(PyObject *module);

	private:
		static PyObject *CoverFlowNew(PyTypeObject *type, PyObject *args, PyObject *kwds);
		static PyObject *CoverFlowNewModule(PyObject *self, PyObject *args, PyObject *kwds);
		static void CoverFlowDelete(PyObject *self);
		static PyObject *CoverFlowExec(PyObject *self, PyObject *args, PyObject *kwds);
		static PyObject *CoverFlowActive(PyObject *self, PyObject *args, PyObject *kwds);
		static PyObject *CoverFlowPaint(PyObject *self, PyObject *args, PyObject *kwds);
		static PyObject *CoverFlowHide(PyObject *self, PyObject *args, PyObject *kwds);
};

#endif //_PYCOVERFLOW_H
