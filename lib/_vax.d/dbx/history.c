#ifndef lint
static char *sccsid = "@(#)history.c	4.1 11/9/90";
#endif

#include "defs.h"
#include "lists.h"
#include "history.h"
#include "tree.h"
#include "eval.h"

#define foreach_rev(type, i, list) \
{ \
    register ListItem _item; \
 \
    _item = list_tail(list); \
    while (_item != nil) { \
	i = list_element(type, _item); \
	_item = list_prev(_item);


/*
 *  Input_init - initialize history list
 *  Iline_alloc - add a line to history list
 *  del_Iline - delete line from history list
 *  get_Iline  - get a line from history list
 */


#ifndef public
typedef char *Iline;
#endif

public Integer historyevent;		/* number of current history event */


private List Input_lines;		/* record or playback list */

public Iline Iline_alloc(str)
char *str;
{
    ListItem li;
    integer maxl = 22, cnt;
    Node s;

    s = (Node)findvar(identname("$historywindow", true));
    if (s != nil) {
		eval(s);
		maxl = pop(integer);
    }
	li = (ListItem) malloc(sizeof(struct ListItem));
	li->element = strdup(str);
	list_insert(li, list_head(Input_lines), Input_lines);
	historyevent++;
	while(list_size(Input_lines) > maxl)
		list_delete(list_tail(Input_lines), Input_lines);
    return;
}

public Iline get_Iline(hid)
int hid;
{
    Iline found;
    Iline r;

    found = NULL;
    foreach (Iline, r, Input_lines)
	if (hid-- == 0) {
	    found = r;
	    break;
	}
    endfor;
    return found;
}

public Iline srch_Iline(str)
char *str;
{
    Iline found;
    Iline r;
    int len;

    found = NULL;
    len = strlen(str);
    foreach (Iline, r, Input_lines)
	if (strncmp(str, r, len) == 0) {
	    found = r;
	    break;
	}
    endfor;
    return found;
}

public boolean del_Iline(id)
{
    boolean found;
    Iline r;

    found = false;
    foreach (Iline, r, Input_lines)
	if (id-- == 0) {
	    found = true;
	    list_delete(list_curitem(Input_lines), Input_lines);
	    break;
	}
    endfor;
    return found;
}

public dump_Iline()
{
    Iline r;
    int i;

    i = list_size(Input_lines);
    foreach_rev (Iline, r, Input_lines)
	    fprintf(stderr, "%3d\t%s", historyevent + 1 - i--, r);
    endfor;
}

Input_init()
{
	Input_lines = list_alloc();
}
