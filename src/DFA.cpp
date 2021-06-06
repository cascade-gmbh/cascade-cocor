/*-------------------------------------------------------------------------
DFA -- Generation of the Scanner Automaton
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

#include <stdlib.h>
#include "DFA.h"
#include "Tab.h"
#include "Parser.h"
#include "BitArray.h"
#include "Generator.h"

namespace Coco {

#define SZWC10 10
#define SZWC20 20
typedef wchar_t wchar_t_10[SZWC10+1];
typedef wchar_t wchar_t_20[SZWC20+1];

//---------- Output primitives
static wchar_t* DFACh(int ch, wchar_t_10 &format) {
	if (ch < _SC(' ') || ch >= 127 || ch == _SC('\'') || ch == _SC('\\'))
		coco_swprintf(format, SZWC10, _SC("%d"), (int) ch);
	else
		coco_swprintf(format, SZWC10, _SC("_SC('%") _CHFMT _SC("')"), (int) ch);
	format[SZWC10] = _SC('\0');
	return format;
}

static wchar_t* DFAChCond(int ch, wchar_t_20 &format) {
        wchar_t_10 fmt;
	wchar_t* res = DFACh(ch, fmt);
	coco_swprintf(format, SZWC20, _SC("ch == %") _SFMT, res);
	format[SZWC20] = _SC('\0');
	return format;
}

void DFA::PutRange(CharSet *s) {
        wchar_t_10 fmt1, fmt2;
	for (CharSet::Range *r = s->head; r != NULL; r = r->next) {
		if (r->from == r->to) {
			wchar_t *from = DFACh(r->from, fmt1);
			fwprintf(gen, _SC("ch == %") _SFMT, from);
		} else if (r->from == 0) {
			wchar_t *to = DFACh(r->to, fmt1);
			fwprintf(gen, _SC("ch <= %") _SFMT, to);
		} else {
			wchar_t *from = DFACh(r->from, fmt1);
			wchar_t *to = DFACh(r->to, fmt2);
			fwprintf(gen, _SC("(ch >= %") _SFMT _SC(" && ch <= %") _SFMT _SC(")"), from, to);
		}
		if (r->next != NULL) fputws(_SC(" || "), gen);
	}
}


//---------- State handling

State* DFA::NewState() {
	State *s = new State(); s->nr = ++lastStateNr;
	if (firstState == NULL) firstState = s; else lastState->next = s;
	lastState = s;
	return s;
}

void DFA::NewTransition(State *from, State *to, int typ, int sym, int tc) {
	Target *t = new Target(to);
	Action *a = new Action(typ, sym, tc); a->target = t;
	from->AddAction(a);
	if (typ == Node::clas) curSy->tokenKind = Symbol::classToken;
}

void DFA::CombineShifts() {
	State *state;
	Action *a, *b, *c;
	CharSet *seta, *setb;
	for (state = firstState; state != NULL; state = state->next) {
		for (a = state->firstAction; a != NULL; a = a->next) {
			b = a->next;
			while (b != NULL)
				if (a->target->state == b->target->state && a->tc == b->tc) {
					seta = a->Symbols(tab); setb = b->Symbols(tab);
					seta->Or(setb);
					if(!a->ShiftWith(seta, tab)) delete seta;
					c = b; b = b->next; state->DetachAction(c);
					delete setb;
				} else b = b->next;
		}
	}
}

void DFA::FindUsedStates(const State *state, BitArray *used) {
	if ((*used)[state->nr]) return;
	used->Set(state->nr, true);
	for (Action *a = state->firstAction; a != NULL; a = a->next)
		FindUsedStates(a->target->state, used);
}

static void deleteOnlyThisState(State **state) {
        (*state)->next = NULL;
        delete *state;
        *state = NULL;
}

void DFA::DeleteRedundantStates() {
	//State *newState = new State[State::lastNr + 1];
	State **newState = (State**) malloc (sizeof(State*) * (lastStateNr + 1));
	BitArray used(lastStateNr + 1);
	FindUsedStates(firstState, &used);
	// combine equal final states
	for (State *s1 = firstState->next; s1 != NULL; s1 = s1->next) // firstState cannot be final
		if (used[s1->nr] && s1->endOf != NULL && s1->firstAction == NULL && !(s1->ctx))
			for (State *s2 = s1->next; s2 != NULL; s2 = s2->next)
				if (used[s2->nr] && s1->endOf == s2->endOf && s2->firstAction == NULL && !(s2->ctx)) {
					used.Set(s2->nr, false); newState[s2->nr] = s1;
				}

	State *state;
	for (state = firstState; state != NULL; state = state->next)
		if (used[state->nr])
			for (Action *a = state->firstAction; a != NULL; a = a->next)
				if (!(used[a->target->state->nr]))
					a->target->state = newState[a->target->state->nr];
	// delete unused states
	lastState = firstState; lastStateNr = 0; // firstState has number 0
	State *state_to_delete = NULL;
	for (state = firstState->next; state != NULL; state = state->next) {
		if(state_to_delete) deleteOnlyThisState(&state_to_delete);
		if (used[state->nr]) {state->nr = ++lastStateNr; lastState = state;}
		else { lastState->next = state->next; state_to_delete = state;}
        }
	if(state_to_delete) deleteOnlyThisState(&state_to_delete);
	free (newState);
}

State* DFA::TheState(const Node *p) {
	State *state;
	if (p == NULL) {state = NewState(); state->endOf = curSy; return state;}
	else return p->state;
}

void DFA::Step(State *from, const Node *p, BitArray *stepped) {
	if (p == NULL) return;
	stepped->Set(p->n, true);

	if (p->typ == Node::clas || p->typ == Node::chr) {
		NewTransition(from, TheState(p->next), p->typ, p->val, p->code);
	} else if (p->typ == Node::alt) {
		Step(from, p->sub, stepped); Step(from, p->down, stepped);
	} else if (p->typ == Node::iter) {
		if (tab->DelSubGraph(p->sub)) {
			parser->SemErr(_SC("contents of {...} must not be deletable"));
			return;
		}
		if (p->next != NULL && !((*stepped)[p->next->n])) Step(from, p->next, stepped);
		Step(from, p->sub, stepped);
		if (p->state != from) {
			BitArray newStepped(tab->nodes.Count);
			Step(p->state, p, &newStepped);
		}
	} else if (p->typ == Node::opt) {
		if (p->next != NULL && !((*stepped)[p->next->n])) Step(from, p->next, stepped);
		Step(from, p->sub, stepped);
	}
}

// Assigns a state n.state to every node n. There will be a transition from
// n.state to n.next.state triggered by n.val. All nodes in an alternative
// chain are represented by the same state.
// Numbering scheme:
//  - any node after a chr, clas, opt, or alt, must get a new number
//  - if a nested structure starts with an iteration the iter node must get a new number
//  - if an iteration follows an iteration, it must get a new number
void DFA::NumberNodes(Node *p, State *state, bool renumIter) {
	if (p == NULL) return;
	if (p->state != NULL) return; // already visited;
	if ((state == NULL) || ((p->typ == Node::iter) && renumIter)) state = NewState();
	p->state = state;
	if (tab->DelGraph(p)) state->endOf = curSy;

	if (p->typ == Node::clas || p->typ == Node::chr) {
		NumberNodes(p->next, NULL, false);
	} else if (p->typ == Node::opt) {
		NumberNodes(p->next, NULL, false);
		NumberNodes(p->sub, state, true);
	} else if (p->typ == Node::iter) {
		NumberNodes(p->next, state, true);
		NumberNodes(p->sub, state, true);
	} else if (p->typ == Node::alt) {
		NumberNodes(p->next, NULL, false);
		NumberNodes(p->sub, state, true);
		NumberNodes(p->down, state, renumIter);
	}
}

void DFA::FindTrans (const Node *p, bool start, BitArray *marked) {
	if (p == NULL || (*marked)[p->n]) return;
	marked->Set(p->n, true);
	if (start) {
		BitArray stepped(tab->nodes.Count);
		Step(p->state, p, &stepped); // start of group of equally numbered nodes
	}

	if (p->typ == Node::clas || p->typ == Node::chr) {
		FindTrans(p->next, true, marked);
	} else if (p->typ == Node::opt) {
		FindTrans(p->next, true, marked); FindTrans(p->sub, false, marked);
	} else if (p->typ == Node::iter) {
		FindTrans(p->next, false, marked); FindTrans(p->sub, false, marked);
	} else if (p->typ == Node::alt) {
		FindTrans(p->sub, false, marked); FindTrans(p->down, false, marked);
	}
}

void DFA::ConvertToStates(Node *p, Symbol *sym) {
	curGraph = p; curSy = sym;
        if (tab->DelGraph(curGraph)) {
          parser->SemErr(_SC("token might be empty"));
          return;
        }
	NumberNodes(curGraph, firstState, true);
	BitArray ba(tab->nodes.Count);
	FindTrans(curGraph, true, &ba);
	if (p->typ == Node::iter) {
		ba.SetAll(false);
		Step(firstState, p, &ba);
	}
}

// match string against current automaton; store it either as a fixedToken or as a litToken
void DFA::MatchLiteral(wchar_t* s, Symbol *sym) {
	wchar_t *subS = coco_string_create(s, 1, coco_string_length(s)-2);
	s = tab->Unescape(subS);
	coco_string_delete(subS);
	int i, len = coco_string_length(s);
	State *state = firstState;
	Action *a = NULL;
	for (i = 0; i < len; i++) { // try to match s against existing DFA
		a = FindAction(state, s[i]);
		if (a == NULL) break;
		state = a->target->state;
	}
	// if s was not totally consumed or leads to a non-final state => make new DFA from it
	if (i != len || state->endOf == NULL) {
		state = firstState; i = 0; a = NULL;
		dirtyDFA = true;
	}
	for (; i < len; i++) { // make new DFA for s[i..len-1]
		State *to = NewState();
		NewTransition(state, to, Node::chr, s[i], Node::normalTrans);
		state = to;
	}
	coco_string_delete(s);
	Symbol *matchedSym = state->endOf;
	if (state->endOf == NULL) {
		state->endOf = sym;
	} else if (matchedSym->tokenKind == Symbol::fixedToken || (a != NULL && a->tc == Node::contextTrans)) {
		// s matched a token with a fixed definition or a token with an appendix that will be cut off
		const size_t format_size = 200;
		wchar_t format[format_size];
		coco_swprintf(format, format_size, _SC("tokens %") _SFMT _SC(" and %") _SFMT _SC(" cannot be distinguished"), sym->name, matchedSym->name);
		parser->SemErr(format);
	} else { // matchedSym == classToken || classLitToken
		matchedSym->tokenKind = Symbol::classLitToken;
		sym->tokenKind = Symbol::litToken;
	}
}

void DFA::SplitActions(State *state, Action *a, Action *b) {
	Action *c; CharSet *seta, *setb, *setc;
	seta = a->Symbols(tab); setb = b->Symbols(tab);
	if (seta->Equals(setb)) {
		a->AddTargets(b);
		state->DetachAction(b);
	} else if (seta->Includes(setb)) {
		setc = seta->Clone(); setc->Subtract(setb);
		b->AddTargets(a);
		if(!a->ShiftWith(setc, tab)) delete setc;
	} else if (setb->Includes(seta)) {
		setc = setb->Clone(); setc->Subtract(seta);
		a->AddTargets(b);
		if(!b->ShiftWith(setc, tab)) delete setc;
	} else {
		setc = seta->Clone(); setc->And(setb);
		seta->Subtract(setc);
		setb->Subtract(setc);
		if(!a->ShiftWith(seta, tab)) delete seta;
		if(!b->ShiftWith(setb, tab)) delete setb;
		c = new Action(0, 0, Node::normalTrans);  // typ and sym are set in ShiftWith
		c->AddTargets(a);
		c->AddTargets(b);
		if(!c->ShiftWith(setc, tab)) delete setc;
		state->AddAction(c);
		return; //don't need to delete anything
	}
	delete seta; delete setb;
}

bool DFA::Overlap(const Action *a, const Action *b) {
	CharSet *seta, *setb;
	if (a->typ == Node::chr)
		if (b->typ == Node::chr) return (a->sym == b->sym);
		else {setb = tab->CharClassSet(b->sym);	return setb->Get(a->sym);}
	else {
		seta = tab->CharClassSet(a->sym);
		if (b->typ == Node::chr) return seta->Get(b->sym);
		else {setb = tab->CharClassSet(b->sym); return seta->Intersects(setb);}
	}
}

bool DFA::MakeUnique(State *state) { // return true if actions were split
	bool changed = false;
	for (Action *a = state->firstAction; a != NULL; a = a->next)
		for (Action *b = a->next; b != NULL; b = b->next)
			if (Overlap(a, b)) {
				SplitActions(state, a, b);
				changed = true;
			}
	return changed;
}

void DFA::MeltStates(State *state) {
	bool changed, ctx;
	BitArray *targets;
	Symbol *endOf;
	for (Action *action = state->firstAction; action != NULL; action = action->next) {
		if (action->target->next != NULL) {
			GetTargetStates(action, targets, endOf, ctx);
			Melted *melt = StateWithSet(targets);
			if (melt == NULL) {
				State *s = NewState(); s->endOf = endOf; s->ctx = ctx;
				for (Target *targ = action->target; targ != NULL; targ = targ->next)
					s->MeltWith(targ->state);
				do {changed = MakeUnique(s);} while (changed);
				melt = NewMelted(targets, s);
			}
                        else delete targets;
			action->target->next = NULL;
			action->target->state = melt->state;
		}
	}
}

void DFA::FindCtxStates() {
	for (State *state = firstState; state != NULL; state = state->next)
		for (Action *a = state->firstAction; a != NULL; a = a->next)
			if (a->tc == Node::contextTrans) a->target->state->ctx = true;
}

void DFA::MakeDeterministic() {
	State *state;
	bool changed;
	lastSimState = lastState->nr;
	maxStates = 2 * lastSimState; // heuristic for set size in Melted.set
	FindCtxStates();
	for (state = firstState; state != NULL; state = state->next)
		do {changed = MakeUnique(state);} while (changed);
	for (state = firstState; state != NULL; state = state->next)
		MeltStates(state);
	DeleteRedundantStates();
	CombineShifts();
}

void DFA::PrintStates() {
	fwprintf(trace, _SC("\n---------- states ----------\n"));
        wchar_t_10 fmt;
	for (State *state = firstState; state != NULL; state = state->next) {
		bool first = true;
		if (state->endOf == NULL) fputws(_SC("               "), trace);
		else {
			wchar_t *paddedName = tab->Name(state->endOf->name);
			fwprintf(trace, _SC("E(%12") _SFMT _SC(")"), paddedName);
			coco_string_delete(paddedName);
		}
		fwprintf(trace, _SC("%3d:"), state->nr);
		if (state->firstAction == NULL) fputws(_SC("\n"), trace);
		for (Action *action = state->firstAction; action != NULL; action = action->next) {
			if (first) {fputws(_SC(" "), trace); first = false;} else fputws(_SC("                    "), trace);

			if (action->typ == Node::clas) fwprintf(trace, _SC("%") _SFMT, tab->classes[action->sym]->name);
			else fwprintf(trace, _SC("%3") _SFMT, DFACh(action->sym, fmt));
			for (Target *targ = action->target; targ != NULL; targ = targ->next) {
				fwprintf(trace, _SC("%3d"), targ->state->nr);
			}
			if (action->tc == Node::contextTrans) fputws(_SC(" context\n"), trace); else fputws(_SC("\n"), trace);
		}
	}
	fputws(_SC("\n---------- character classes ----------\n"), trace);
	tab->WriteCharClasses();
}

//---------------------------- actions --------------------------------

Action* DFA::FindAction(const State *state, int ch) {
	for (Action *a = state->firstAction; a != NULL; a = a->next)
		if (a->typ == Node::chr && ch == a->sym) return a;
		else if (a->typ == Node::clas) {
			CharSet *s = tab->CharClassSet(a->sym);
			if (s->Get(ch)) return a;
		}
	return NULL;
}


void DFA::GetTargetStates(const Action *a, BitArray* &targets, Symbol* &endOf, bool &ctx) {
	// compute the set of target states
	targets = new BitArray(maxStates); endOf = NULL;
	ctx = false;
	for (Target *t = a->target; t != NULL; t = t->next) {
		int stateNr = t->state->nr;
		if (stateNr <= lastSimState) { targets->Set(stateNr, true); }
		else { targets->Or(MeltedSet(stateNr)); }
		if (t->state->endOf != NULL) {
			if (endOf == NULL || endOf == t->state->endOf) {
				endOf = t->state->endOf;
			}
			else {
				wprintf(_SC("Tokens %") _SFMT _SC(" and %") _SFMT _SC(" cannot be distinguished\n"), endOf->name, t->state->endOf->name);
				errors->count++;
			}
		}
		if (t->state->ctx) {
			ctx = true;
			// The following check seems to be unnecessary. It reported an error
			// if a symbol + context was the prefix of another symbol, e.g.
			//   s1 = "a" "b" "c".
			//   s2 = "a" CONTEXT("b").
			// But this is ok.
			// if (t.state.endOf != null) {
			//   Console.WriteLine("Ambiguous context clause");
			//	 Errors.count++;
			// }
		}
	}
}


//------------------------- melted states ------------------------------


Melted* DFA::NewMelted(BitArray *set, State *state) {
	Melted *m = new Melted(set, state);
	m->next = firstMelted; firstMelted = m;
	return m;

}

const BitArray* DFA::MeltedSet(int nr) {
	Melted *m = firstMelted;
	while (m != NULL) {
		if (m->state->nr == nr) return m->set; else m = m->next;
	}
	//Errors::Exception("-- compiler error in Melted::Set");
	//throw new Exception("-- compiler error in Melted::Set");
	return NULL;
}

Melted* DFA::StateWithSet(const BitArray *s) {
	for (Melted *m = firstMelted; m != NULL; m = m->next)
		if (Sets::Equals(s, m->set)) return m;
	return NULL;
}


//------------------------ comments --------------------------------

wchar_t* DFA::CommentStr(const Node *p) {
	StringBuilder s;
	while (p != NULL) {
		if (p->typ == Node::chr) {
			s.Append((wchar_t)p->val);
		} else if (p->typ == Node::clas) {
			CharSet *set = tab->CharClassSet(p->val);
			if (set->Elements() != 1) parser->SemErr(_SC("character set contains more than 1 character"));
			s.Append((wchar_t) set->First());
		}
		else parser->SemErr(_SC("comment delimiters may not be structured"));
		p = p->next;
	}
	if (s.GetLength() == 0 || s.GetLength() > 8) {
		parser->SemErr(_SC("comment delimiters must be 1 or 8 characters long"));
		s = StringBuilder(_SC("?"));
	}
	return s.ToString();
}


void DFA::NewComment(const Node *from, const Node *to, bool nested) {
	Comment *c = new Comment(CommentStr(from), CommentStr(to), nested, false);
	c->next = firstComment; firstComment = c;
}


//------------------------ scanner generation ----------------------

void DFA::GenCommentIndented(int n, const wchar_t *s) {
        for(int i= 1; i < n; ++i) fputws(_SC("\t"), gen);
        fputws(s, gen);
}

void DFA::GenComBody(const Comment *com) {
	int imax = coco_string_length(com->start)-1;
	int imaxStop = coco_string_length(com->stop)-1;
	GenCommentIndented(imax, _SC("\t\tfor(;;) {\n"));

	wchar_t_20 fmt;
	wchar_t* res = DFAChCond(com->stop[0], fmt);
	GenCommentIndented(imax, _SC("\t\t\tif ("));
	fwprintf(gen, _SC("%") _SFMT _SC(") {\n"), res);

	if (imaxStop == 0) {
		fwprintf(gen, _SC("%s"), 
                        "\t\t\t\tlevel--;\n"
                        "\t\t\t\tif (level == 0) { oldEols = line - line0; NextCh(); return true; }\n"
                        "\t\t\t\tNextCh();\n");
	} else {
                int currIndent, indent = imax - 1;
                for(int sidx = 1; sidx <= imaxStop; ++sidx) {
                        currIndent = indent + sidx;
                        GenCommentIndented(currIndent, _SC("\t\t\t\tNextCh();\n"));
                        GenCommentIndented(currIndent, _SC("\t\t\t\tif ("));
                        fwprintf(gen, _SC("%") _SFMT _SC(") {\n"), DFAChCond(com->stop[sidx], fmt));
                }
                currIndent = indent + imax;
                GenCommentIndented(currIndent, _SC("\t\t\tlevel--;\n"));
                GenCommentIndented(currIndent, _SC("\t\t\tif (level == 0) { /*oldEols = line - line0;*/ NextCh(); return true; }\n"));
                GenCommentIndented(currIndent, _SC("\t\t\tNextCh();\n"));
                for(int sidx = imaxStop; sidx > 0; --sidx) {
                        GenCommentIndented(indent + sidx, _SC("\t\t\t\t}\n"));
                }
	}
	if (com->nested) {
		GenCommentIndented(imax, _SC("\t\t\t}"));
		wchar_t* res = DFAChCond(com->start[0], fmt);
		fwprintf(gen, _SC(" else if (%") _SFMT _SC(") {\n"), res);
		if (imaxStop == 0)
			fputws(_SC("\t\t\tlevel++; NextCh();\n"), gen);
		else {
                        int indent = imax - 1;
                        for(int sidx = 1; sidx <= imax; ++sidx) {
                                int loopIndent = indent + sidx;
                                GenCommentIndented(loopIndent, _SC("\t\t\t\tNextCh();\n"));
                                GenCommentIndented(loopIndent, _SC("\t\t\t\tif ("));
                                fwprintf(gen, _SC("%") _SFMT _SC(") {\n"), DFAChCond(com->start[sidx], fmt));
                        }
                        GenCommentIndented(indent + imax, _SC("\t\t\t\t\tlevel++; NextCh();\n"));
                        for(int sidx = imax; sidx > 0; --sidx) {
                                GenCommentIndented(indent + sidx, _SC("\t\t\t\t}\n"));
                        }
		}
	}
	GenCommentIndented(imax, _SC("\t\t\t} else if (ch == buffer->EoF) return false;\n"));
	GenCommentIndented(imax, _SC("\t\t\telse NextCh();\n"));
	GenCommentIndented(imax, _SC("\t\t}\n"));
}

void DFA::GenCommentHeader(const Comment *com, int i) {
	fwprintf(gen, _SC("\tbool Comment%d();\n"), i);
}

void DFA::GenComment(const Comment *com, int i) {
        wchar_t_20 fmt;
	fwprintf(gen, _SC("\nbool Scanner::Comment%d() {\n"), i);
	fwprintf(gen, _SC("%s"),
                    "\tint level = 1, pos0 = pos, line0 = line, col0 = col, charPos0 = charPos;\n"
                    "\tNextCh();\n");
	int imax = coco_string_length(com->start)-1;
	if (imax == 0) {
		GenComBody(com);
	} else {
                for(int sidx = 1; sidx <= imax; ++sidx) {
                        GenCommentIndented(sidx, _SC("\tif ("));
                        fwprintf(gen, _SC("%") _SFMT _SC(") {\n"), DFAChCond(com->start[sidx], fmt));
                        GenCommentIndented(sidx, _SC("\t\tNextCh();\n"));
                }
                GenComBody(com);
                for(int sidx = imax; sidx > 0; --sidx) {
                        GenCommentIndented(sidx, _SC("\t}\n"));
                }
		fwprintf(gen, _SC("%s"),
                        "\tbuffer->SetPos(pos0); NextCh(); line = line0; col = col0; charPos = charPos0;\n"
                        "\treturn false;\n");
	}
	fputws(_SC("}\n"), gen);
}

const wchar_t* DFA::SymName(const Symbol *sym) { // real name value is stored in Tab.literals
	if (('a'<=sym->name[0] && sym->name[0]<='z') ||
		('A'<=sym->name[0] && sym->name[0]<='Z')) { //Char::IsLetter(sym->name[0])

		Iterator *iter = tab->literals.GetIterator();
		while (iter->HasNext()) {
			DictionaryEntry *e = iter->Next();
			if (e->val == sym) { delete iter; return e->key; }
		}
                delete iter;
	}
	return sym->name;
}

void DFA::GenLiterals () {
	Symbol *sym;

	TArrayList<Symbol*> *ts[2];
	ts[0] = &tab->terminals;
	ts[1] = &tab->pragmas;

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < ts[i]->Count; j++) {
			sym = (Symbol*) ((*(ts[i]))[j]);
			if (sym->tokenKind == Symbol::litToken) {
				wchar_t* name = coco_string_create(SymName(sym));
				if (ignoreCase) {
					wchar_t *oldName = name;
					name = coco_string_create_lower(name);
					coco_string_delete(oldName);
				}
				// sym.name stores literals with quotes, e.g. "\"Literal\""

				fputws(_SC("\tkeywords.set(_SC("), gen);
				// write keyword, escape non printable characters
				for (int k = 0; name[k] != _SC('\0'); k++) {
					wchar_t c = name[k];
					fwprintf(gen, (c >= 32 && c <= 127) ? _SC("%") _CHFMT : _SC("\\x%04x"), c);
				}
				fwprintf(gen, _SC("), %d);\n"), sym->n);

				coco_string_delete(name);
			}
		}
	}
}

int DFA::GenNamespaceOpen(const wchar_t *nsName) {
	if (nsName == NULL || coco_string_length(nsName) == 0) {
		return 0;
	}
	const int len = coco_string_length(nsName);
	int startPos = 0;
	int nrOfNs = 0;
	do {
		int curLen = coco_string_indexof(nsName + startPos, COCO_CPP_NAMESPACE_SEPARATOR);
		if (curLen == -1) { curLen = len - startPos; }
		wchar_t *curNs = coco_string_create(nsName, startPos, curLen);
		fwprintf(gen, _SC("namespace %") _SFMT _SC(" {\n"), curNs);
		coco_string_delete(curNs);
		startPos = startPos + curLen + 1;
		if (startPos < len && nsName[startPos] == COCO_CPP_NAMESPACE_SEPARATOR) {
			++startPos;
		}
		++nrOfNs;
	} while (startPos < len);
	return nrOfNs;
}

void DFA::GenNamespaceClose(int nrOfNs) {
	for (int i = 0; i < nrOfNs; ++i) {
		fputws(_SC("} // namespace\n"), gen);
	}
}

void DFA::CheckLabels() {
	int i;
	State *state;
	Action *action;

	for (i=0; i < lastStateNr+1; i++) {
		existLabel[i] = false;
	}

	for (state = firstState->next; state != NULL; state = state->next) {
		for (action = state->firstAction; action != NULL; action = action->next) {
			existLabel[action->target->state->nr] = true;
		}
	}
}

/* TODO better interface for CopySourcePart */
void DFA::CopySourcePart (const Position *pos, int indent) {
        // Copy text described by pos from atg to gen
        int oldPos = parser->pgen->buffer->GetPos();  // Pos is modified by CopySourcePart
        FILE* prevGen = parser->pgen->gen;
        parser->pgen->gen = gen;
        parser->pgen->CopySourcePart(pos, 0);
        parser->pgen->gen = prevGen;
        parser->pgen->buffer->SetPos(oldPos);
}

void DFA::WriteState(const State *state) {
	Symbol *endOf = state->endOf;
	fwprintf(gen, _SC("\t\tcase %d:\n"), state->nr);
	if (existLabel[state->nr])
		fwprintf(gen, _SC("\t\t\tcase_%d:\n"), state->nr);

	if (endOf != NULL && state->firstAction != NULL) {
		fwprintf(gen, _SC("\t\t\trecEnd = pos; recKind = %d /* %") _SFMT _SC(" */;\n"), endOf->n, endOf->name);
	}
	bool ctxEnd = state->ctx;

        wchar_t_20 fmt;
	for (Action *action = state->firstAction; action != NULL; action = action->next) {
		if (action == state->firstAction) fputws(_SC("\t\t\tif ("), gen);
		else fputws(_SC("\t\t\telse if ("), gen);
		if (action->typ == Node::chr) {
			wchar_t* res = DFAChCond(action->sym, fmt);
			fwprintf(gen, _SC("%") _SFMT, res);
		} else PutRange(tab->CharClassSet(action->sym));
		fputws(_SC(") {"), gen);

		if (action->tc == Node::contextTrans) {
			fputws(_SC("apx++; "), gen); ctxEnd = false;
		} else if (state->ctx)
			fputws(_SC("apx = 0; "), gen);
		fwprintf(gen, _SC("AddCh(); goto case_%d;}\n"), action->target->state->nr);
	}
	if (state->firstAction == NULL)
		fputws(_SC("\t\t\t{"), gen);
	else
		fputws(_SC("\t\t\telse {"), gen);
	if (ctxEnd) { // final context state: cut appendix
		fwprintf(gen, _SC("%s"),
                            "\n"
                            "\t\t\t\ttlen -= apx;\n"
                            "\t\t\t\tSetScannerBehindT();"
                            "\t\t\t\tbuffer->SetPos(t->pos); NextCh(); line = t->line; col = t->col;\n"
                            "\t\t\t\tfor (int i = 0; i < tlen; i++) NextCh();\n"
                            "\t\t\t\t");
	}
	if (endOf == NULL) {
		fputws(_SC("goto case_0;}\n"), gen);
	} else {
		fwprintf(gen, _SC("t->kind = %d /* %") _SFMT _SC(" */; "), endOf->n, endOf->name);
		if (endOf->tokenKind == Symbol::classLitToken) {
			if (ignoreCase) {
				fwprintf(gen, _SC("%s"), "t->kind = keywords.get(tval, tlen, t->kind, true); loopState = false; break;}\n");
			} else {
				fwprintf(gen, _SC("%s"), "t->kind = keywords.get(tval, tlen, t->kind, false); loopState = false; break;}\n");
			}
		} else {
			fputws(_SC("loopState = false;"), gen);
			if(endOf->semPos && endOf->typ == Node::t) CopySourcePart(endOf->semPos, 0);
			fputws(_SC(" break;}\n"), gen);
		}
	}
}

void DFA::WriteStartTab() {
	bool firstRange = true;
	for (Action *action = firstState->firstAction; action != NULL; action = action->next) {
		int targetState = action->target->state->nr;
		if (action->typ == Node::chr) {
			fwprintf(gen, _SC("\tstart.set(%d, %d);\n"), action->sym, targetState);
		} else {
			CharSet *s = tab->CharClassSet(action->sym);
			for (CharSet::Range *r = s->head; r != NULL; r = r->next) {
				if (firstRange) {
					firstRange = false;
					fputws(_SC("\tint i;\n"), gen);
				}
				fwprintf(gen, _SC("\tfor (i = %d; i <= %d; ++i) start.set(i, %d);\n"), r->from, r->to, targetState);
			}
		}
	}
	fwprintf(gen, _SC("%s"), "\t\tstart.set(Buffer::EoF, -1);\n");
}

void DFA::WriteScanner() {
	Generator g(tab, errors);
	fram = g.OpenFrame(_SC("Scanner.frame"));
	gen = g.OpenGen(_SC("Scanner.h"));
	if (dirtyDFA) MakeDeterministic();

	// Header
	g.GenCopyright();
	g.SkipFramePart(_SC("-->begin"));
	
	g.CopyFramePart(_SC("-->prefix"));
	g.GenPrefixFromNamespace();

	g.CopyFramePart(_SC("-->prefix"));
	g.GenPrefixFromNamespace();

	g.CopyFramePart(_SC("-->namespace_open"));
	int nrOfNs = GenNamespaceOpen(tab->nsName);

	g.CopyFramePart(_SC("-->casing0"));
	if (ignoreCase) {
		fwprintf(gen, _SC("%s"), "\twchar_t valCh;       // current input character (for token.val)\n");
	}
	g.CopyFramePart(_SC("-->commentsheader"));
	Comment *com = firstComment;
	int cmdIdx = 0;
	while (com != NULL) {
		GenCommentHeader(com, cmdIdx);
		com = com->next; cmdIdx++;
	}

	g.CopyFramePart(_SC("-->namespace_close"));
	GenNamespaceClose(nrOfNs);

	g.CopyFramePart(_SC("-->implementation"));
	fclose(gen);

	// Source
	gen = g.OpenGen(_SC("Scanner.cpp"));
	g.GenCopyright();
	g.SkipFramePart(_SC("-->begin"));
	g.CopyFramePart(_SC("-->namespace_open"));
	nrOfNs = GenNamespaceOpen(tab->nsName);

	g.CopyFramePart(_SC("-->declarations"));
	fwprintf(gen, _SC("\tmaxT = %d;\n"), tab->terminals.Count - 1);
	fwprintf(gen, _SC("\tnoSym = %d;\n"), tab->noSym->n);
	WriteStartTab();
	GenLiterals();

	g.CopyFramePart(_SC("-->initialization"));
	g.CopyFramePart(_SC("-->casing1"));
	if (ignoreCase) {
		fwprintf(gen, _SC("%s"),
                        "\t\tvalCh = ch;\n"
                        "\t\tif ('A' <= ch && ch <= 'Z') ch = ch - 'A' + 'a'; // ch.ToLower()");
	}
	g.CopyFramePart(_SC("-->casing2"));
	fputws(_SC("\t\ttval[tlen++] = "), gen);
	if (ignoreCase) fputws(_SC("valCh;"), gen); else fputws(_SC("ch;"), gen);

	g.CopyFramePart(_SC("-->comments"));
	com = firstComment; cmdIdx = 0;
	while (com != NULL) {
		GenComment(com, cmdIdx);
		com = com->next; cmdIdx++;
	}

	g.CopyFramePart(_SC("-->scan1"));
	fputws(_SC("\t\t\t"), gen);
	if (tab->ignored->Elements() > 0) { PutRange(tab->ignored); } else { fputws(_SC("false"), gen); }

	g.CopyFramePart(_SC("-->scan2"));
	if (firstComment != NULL) {
		fputws(_SC("\t\tif ("), gen);
		com = firstComment; cmdIdx = 0;
                wchar_t_20 fmt;
		while (com != NULL) {
			wchar_t* res = DFAChCond(com->start[0], fmt);
			fwprintf(gen, _SC("(%") _SFMT _SC(" && Comment%d())"), res, cmdIdx);
			if (com->next != NULL) {
				fputws(_SC(" || "), gen);
			}
			com = com->next; cmdIdx++;
		}
		fputws(_SC(") continue;"), gen);
	}
        g.CopyFramePart(_SC("-->scan22"));
	if (hasCtxMoves) { fputws(_SC("\n\tint apx = 0;"), gen); } /* pdt */
	g.CopyFramePart(_SC("-->scan3"));

	/* CSB 02-10-05 check the Labels */
	existLabel = new bool[lastStateNr+1];
	CheckLabels();
	for (State *state = firstState->next; state != NULL; state = state->next)
		WriteState(state);
	delete [] existLabel;

	g.CopyFramePart(_SC("-->namespace_close"));
	GenNamespaceClose(nrOfNs);

	g.CopyFramePart(NULL);
	fclose(gen);
}

DFA::DFA(Parser *parser) {
	this->parser = parser;
	tab = parser->tab;
	errors = &parser->errors;
	trace = parser->trace;
	firstState = NULL; lastState = NULL; lastStateNr = -1;
	firstState = NewState();
	firstMelted = NULL; firstComment = NULL;
	ignoreCase = false;
	dirtyDFA = false;
	hasCtxMoves = false;
}

DFA::~DFA() {
    delete firstState;
    delete firstComment;
    delete firstMelted;
}

}; // namespace
