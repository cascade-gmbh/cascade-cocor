/*-------------------------------------------------------------------------
Compiler Generator Coco/R,
Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
extended by M. Loeberbauer & A. Woess, Univ. of Linz
ported to C++ by Csaba Balazs, University of Szeged
with improvements by Pat Terry, Rhodes University

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

As an exception, it is allowed to write an extension of Coco/R that is
used as a plugin in non-free software.

If not otherwise stated, any source code generated by Coco/R (other than
Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/

#include "Action.h"
#include "Target.h"
#include "CharSet.h"

namespace Coco {

Action::Action(int typ, int sym, int tc) {
	this->target = NULL;
	this->next   = NULL;

	this->typ = typ; this->sym = sym; this->tc = tc;
}

Action::~Action() {
    delete this->target;
    delete this->next;
}

bool Action::AddTarget(State *state) { // add t to the action.targets
	Target *last = NULL;
	Target *p = target;
	while (p != NULL && state->nr >= p->state->nr) {
		if (state == p->state) return false;
		last = p; p = p->next;
	}
        Target *t = new Target(state);
	t->next = p;
	if (p == target) target = t; else last->next = t;
        return true;
}

void Action::AddTargets(Action *a) {// add copy of a.targets to action.targets
	for (Target *p = a->target; p != NULL; p = p->next) {
		AddTarget(p->state);
	}
	if (a->tc == TransitionCode::contextTrans) tc = TransitionCode::contextTrans;
}

CharSet* Action::Symbols(Tab *tab) {
	CharSet *s;
	if (typ == NodeType::clas)
		s = tab->CharClassSet(sym)->Clone();
	else {
		s = new CharSet(); s->Set(sym);
	}
	return s;
}

bool Action::ShiftWith(CharSet *s, Tab *tab) { //return true if it used the CharSet *s
	bool rc = false;
	if (s->Elements() == 1) {
		typ = NodeType::chr; sym = s->First();
	} else {
		CharClass *c = tab->FindCharClass(s);
		if (c == NULL) {
                    c = tab->NewCharClass(_SC("#"), s); // class with dummy name
                    rc = true;
                }
		typ = NodeType::clas; sym = c->n;
	}
        return rc;
}

}; // namespace
