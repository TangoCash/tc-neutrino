/*
 * python coverflow
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

#include "py_coverflow.h"

#include <config.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <global.h>
#include <system/debug.h>
#include <neutrino.h>

#include "pyinstance.h"
#include <scriptapi/script_ui.h>

typedef struct
{
	PyObject_HEAD
	CPyCoverFlow *d;
} PyCoverFlowObject;

static PyTypeObject *coverflow_type = NULL;

CPythonInstCoverFlow *CPythonInstCoverFlow::getInstance()
{
	static CPythonInstCoverFlow *PythonInstCoverFlow = NULL;

	if (!PythonInstCoverFlow)
		PythonInstCoverFlow = new CPythonInstCoverFlow();
	return PythonInstCoverFlow;
}

static CPyCoverFlow *coverflowOf(PyObject *self)
{
	PyCoverFlowObject *o = (PyCoverFlowObject *)self;
	return o->d;
}

/* Reads one text field of a card entry, be it a dictionary or an object. */
static bool cardFieldString(PyObject *item, const char *what, std::string &value)
{
	bool res = false;

	if (PyDict_Check(item)) {
		PyObject *v = PyDict_GetItemString((PyObject *)item, what);   /* borrowed */
		if (v != NULL && PyUnicode_Check(v)) {
			const char *s = PyUnicode_AsUTF8(v);
			if (s != NULL) {
				value = s;
				res = true;
			} else
				PyErr_Clear();
		}
	} else {
		PyObject *v = PyObject_GetAttrString(item, what);   /* new reference */
		if (v != NULL) {
			if (PyUnicode_Check(v)) {
				const char *s = PyUnicode_AsUTF8(v);
				if (s != NULL) {
					value = s;
					res = true;
				} else
					PyErr_Clear();
			}
			Py_DECREF(v);
		} else
			PyErr_Clear();
	}
	return res;
}

/* coverflow.new(cards=[{image="logo", text="text"}, ...], width=480, max_lines=5, pic_max_h=300, title="")
 *
 * The menu a script shows: one card per entry, the active one in the middle.
 * What is done with the parameters is done by the script API, so that both
 * bindings ask for one menu and cannot come to differ. */
PyObject *CPythonInstCoverFlow::CoverFlowNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	if (args != NULL && PyTuple_GET_SIZE(args) > 0)
	{
		PyErr_SetString(PyExc_TypeError, "coverflow.new() takes keyword arguments only");
		return NULL;
	}

	ScriptCoverFlow::params_t pars;
	pars.width     = COVERFLOW_DEFAULT_WIDTH;
	pars.max_lines = COVERFLOW_MAX_LINES;
	pars.pic_max_h = COVERFLOW_PIC_MAX_HEIGHT;

	long width = (long)pars.width;
	pyLookupInt(kwds, "width", width);
	pars.width = (int)width;

	long max_lines = (long)pars.max_lines;
	pyLookupInt(kwds, "max_lines", max_lines);
	pars.max_lines = (int)max_lines;

	long pic_max_h = (long)pars.pic_max_h;
	pyLookupInt(kwds, "pic_max_h", pic_max_h);
	pars.pic_max_h = (int)pic_max_h;

	pyLookupString(kwds, "title", pars.title);   /* the bar above the cards, optional */

	PyObject *cards = NULL;
	if (!pyLookupList(kwds, "cards", &cards)) {
		PyErr_SetString(PyExc_TypeError, "coverflow.new(): 'cards' must be a list of dictionaries with image and text");
		return NULL;
	}

	for (Py_ssize_t i = 0; i < PySequence_Size(cards); i++) {
		PyObject *item = PySequence_GetItem(cards, i);   /* new reference */
		if (item == NULL)
			return NULL;

		ScriptCoverFlow::card_t c;
		cardFieldString(item, "image", c.image);
		cardFieldString(item, "text", c.text);
		Py_DECREF(item);

		pars.cards.push_back(c);
	}
	if (pars.cards.empty()) {
		PyErr_SetString(PyExc_TypeError, "coverflow.new(): 'cards' must hold at least one card");
		return NULL;
	}

	PyCoverFlowObject *self = (PyCoverFlowObject *)type->tp_alloc(type, 0);
	if (self == NULL)
	{
		PyErr_Clear();
		return NULL;
	}

	self->d = new CPyCoverFlow(pars);
	return (PyObject *)self;
}

PyObject *CPythonInstCoverFlow::CoverFlowNewModule(PyObject * /*self*/, PyObject *args, PyObject *kwds)
{
	if (coverflow_type == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "coverflow is not registered");
		return NULL;
	}
	return CoverFlowNew(coverflow_type, args, kwds);
}

void CPythonInstCoverFlow::CoverFlowDelete(PyObject *self)
{
	PyCoverFlowObject *o = (PyCoverFlowObject *)self;
	delete o->d;
	o->d = NULL;
	Py_TYPE(self)->tp_free(self);
}

PyObject *CPythonInstCoverFlow::CoverFlowPaint(PyObject *self, PyObject * /*args*/, PyObject * /*kwds*/)
{
	CPyCoverFlow *D = coverflowOf(self);
	if (D != NULL)
		D->paint();
	Py_RETURN_NONE;
}

PyObject *CPythonInstCoverFlow::CoverFlowHide(PyObject *self, PyObject * /*args*/, PyObject * /*kwds*/)
{
	CPyCoverFlow *D = coverflowOf(self);
	if (D != NULL)
		D->hide();
	Py_RETURN_NONE;
}

/* b.exec(timeout=-1): runs the menu and hands back the picked entry (1..n),
 * or 0 when nothing was picked, with the loop the box runs when a menu of
 * the program shows it, which is the script API's. */
PyObject *CPythonInstCoverFlow::CoverFlowExec(PyObject *self, PyObject *args, PyObject *kwds)
{
	static const char *kwlist[] = { "timeout", NULL };
	int timeout = -1;
	CPyCoverFlow *D = coverflowOf(self);

	if (D == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "coverflow is gone");
		return NULL;
	}
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", (char **)kwlist, &timeout))
		return NULL;

	ScriptUi::open();
	int selected = D->exec(timeout);
	ScriptUi::close();
	return PyLong_FromLong(selected);
}

/* b.active(): the entry in the middle now, 1..n */
PyObject *CPythonInstCoverFlow::CoverFlowActive(PyObject *self, PyObject * /*args*/, PyObject * /*kwds*/)
{
	CPyCoverFlow *D = coverflowOf(self);
	if (D == NULL)
		return PyLong_FromLong(0);
	return PyLong_FromLong(D->getActive());
}


bool CPythonInstCoverFlow::CoverFlowRegister(PyObject *module)
{
	static PyMethodDef coverflow_methods[] =
	{
		PY_METHODDEF("exec",  CPythonInstCoverFlow::CoverFlowExec),
		PY_METHODDEF("active", CPythonInstCoverFlow::CoverFlowActive),
		PY_METHODDEF("paint", CPythonInstCoverFlow::CoverFlowPaint),
		PY_METHODDEF("hide",  CPythonInstCoverFlow::CoverFlowHide),
		{ NULL, NULL, 0, NULL }
	};

	static PyMethodDef module_methods[] =
	{
		PY_METHODDEF("new", CPythonInstCoverFlow::CoverFlowNewModule),
		{ NULL, NULL, 0, NULL }
	};

	static PyType_Slot coverflow_slots[] =
	{
		{ Py_tp_new, (void *)CPythonInstCoverFlow::CoverFlowNew },
		{ Py_tp_dealloc, (void *)CPythonInstCoverFlow::CoverFlowDelete },
		{ Py_tp_methods, (void *)coverflow_methods },
		{ Py_tp_alloc, (void *)PyType_GenericAlloc },
		{ Py_tp_free, (void *)PyObject_Free },
		{ Py_tp_doc, (void *)"coverflow" },
		{ 0, NULL }
	};

	static PyType_Spec coverflow_spec =
	{
		"neutrino.coverflow.coverflow",
		sizeof(PyCoverFlowObject),
		0,
		Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
		coverflow_slots
	};

	static PyModuleDef module_def =
	{
		PyModuleDef_HEAD_INIT,
		"neutrino.coverflow",
		"coverflow",
		-1,
		module_methods,
		NULL, NULL, NULL, NULL
	};

	PyObject *sub = PyModule_Create(&module_def);
	if (sub == NULL)
	{
		PyErr_Clear();
		return false;
	}

	coverflow_type = (PyTypeObject *)PyType_FromSpec(&coverflow_spec);
	if (coverflow_type == NULL)
	{
		PyErr_Clear();
		Py_DECREF(sub);
		return false;
	}
	if (PyModule_AddObject(sub, "coverflow", (PyObject *)coverflow_type) < 0)
	{
		PyErr_Clear();
		Py_DECREF((PyObject *)coverflow_type);
		coverflow_type = NULL;
		Py_DECREF(sub);
		return false;
	}

	return PythonInstAddSubmodule(module, "coverflow", sub);
}

