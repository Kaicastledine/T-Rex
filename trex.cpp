/* see copyright notice in trex.h */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#include "trex.h"

typedef int TRexNodeType;

typedef struct tagTRexNode{
    TRexNodeType type;
    int left;
    int right;
    int next;
}TRexNode;

template < typename TChar > 
class TRexTraits
{
public:
    struct TRex
    {

    };
};

template<> class TRexTraits< TRexWChar >
{
public:
    typedef TRexWMatch TRexMatchType;

    static int scisprint(int _C){ return iswprint(_C); }
    static int scstrlen(TRexWChar* _psz){ return wcslen(_psz); }
    static int scprintf(TRexWChar* fmt, ...)
    {
        va_list v;
        va_start(v, fmt);
        int i = vwprintf(fmt, v);
        va_end(v);
        return i;
    }
    struct TRex{
        const TRexWChar *_eol;
        const TRexWChar *_bol;
        const TRexWChar *_p;
        int _first;
        int _op;
        TRexNode *_nodes;
        int _nallocated;
        int _nsize;
        int _nsubexpr;
        TRexMatchType *_matches;
        int _currsubexp;
        void *_jmpbuf;
        const TRexChar **_error;
    };

    static const int MAX_CHAR      = 0xFFFF;

    static const int OP_GREEDY     = (MAX_CHAR+1); // * + ? {n}
    static const int OP_OR         = (MAX_CHAR+2);
    static const int OP_EXPR       = (MAX_CHAR+3); //parentesis ()
    static const int OP_NOCAPEXPR  = (MAX_CHAR+4); //parentesis (?:)
    static const int OP_DOT        = (MAX_CHAR+5);
    static const int OP_CLASS      = (MAX_CHAR+6);
    static const int OP_CCLASS     = (MAX_CHAR+7);
    static const int OP_NCLASS     = (MAX_CHAR+8); //negates class the [^
    static const int OP_RANGE      = (MAX_CHAR+9);
    static const int OP_CHAR       = (MAX_CHAR+10);
    static const int OP_EOL        = (MAX_CHAR+11);
    static const int OP_BOL        = (MAX_CHAR+12);
    static const int OP_WB         = (MAX_CHAR+13);

    typedef TRexWMatch TRexMatchType;
};

template<> class TRexTraits< TRexChar >
{
public:
    typedef TRexMatch TRexMatchType;

    static int scisprint(int _C){ return isprint(_C); }
    static int scstrlen(TRexChar* _psz){ return strlen(_psz); }
    static int scprintf(TRexChar* fmt, ...)
    {
        va_list v;
        va_start(v, fmt);
        int i = vprintf(fmt, v);
        va_end(v);
        return i;
    }

    struct TRex{
        const TRexChar *_eol;
        const TRexChar *_bol;
        const TRexChar *_p;
        int _first;
        int _op;
        TRexNode *_nodes;
        int _nallocated;
        int _nsize;
        int _nsubexpr;
        TRexMatchType *_matches;
        int _currsubexp;
        void *_jmpbuf;
        const TRexChar **_error;
    };

    static const int MAX_CHAR      = 0xFF;

    static const int OP_GREEDY     = (MAX_CHAR+1); // * + ? {n}
    static const int OP_OR         = (MAX_CHAR+2);
    static const int OP_EXPR       = (MAX_CHAR+3); //parentesis ()
    static const int OP_NOCAPEXPR  = (MAX_CHAR+4); //parentesis (?:)
    static const int OP_DOT        = (MAX_CHAR+5);
    static const int OP_CLASS      = (MAX_CHAR+6);
    static const int OP_CCLASS     = (MAX_CHAR+7);
    static const int OP_NCLASS     = (MAX_CHAR+8); //negates class the [^
    static const int OP_RANGE      = (MAX_CHAR+9);
    static const int OP_CHAR       = (MAX_CHAR+10);
    static const int OP_EOL        = (MAX_CHAR+11);
    static const int OP_BOL        = (MAX_CHAR+12);
    static const int OP_WB         = (MAX_CHAR+13);

    
};

#ifdef _DEBUG
const TRexChar* g_nnames[] =
{
    "NONE", "OP_GREEDY", "OP_OR",
    "OP_EXPR", "OP_NOCAPEXPR", "OP_DOT", "OP_CASS",
    "OP_CCASS", "OP_NCASS", "OP_RANGE", "OP_CHAR",
    "OP_EO", "OP_BO", "OP_WB"
};

#define debug_print printf

#endif

#define TREX_SYMBOL_ANY_CHAR ('.')
#define TREX_SYMBOL_GREEDY_ONE_OR_MORE ('+')
#define TREX_SYMBOL_GREEDY_ZERO_OR_MORE ('*')
#define TREX_SYMBOL_GREEDY_ZERO_OR_ONE ('?')
#define TREX_SYMBOL_BRANCH ('|')
#define TREX_SYMBOL_END_OF_STRING ('$')
#define TREX_SYMBOL_BEGINNING_OF_STRING ('^')
#define TREX_SYMBOL_ESCAPE_CHAR ('\\')




template< typename T_Char >
int trex_list(typename TRexTraits< T_Char >::TRex *exp);

template< typename T_Char >
void trex_error(typename TRexTraits< T_Char >::TRex *exp, const TRexChar *error)
{
    if (exp->_error) *exp->_error = error;
    longjmp(*((jmp_buf*)exp->_jmpbuf), -1);
}

template< typename T_Char >
int trex_newnode(typename TRexTraits< T_Char >::TRex *exp, TRexNodeType type)
{
    TRexNode n;
    int newid;
    n.type = type;
    n.next = n.right = n.left = -1;
    if (type == TRexTraits< T_Char >::OP_EXPR)
        n.right = exp->_nsubexpr++;
    if(exp->_nallocated < (exp->_nsize + 1)) {
        int oldsize = exp->_nallocated;
        exp->_nallocated *= 2;
        //TOMW - Fix realloc leak...
        TRexNode* pOld = exp->_nodes;
        exp->_nodes = (TRexNode *)realloc(exp->_nodes, exp->_nallocated * sizeof(TRexNode));
        if (exp->_nodes == NULL)
        {
            exp->_nodes = pOld;
            trex_error<T_Char>(exp, "memory allocation faiure");
        }
        //--

    }
    exp->_nodes[exp->_nsize++] = n;
    newid = exp->_nsize - 1;
    return (int)newid;
}

template< typename T_Char >
void trex_expect(typename TRexTraits< T_Char >::TRex *exp, int n){
    if((*exp->_p) != n)
        trex_error<T_Char>(exp, "expected paren");
    exp->_p++;
}

template< typename T_Char >
int trex_escapechar(typename TRexTraits< T_Char >::TRex *exp)
{
    if (*exp->_p == TREX_SYMBOL_ESCAPE_CHAR){
        exp->_p++;
        switch (*exp->_p) {
        case 'v': exp->_p++; return '\v';
        case 'n': exp->_p++; return '\n';
        case 't': exp->_p++; return '\t';
        case 'r': exp->_p++; return '\r';
        case 'f': exp->_p++; return '\f';
        default: return (*exp->_p++);
        }
    }
    else if (!TRexTraits< TRexWChar >::scisprint(*exp->_p)) trex_error<T_Char>(exp, "letter expected");
    return (*exp->_p++);
}

template< typename T_Char >
int trex_charclass(typename TRexTraits< T_Char >::TRex *exp, int classid)
{
    int n = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_CCLASS);
    exp->_nodes[n].left = classid;
    return n;
}

template< typename T_Char >
int trex_charnode(typename TRexTraits< T_Char >::TRex *exp, TRexBool isclass)
{
    T_Char t;
    if(*exp->_p == TREX_SYMBOL_ESCAPE_CHAR) {
        exp->_p++;
        switch(*exp->_p) {
            case 'n': exp->_p++; return trex_newnode<T_Char>(exp,'\n');
            case 't': exp->_p++; return trex_newnode<T_Char>(exp,'\t');
            case 'r': exp->_p++; return trex_newnode<T_Char>(exp,'\r');
            case 'f': exp->_p++; return trex_newnode<T_Char>(exp,'\f');
            case 'v': exp->_p++; return trex_newnode<T_Char>(exp,'\v');
            case 'a': case 'A': case 'w': case 'W': case 's': case 'S':
            case 'd': case 'D': case 'x': case 'X': case 'c': case 'C':
            case 'p': case 'P': case 'l': case 'u':
                {
                t = *exp->_p; exp->_p++;
                return trex_charclass<T_Char>(exp, t);
                }
            case 'b':
            case 'B':
                if(!isclass) {
                    int node = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_WB);
                    exp->_nodes[node].left = *exp->_p;
                    exp->_p++;
                    return node;
                } //else default
            default:
                t = *exp->_p; exp->_p++;
                return trex_newnode<T_Char>(exp, t);
        }
    }
    else if (!TRexTraits< T_Char >::scisprint(*exp->_p)) {

        trex_error<T_Char>(exp, "letter expected");
    }
    t = *exp->_p; exp->_p++;
    return trex_newnode<T_Char>(exp, t);
}

template< typename T_Char >
int trex_class(typename TRexTraits< T_Char >::TRex *exp)
{
    int ret = -1;
    int first = -1,chain;
    if(*exp->_p == TREX_SYMBOL_BEGINNING_OF_STRING){
        ret = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_NCLASS);
        exp->_p++;
    }
    else ret = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_CLASS);

    if (*exp->_p == ']') trex_error<T_Char>(exp, "empty class");
    chain = ret;
    while(*exp->_p != ']' && exp->_p != exp->_eol) {
        if(*exp->_p == '-' && first != -1){
            int r,t;
            if (*exp->_p++ == ']') trex_error<T_Char>(exp, "unfinished range");
            r = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_RANGE);
            if (first>*exp->_p) trex_error<T_Char>(exp, "invalid range");
            if (exp->_nodes[first].type == TRexTraits< T_Char >::OP_CCLASS) trex_error<T_Char>(exp, "cannot use character classes in ranges");
            exp->_nodes[r].left = exp->_nodes[first].type;
            t = trex_escapechar<T_Char>(exp);
            exp->_nodes[r].right = t;
            exp->_nodes[chain].next = r;
            chain = r;
            first = -1;
        }
        else{
            if(first!=-1){
                int c = first;
                exp->_nodes[chain].next = c;
                chain = c;
                first = trex_charnode<T_Char>(exp, TRex_True);
            }
            else{
                first = trex_charnode<T_Char>(exp, TRex_True);
            }
        }
    }
    if(first!=-1){
        int c = first;
        exp->_nodes[chain].next = c;
        chain = c;
        first = -1;
    }
    /* hack? */
    exp->_nodes[ret].left = exp->_nodes[ret].next;
    exp->_nodes[ret].next = -1;
    return ret;
}

template< typename T_Char >
int trex_parsenumber(typename TRexTraits< T_Char >::TRex *exp)
{
    int ret = *exp->_p-'0';
    int positions = 10;
    exp->_p++;
    while(isdigit(*exp->_p)) {
        ret = ret*10+(*exp->_p++-'0');
        if (positions == 1000000000) trex_error<T_Char>(exp, "overflow in numeric constant");
        positions *= 10;
    };
    return ret;
}

template< typename T_Char >
int trex_element(typename TRexTraits< T_Char >::TRex *exp)
{
    int ret = -1;
    switch(*exp->_p)
    {
    case '(': {
        int expr,newn;
        exp->_p++;


        if(*exp->_p =='?') {
            exp->_p++;
            trex_expect<T_Char>(exp,':');
            expr = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_NOCAPEXPR);
        }
        else
            expr = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_EXPR);
        newn = trex_list<T_Char>(exp);
        exp->_nodes[expr].left = newn;
        ret = expr;
        trex_expect<T_Char>(exp, ')');
              }
              break;
    case '[':
        exp->_p++;
        ret = trex_class<T_Char>(exp);
        trex_expect<T_Char>(exp, ']');
        break;
    case TREX_SYMBOL_END_OF_STRING: exp->_p++; ret = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_EOL); break;
    case TREX_SYMBOL_ANY_CHAR: exp->_p++; ret = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_DOT); break;
    default:
        ret = trex_charnode<T_Char>(exp, TRex_False);
        break;
    }

    {
        int op;
        TRexBool isgreedy = TRex_False;
        unsigned short p0 = 0, p1 = 0;
        switch(*exp->_p){
            case TREX_SYMBOL_GREEDY_ZERO_OR_MORE: p0 = 0; p1 = 0xFFFF; exp->_p++; isgreedy = TRex_True; break;
            case TREX_SYMBOL_GREEDY_ONE_OR_MORE: p0 = 1; p1 = 0xFFFF; exp->_p++; isgreedy = TRex_True; break;
            case TREX_SYMBOL_GREEDY_ZERO_OR_ONE: p0 = 0; p1 = 1; exp->_p++; isgreedy = TRex_True; break;
            case '{':
                exp->_p++;
                if (!isdigit(*exp->_p)) trex_error<T_Char>(exp, "number expected");
                p0 = (unsigned short)trex_parsenumber<T_Char>(exp);
                /*******************************/
                switch(*exp->_p) {
            case '}':
                p1 = p0; exp->_p++;
                break;
            case ',':
                exp->_p++;
                p1 = 0xFFFF;
                if(isdigit(*exp->_p)){
                    p1 = (unsigned short)trex_parsenumber<T_Char>(exp);
                }
                trex_expect<T_Char>(exp, '}');
                break;
            default:
                trex_error<T_Char>(exp, ", or } expected");
        }
        /*******************************/
        isgreedy = TRex_True;
        break;

        }
        if(isgreedy) {
            int nnode = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_GREEDY);
            op = TRexTraits< T_Char >::OP_GREEDY;
            exp->_nodes[nnode].left = ret;
            exp->_nodes[nnode].right = ((p0)<<16)|p1;
            ret = nnode;
        }
    }
    if((*exp->_p != TREX_SYMBOL_BRANCH) && (*exp->_p != ')') && (*exp->_p != TREX_SYMBOL_GREEDY_ZERO_OR_MORE) && (*exp->_p != TREX_SYMBOL_GREEDY_ONE_OR_MORE) && (*exp->_p != '\0')) {
        int nnode = trex_element<T_Char>(exp);
        exp->_nodes[ret].next = nnode;
    }

    return ret;
}

template< typename T_Char >
int trex_list(typename TRexTraits< T_Char >::TRex *exp)
{
    int ret=-1,e;
    if(*exp->_p == TREX_SYMBOL_BEGINNING_OF_STRING) {
        exp->_p++;
        ret = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_BOL);
    }
    e = trex_element<T_Char>(exp);
    if(ret != -1) {
        exp->_nodes[ret].next = e;
    }
    else ret = e;

    if(*exp->_p == TREX_SYMBOL_BRANCH) {
        int temp,tright;
        exp->_p++;
        temp = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_OR);
        exp->_nodes[temp].left = ret;
        tright = trex_list<T_Char>(exp);
        exp->_nodes[temp].right = tright;
        ret = temp;
    }
    return ret;
}

template< typename T_Char >
TRexBool trex_matchcclass(int cclass, T_Char c)
{
    switch(cclass) {
    case 'a': return isalpha(c)?TRex_True:TRex_False;
    case 'A': return !isalpha(c)?TRex_True:TRex_False;
    case 'w': return (isalnum(c) || c == '_')?TRex_True:TRex_False;
    case 'W': return (!isalnum(c) && c != '_')?TRex_True:TRex_False;
    case 's': return isspace(c)?TRex_True:TRex_False;
    case 'S': return !isspace(c)?TRex_True:TRex_False;
    case 'd': return isdigit(c)?TRex_True:TRex_False;
    case 'D': return !isdigit(c)?TRex_True:TRex_False;
    case 'x': return isxdigit(c)?TRex_True:TRex_False;
    case 'X': return !isxdigit(c)?TRex_True:TRex_False;
    case 'c': return iscntrl(c)?TRex_True:TRex_False;
    case 'C': return !iscntrl(c)?TRex_True:TRex_False;
    case 'p': return ispunct(c)?TRex_True:TRex_False;
    case 'P': return !ispunct(c)?TRex_True:TRex_False;
    case 'l': return islower(c)?TRex_True:TRex_False;
    case 'u': return isupper(c)?TRex_True:TRex_False;
    }
    return TRex_False; /*cannot happen*/
}
template< typename T_Char >
TRexBool trex_matchclass(typename TRexTraits< T_Char >::TRex* exp, TRexNode *node, T_Char c)
{
    //TOMW - fix possible NULL deref
    if (exp && exp->_nodes)
    {
        do {
            switch (node->type) {
            case TRexTraits< T_Char >::OP_RANGE:
                if (c >= node->left && c <= node->right) return TRex_True;
                break;
            case TRexTraits< T_Char >::OP_CCLASS:
                if (trex_matchcclass<T_Char>(node->left, c)) return TRex_True;
                break;
            default:
                if (c == node->type)return TRex_True;
            }
        } while ((node->next != -1) && exp->_nodes && (node = &exp->_nodes[node->next]));
    }
    return TRex_False;
}
template< typename T_Char >
const T_Char *trex_matchnode(typename TRexTraits< T_Char >::TRex* exp, TRexNode *node, const T_Char *str, TRexNode *next)
{

    TRexNodeType type = node->type;
    switch(type) {
    case TRexTraits< T_Char >::OP_GREEDY: {
        //TRexNode *greedystop = (node->next != -1) ? &exp->_nodes[node->next] : NULL;
        TRexNode *greedystop = NULL;
        int p0 = (node->right >> 16)&0x0000FFFF, p1 = node->right&0x0000FFFF, nmaches = 0;
        const T_Char *s = str, *good = str;

        if(node->next != -1) {
            greedystop = &exp->_nodes[node->next];
        }
        else {
            greedystop = next;
        }

        while((nmaches == 0xFFFF || nmaches < p1)) {

            const T_Char *stop;
            if (!(s = trex_matchnode<T_Char>(exp, &exp->_nodes[node->left], s, greedystop)))
                break;
            nmaches++;
            good=s;
            if(greedystop) {
                //checks that 0 matches satisfy the expression(if so skips)
                //if not would always stop(for instance if is a '?')
                if (greedystop->type != TRexTraits< T_Char >::OP_GREEDY ||
                    (greedystop->type == TRexTraits< T_Char >::OP_GREEDY && ((greedystop->right >> 16) & 0x0000FFFF) != 0))
                {
                    TRexNode *gnext = NULL;
                    if(greedystop->next != -1) {
                        gnext = &exp->_nodes[greedystop->next];
                    }else if(next && next->next != -1){
                        gnext = &exp->_nodes[next->next];
                    }
                    stop = trex_matchnode<T_Char>(exp, greedystop, s, gnext);
                    if(stop) {
                        //if satisfied stop it
                        if(p0 == p1 && p0 == nmaches) break;
                        else if(nmaches >= p0 && p1 == 0xFFFF) break;
                        else if(nmaches >= p0 && nmaches <= p1) break;
                    }
                }
            }

            if(s >= exp->_eol)
                break;
        }
        if(p0 == p1 && p0 == nmaches) return good;
        else if(nmaches >= p0 && p1 == 0xFFFF) return good;
        else if(nmaches >= p0 && nmaches <= p1) return good;
        return NULL;
    }
    case TRexTraits< T_Char >::OP_OR: {
            const T_Char *asd = str;
            TRexNode *temp=&exp->_nodes[node->left];
            while ((asd = trex_matchnode<T_Char>(exp, temp, asd, NULL))) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            asd = str;
            temp = &exp->_nodes[node->right];
            while ((asd = trex_matchnode<T_Char>(exp, temp, asd, NULL))) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            return NULL;
            break;
    }
    case TRexTraits< T_Char >::OP_EXPR:
    case TRexTraits< T_Char >::OP_NOCAPEXPR:{
            TRexNode *n = &exp->_nodes[node->left];
            const T_Char *cur = str;
            int capture = -1;
            if (node->type != TRexTraits< T_Char >::OP_NOCAPEXPR && node->right == exp->_currsubexp) {
                capture = exp->_currsubexp;
                exp->_matches[capture].begin = cur;
                exp->_currsubexp++;
            }

            do {
                TRexNode *subnext = NULL;
                if(n->next != -1) {
                    subnext = &exp->_nodes[n->next];
                }else {
                    subnext = next;
                }
                if (!(cur = trex_matchnode<T_Char>(exp, n, cur, subnext))) {
                    if(capture != -1){
                        exp->_matches[capture].begin = 0;
                        exp->_matches[capture].len = 0;
                    }
                    return NULL;
                }
            } while((n->next != -1) && (n = &exp->_nodes[n->next]));

            if(capture != -1)
                exp->_matches[capture].len = cur - exp->_matches[capture].begin;
            return cur;
    }
    case TRexTraits< T_Char >::OP_WB:
        if(str == exp->_bol && !isspace(*str)
         || (str == exp->_eol && !isspace(*(str-1)))
         || (!isspace(*str) && isspace(*(str+1)))
         || (isspace(*str) && !isspace(*(str+1))) ) {
            return (node->left == 'b')?str:NULL;
        }
        return (node->left == 'b')?NULL:str;
    case TRexTraits< T_Char >::OP_BOL:
        if(str == exp->_bol) return str;
        return NULL;
    case TRexTraits< T_Char >::OP_EOL:
        if(str == exp->_eol) return str;
        return NULL;
    case TRexTraits< T_Char >::OP_DOT:
        {
            str++;
        }
        return str;
    case TRexTraits< T_Char >::OP_NCLASS:
    case TRexTraits< T_Char >::OP_CLASS:
        if (trex_matchclass<T_Char>(exp, &exp->_nodes[node->left], *str) ? 
                                   (type == TRexTraits< T_Char >::OP_CLASS ? TRex_True : TRex_False) : (type == TRexTraits< T_Char >::OP_NCLASS ? TRex_True : TRex_False))
        {
            str++;
            return str;
        }
        return NULL;
    case TRexTraits< T_Char >::OP_CCLASS:
        if (trex_matchcclass<T_Char>(node->left, *str)) {
            str++;
            return str;
        }
        return NULL;
    default: /* char */
        if(*str != node->type) return NULL;
        str++;
        return str;
    }
    return NULL;
}

/* public api */
template< typename T_Char >
typename TRexTraits< T_Char >::TRex *trex_compile_t(const T_Char *pattern, const TRexChar **error)
{
    typedef TRexTraits< T_Char >::TRex TRex;

    TRex *exp = (TRex*)malloc(sizeof(TRex));

    //TOMW fix NULL deref
    if (exp)
    {
        exp->_eol = exp->_bol = NULL;
        exp->_p = pattern;
        //TOMW const cast :(
        exp->_nallocated = (int)TRexTraits< T_Char >::scstrlen(const_cast<T_Char*>(pattern)) * sizeof(T_Char);
        exp->_nodes = (TRexNode *)malloc(exp->_nallocated * sizeof(TRexNode));
        exp->_nsize = 0;
        exp->_matches = 0;
        exp->_nsubexpr = 0;
        exp->_first = trex_newnode<T_Char>(exp, TRexTraits< T_Char >::OP_EXPR);
        exp->_error = error;
        exp->_jmpbuf = malloc(sizeof(jmp_buf));

        //TOMW - fix NULL deref

        if (exp->_jmpbuf)
        {
            if (setjmp(*((jmp_buf*)exp->_jmpbuf)) == 0) {
                int res = trex_list<T_Char>(exp);

                if (exp->_nodes)
                {

                    exp->_nodes[exp->_first].left = res;
                    if (*exp->_p != '\0')
                        trex_error<T_Char>(exp, "unexpected character");
#ifdef _DEBUG
                    {
                        int nsize, i;
                        TRexNode *t;
                        nsize = exp->_nsize;
                        t = &exp->_nodes[0];
                        debug_print(("\n"));
                        for (i = 0; i < nsize; i++) {
                            if (exp->_nodes[i].type>TRexTraits< T_Char >::MAX_CHAR)
                                debug_print(("[%02d] %10s "), i, g_nnames[exp->_nodes[i].type - TRexTraits< T_Char >::MAX_CHAR]);
                            else
                                debug_print(("[%02d] %10c "), i, exp->_nodes[i].type);
                            debug_print(("left %02d right %02d next %02d\n"), exp->_nodes[i].left, exp->_nodes[i].right, exp->_nodes[i].next);
                        }
                        debug_print(("\n"));
                    }
#endif
                    exp->_matches = (TRexTraits< T_Char >::TRexMatchType*)malloc(exp->_nsubexpr * sizeof(TRexTraits< T_Char >::TRexMatchType));
                    memset(exp->_matches, 0, exp->_nsubexpr * sizeof(TRexTraits< T_Char >::TRexMatchType));
                }
                else
                {
                    goto error;
                }
            }
            else
            {
                goto error;
            }
        }
        else
        {
            goto error;
        }

        return exp;
    }

    error:
        trex_free_t<T_Char>(exp);

    return NULL;
}

template< typename T_Char >
void trex_free_t(typename TRexTraits< T_Char >::TRex *exp)
{
    if(exp)    {
        if(exp->_nodes) free(exp->_nodes);
        if(exp->_jmpbuf) free(exp->_jmpbuf);
        if(exp->_matches) free(exp->_matches);
        free(exp);
    }
}

template< typename T_Char>
TRexBool trex_match_t(typename TRexTraits< T_Char >::TRex* exp, const T_Char* text)
{
    const T_Char* res = NULL;
    exp->_bol = text;
    //TOMW - Const cast
    exp->_eol = text + TRexTraits< T_Char >::scstrlen(const_cast<T_Char*>(text));
    exp->_currsubexp = 0;
    res = trex_matchnode<T_Char>(exp,exp->_nodes,text,NULL);

    #ifdef _DEBUG
        debug_print("DEBUG trex_match: res = '%s'\n", res);
        debug_print("DEBUG trex_match: exp->_eol = '%s'\n", exp->_eol);
    #endif

    // Fail match if trex_matchnode returns nothing
    if (!res) {
        return TRex_False;
    }

    return TRex_True;
}

template< typename T_Char>
TRexBool trex_searchrange_t(typename TRexTraits< T_Char >::TRex* exp, const T_Char* text_begin, const T_Char* text_end, const T_Char** out_begin, const T_Char** out_end)
{
    const T_Char *cur = NULL;
    int node = exp->_first;
    if(text_begin >= text_end) return TRex_False;
    exp->_bol = text_begin;
    exp->_eol = text_end;
    do {
        cur = text_begin;
        while(node != -1) {
            exp->_currsubexp = 0;
            cur = trex_matchnode(exp,&exp->_nodes[node],cur,NULL);
            if(!cur)
                break;
            node = exp->_nodes[node].next;
        }
        text_begin++;
    } while(cur == NULL && text_begin != text_end);

    if(cur == NULL)
        return TRex_False;

    --text_begin;

    if(out_begin) *out_begin = text_begin;
    if(out_end) *out_end = cur;
    return TRex_True;
}

template< typename T_Char >
TRexBool trex_search_t(typename TRexTraits< T_Char >::TRex* exp, const T_Char* text, const T_Char** out_begin, const T_Char** out_end)
{
    //TOMW - const cast
    return trex_searchrange_t<T_Char>(exp, text, text + TRexTraits< T_Char >::scstrlen(const_cast<T_Char*>(text)), out_begin, out_end);
}

template< typename T_Char >
int trex_getsubexpcount_t(typename TRexTraits< T_Char >::TRex* exp)
{
    return exp->_nsubexpr;
}

template< typename T_Char >
TRexBool trex_getsubexp_t(typename TRexTraits< T_Char >::TRex* exp, int n, typename TRexTraits< T_Char >::TRexMatchType *subexp)
{
    if( n<0 || n >= exp->_nsubexpr) return TRex_False;
    *subexp = exp->_matches[n];
    return TRex_True;
}

//
//
//
//

typedef TRexTraits< TRexChar >::TRex TRexASCII;

extern "C"
{


    TREX_API TRex *trex_compile(const TRexChar *pattern, const TRexChar **error)
    {
        return reinterpret_cast<TRex*>(trex_compile_t<TRexChar>(pattern, error));
    }

    TREX_API void trex_free(TRex *exp)
    {
        return trex_free_t<TRexChar>(reinterpret_cast<TRexASCII*>(exp));
    }

    TREX_API TRexBool trex_match(TRex* exp, const TRexChar* text)
    {
        return trex_match_t<TRexChar>(reinterpret_cast<TRexASCII*>(exp), text);
    }

    TREX_API TRexBool trex_search(TRex* exp, const TRexChar* text, const TRexChar** out_begin, const TRexChar** out_end)
    {
        return trex_search_t<TRexChar>(reinterpret_cast<TRexASCII*>(exp), text, out_begin, out_end);
    }

    TREX_API TRexBool trex_searchrange(TRex* exp, const TRexChar* text_begin, const TRexChar* text_end, const TRexChar** out_begin, const TRexChar** out_end)
    {
        return trex_searchrange_t<TRexChar>(reinterpret_cast<TRexASCII*>(exp), text_begin, text_end, out_begin, out_end);
    }

    TREX_API int trex_getsubexpcount(TRex* exp)
    {
        return trex_getsubexpcount_t<TRexChar>(reinterpret_cast<TRexASCII*>(exp));
    }

    TREX_API TRexBool trex_getsubexp(TRex* exp, int n, TRexMatch *subexp)
    {
        return trex_getsubexp_t<TRexChar>(reinterpret_cast<TRexASCII*>(exp), n, subexp);
    }

}

//
//
//
//
typedef TRexTraits< TRexWChar >::TRex TRexUnicode;

extern "C"
{

    TREX_API TRex *trex_compilew(const TRexWChar *pattern, const TRexChar **error)
    {
        return reinterpret_cast<TRex*>(trex_compile_t<TRexWChar>(pattern, error));
    }

    TREX_API void trex_freew(TRex *exp)
    {
        return trex_free_t<TRexWChar>(reinterpret_cast<TRexUnicode*>(exp));
    }

    TREX_API TRexBool trex_matchw(TRex* exp, const TRexWChar* text)
    {
        return trex_match_t<TRexWChar>(reinterpret_cast<TRexUnicode*>(exp), text);
    }

    TREX_API TRexBool trex_searchw(TRex* exp, const TRexWChar* text, const TRexWChar** out_begin, const TRexWChar** out_end)
    {
        return trex_search_t<TRexWChar>(reinterpret_cast<TRexUnicode*>(exp), text, out_begin, out_end);
    }

    TREX_API TRexBool trex_searchrangew(TRex* exp, const TRexWChar* text_begin, const TRexWChar* text_end, const TRexWChar** out_begin, const TRexWChar** out_end)
    {
        return trex_searchrange_t<TRexWChar>(reinterpret_cast<TRexUnicode*>(exp), text_begin, text_end, out_begin, out_end);
    }

    TREX_API int trex_getsubexpcountw(TRex* exp)
    {
        return trex_getsubexpcount_t<TRexWChar>(reinterpret_cast<TRexUnicode*>(exp));
    }

    TREX_API TRexBool trex_getsubexpw(TRex* exp, int n, TRexWMatch *subexp)
    {
        return trex_getsubexp_t<TRexWChar>(reinterpret_cast<TRexUnicode*>(exp), n, subexp);
    }

}