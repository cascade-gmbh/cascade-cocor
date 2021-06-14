/*----------------------------------------------------------------------
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
-----------------------------------------------------------------------*/


#include "Scanner.h"
#include "Parser.h"




#ifdef PARSER_WITH_AST

void Parser::AstAddTerminal() {
        SynTree *st_t = new SynTree( t->Clone() );
        ast_stack.Top()->children.Add(st_t);
}

bool Parser::AstAddNonTerminal(eNonTerminals kind, const wchar_t *nt_name, int line) {
        Token *ntTok = new Token();
        ntTok->kind = kind;
        ntTok->line = line;
        ntTok->val = coco_string_create(nt_name);
        SynTree *st = new SynTree( ntTok );
        ast_stack.Top()->children.Add(st);
        ast_stack.Add(st);
        return true;
}

void Parser::AstPopNonTerminal() {
        ast_stack.Pop();
}

#endif

void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(la->line, la->col, n);
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg) {
	if (errDist >= minErrDist) errors->Error(t->line, t->col, msg);
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::Test() {
#ifdef PARSER_WITH_AST
		Token *ntTok = new Token(); ntTok->kind = eNonTerminals::_Test; ntTok->line = 0; ntTok->val = coco_string_create(_SC("Test"));ast_root = new SynTree( ntTok ); ast_stack.Clear(); ast_stack.Add(ast_root);
#endif
		Expect(_ident);
#ifdef PARSER_WITH_AST
	AstAddTerminal();
#endif
#ifdef PARSER_WITH_AST
		AstPopNonTerminal();
#endif
}




// If the user declared a method Init and a mehtod Destroy they should
// be called in the contructur and the destructor respctively.
//
// The following templates are used to recognize if the user declared
// the methods Init and Destroy.

template<typename T>
struct ParserInitExistsRecognizer {
	template<typename U, void (U::*)() = &U::Init>
	struct ExistsIfInitIsDefinedMarker{};

	struct InitIsMissingType {
		char dummy1;
	};

	struct InitExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static InitIsMissingType is_here(...);

	// exist only if ExistsIfInitIsDefinedMarker is defined
	template<typename U>
	static InitExistsType is_here(ExistsIfInitIsDefinedMarker<U>*);

	enum { InitExists = (sizeof(is_here<T>(NULL)) == sizeof(InitExistsType)) };
};

template<typename T>
struct ParserDestroyExistsRecognizer {
	template<typename U, void (U::*)() = &U::Destroy>
	struct ExistsIfDestroyIsDefinedMarker{};

	struct DestroyIsMissingType {
		char dummy1;
	};

	struct DestroyExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static DestroyIsMissingType is_here(...);

	// exist only if ExistsIfDestroyIsDefinedMarker is defined
	template<typename U>
	static DestroyExistsType is_here(ExistsIfDestroyIsDefinedMarker<U>*);

	enum { DestroyExists = (sizeof(is_here<T>(NULL)) == sizeof(DestroyExistsType)) };
};

// The folloing templates are used to call the Init and Destroy methods if they exist.

// Generic case of the ParserInitCaller, gets used if the Init method is missing
template<typename T, bool = ParserInitExistsRecognizer<T>::InitExists>
struct ParserInitCaller {
	static void CallInit(T *t) {
		// nothing to do
	}
};

// True case of the ParserInitCaller, gets used if the Init method exists
template<typename T>
struct ParserInitCaller<T, true> {
	static void CallInit(T *t) {
		t->Init();
	}
};

// Generic case of the ParserDestroyCaller, gets used if the Destroy method is missing
template<typename T, bool = ParserDestroyExistsRecognizer<T>::DestroyExists>
struct ParserDestroyCaller {
	static void CallDestroy(T *t) {
		// nothing to do
	}
};

// True case of the ParserDestroyCaller, gets used if the Destroy method exists
template<typename T>
struct ParserDestroyCaller<T, true> {
	static void CallDestroy(T *t) {
		t->Destroy();
	}
};

void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(_SC("Dummy Token"));
	Get();
	Test();
	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 2;

	ParserInitCaller<Parser>::CallInit(this);
	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
        this->errors = new Errors(scanner->GetParserFileName());
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[1][4] = {
		{T,x,x,x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	ParserDestroyCaller<Parser>::CallDestroy(this);
	delete dummyToken;
	delete errors;
#ifdef PARSER_WITH_AST
        delete ast_root;
#endif

#ifdef COCO_FRAME_PARSER
        coco_string_delete(noString);
        coco_string_delete(tokenString);
#endif
}

Errors::Errors(const char * FileName) {
	count = 0;
	file = FileName;
}

void Errors::SynErr(int line, int col, int n) {
	const wchar_t* s;
	const size_t format_size = 20;
	wchar_t format[format_size];
	switch (n) {
			case 0: s = _SC("EOF expected"); break;
			case 1: s = _SC("ident expected"); break;
			case 2: s = _SC("??? expected"); break;

		default:
		{
			coco_swprintf(format, format_size, _SC("error %d"), n);
			s = format;
		}
		break;
	}
	wprintf(_SC("%s -- line %d col %d: %") _SFMT _SC("\n"), file, line, col, s);
	count++;
}

void Errors::Error(int line, int col, const wchar_t *s) {
	wprintf(_SC("%s -- line %d col %d: %") _SFMT _SC("\n"), file, line, col, s);
	count++;
}

void Errors::Warning(int line, int col, const wchar_t *s) {
	wprintf(_SC("%s -- line %d col %d: %") _SFMT _SC("\n"), file, line, col, s);
}

void Errors::Warning(const wchar_t *s) {
	wprintf(_SC("%") _SFMT _SC("\n"), s);
}

void Errors::Exception(const wchar_t* s) {
	wprintf(_SC("%") _SFMT _SC(""), s);
	exit(1);
}

#ifdef PARSER_WITH_AST

static void printIndent(int n) {
    for(int i=0; i < n; ++i) wprintf(_SC(" "));
}

SynTree::~SynTree() {
    //wprintf(_SC("Token %") _SFMT _SC(" : %d : %d : %d : %d\n"), tok->val, tok->kind, tok->line, tok->col, children.Count);
    delete tok;
    for(int i=0; i<children.Count; ++i) delete ((SynTree*)children[i]);
}

void SynTree::dump_all(int indent, bool isLast) {
        int last_idx = children.Count;
        if(tok->col) {
            printIndent(indent);
            wprintf(_SC("%s\t%d\t%d\t%d\t%") _SFMT _SC("\n"), ((isLast || (last_idx == 0)) ? "= " : " "), tok->line, tok->col, tok->kind, tok->val);
        }
        else {
            printIndent(indent);
            wprintf(_SC("%d\t%d\t%d\t%") _SFMT _SC("\n"), children.Count, tok->line, tok->kind, tok->val);
        }
        if(last_idx) {
                for(int idx=0; idx < last_idx; ++idx) ((SynTree*)children[idx])->dump_all(indent+4, idx == last_idx);
        }
}

void SynTree::dump_pruned(int indent, bool isLast) {
        int last_idx = children.Count;
        int indentPlus = 4;
        if(tok->col) {
            printIndent(indent);
            wprintf(_SC("%s\t%d\t%d\t%d\t%") _SFMT _SC("\n"), ((isLast || (last_idx == 0)) ? "= " : " "), tok->line, tok->col, tok->kind, tok->val);
        }
        else {
            if(last_idx == 1) {
                if(((SynTree*)children[0])->children.Count == 0) {
                    printIndent(indent);
                    wprintf(_SC("%d\t%d\t%d\t%") _SFMT _SC("\n"), children.Count, tok->line, tok->kind, tok->val);
                }
                else indentPlus = 0;
            }
            else {
                printIndent(indent);
                wprintf(_SC("%d\t%d\t%d\t%") _SFMT _SC("\n"), children.Count, tok->line, tok->kind, tok->val);
            }
        }
        if(last_idx) {
                for(int idx=0; idx < last_idx; ++idx) ((SynTree*)children[idx])->dump_pruned(indent+indentPlus, idx == last_idx);
        }
}

#endif



#ifndef WITH_STDCPP_LIB
/*
This code is to have an executable without libstd++ library dependency
g++ -g -Wall -fno-rtti -fno-exceptions  *.cpp -o YourParser
 */

// MSVC uses __cdecl calling convention for new/delete :-O
#ifdef _MSC_VER
#  define NEWDECL_CALL __cdecl
#else
#  define NEWDECL_CALL
#endif

extern "C" void __cxa_pure_virtual ()
{
    puts("__cxa_pure_virtual called\n");
    abort ();
}

void * NEWDECL_CALL operator new (size_t size)
{
    void *p = malloc (size);
    if(!p)
    {
        puts("not enough memory\n");
        abort ();
    }
    return p;
}

void * NEWDECL_CALL operator new [] (size_t size)
{
    return ::operator new(size);
}

void NEWDECL_CALL operator delete (void *p)
{
    if (p) free (p);
}

void NEWDECL_CALL operator delete [] (void *p)
{
    if (p) free (p);
}

void NEWDECL_CALL operator delete (void *p, size_t)
{
    if (p) free (p);
}
#endif //WITH_STDCPP_LIB
