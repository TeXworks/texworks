/* 
Copyright (c) 2008 jerome DOT laurens AT u-bourgogne DOT fr

This file is part of the SyncTeX package.

License:
--------
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE

Except as contained in this notice, the name of the copyright holder  
shall not be used in advertising or otherwise to promote the sale,  
use or other dealings in this Software without prior written  
authorization from the copyright holder.

*/

#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "limits.h"
#include "time.h"
#include "math.h"
#include "errno.h"

/*  This custom malloc functions initializes to 0 the newly allocated memory. */
void *_synctex_malloc(size_t size) {
	void * ptr = malloc(size);
	if(ptr) {
		memset(ptr,0,size);
	}
	return (void *)ptr;
}

/*  The data is organized in a graph with multiple entries.
 *  The root object is a scanner, it is created with the contents on a synctex file.
 *  Each leaf of the tree is a synctex_node_t object.
 *  There are 3 subtrees, two of them sharing the same leaves.
 *  The first tree is the list of input records, where input file names are associated with tags.
 *  The second tree is the box tree as given by TeX when shipping pages out.
 *  First level objects are sheets, containing boxes, glues, kerns...
 *  The third tree allows to browse leaves according to tag and line.
 */

#include "synctex_parser.h"

/* each synctex node has a class */
typedef struct __synctex_class_t _synctex_class_t;
typedef _synctex_class_t * synctex_class_t;


/*  synctex_node_t is a pointer to a node
 *  _synctex_node is the target of the synctex_node_t pointer
 *  It is a pseudo object oriented program.
 *  class is a pointer to the class object the node belongs to.
 *  implementation is meant to contain the private data of the node
 *  basically, there are 2 kinds of information: navigation information and
 *  synctex information. Both will depend on the type of the node,
 *  thus different nodes will have different private data.
 *  There is no inheritancy overhead.
 */
struct _synctex_node {
	synctex_class_t class;
	int * implementation;
};

/*  Each node of the tree, except the scanner itself belongs to a class.
 *  The class object is just a struct declaring the owning scanner
 *  This is a pointer to the scanner as root of the tree.
 *  The type is used to identify the kind of node.
 *  The class declares pointers to a creator and a destructor method.
 *  The log and display fields are used to log and display the node.
 *  display will also display the child, sibling and parent sibling.
 *  parent, child and sibling are used to navigate the tree,
 *  from TeX box hierarchy point of view.
 *  The friend field points to a method which allows to navigate from friend to friend.
 *  A friend is a node with very close tag and line numbers.
 *  Finally, the info field point to a method giving the private node info offset.
 */
typedef synctex_node_t *(*_synctex_node_getter_t)(synctex_node_t);
typedef int *(*_synctex_int_getter_t)(synctex_node_t);

struct __synctex_class_t {
	synctex_scanner_t scanner;
	int type;
	synctex_node_t (*new)(synctex_scanner_t scanner);
	void (*free)(synctex_node_t);
	void (*log)(synctex_node_t);
	void (*display)(synctex_node_t);
	_synctex_node_getter_t parent;
	_synctex_node_getter_t child;
	_synctex_node_getter_t sibling;
	_synctex_node_getter_t friend;
	_synctex_int_getter_t info;
};

#   pragma mark -
#   pragma mark Abstract OBJECTS and METHODS

/*  These macros are shortcuts
 *  INFO(node) points to the first synctex integer data of node
 *  INFO(node)[index] is the information at index
 */
#   define INFO(NODE) ((*((((NODE)->class))->info))(NODE))

/*  This macro checks if a message can be sent.
 */
#   define CAN_PERFORM(NODE,SELECTOR)\
		(NULL!=((((NODE)->class))->SELECTOR))

/*  This macro is some kind of objc_msg_send.
 *  It takes care of sending the proper message if possible.
 */
#   define MSG_SEND(NODE,SELECTOR) if(NODE && CAN_PERFORM(NODE,SELECTOR)) {\
		(*((((NODE)->class))->SELECTOR))(NODE);\
	}

/*  read only safe getter
 */
#   define GET(NODE,SELECTOR)((NODE && CAN_PERFORM(NODE,SELECTOR))?GETTER(NODE,SELECTOR)[0]:(NULL))

/*  read/write getter
 */
#   define GETTER(NODE,SELECTOR)\
		((synctex_node_t *)((*((((NODE)->class))->SELECTOR))(NODE)))

#   define FREE(NODE) MSG_SEND(NODE,free);

/*  Parent getter and setter
 */
#   define PARENT(NODE) GET(NODE,parent)
#   define SET_PARENT(NODE,NEW_PARENT) if(NODE && NEW_PARENT && CAN_PERFORM(NODE,parent)){\
		GETTER(NODE,parent)[0]=NEW_PARENT;\
	}

/*  Child getter and setter
 */
#   define CHILD(NODE) GET(NODE,child)
#   define SET_CHILD(NODE,NEW_CHILD) if(NODE && NEW_CHILD){\
		GETTER(NODE,child)[0]=NEW_CHILD;\
		GETTER(NEW_CHILD,parent)[0]=NODE;\
	}

/*  Sibling getter and setter
 */
#   define SIBLING(NODE) GET(NODE,sibling)
#   define SET_SIBLING(NODE,NEW_SIBLING) if(NODE && NEW_SIBLING) {\
		GETTER(NODE,sibling)[0]=NEW_SIBLING;\
		if(CAN_PERFORM(NEW_SIBLING,parent) && CAN_PERFORM(NODE,parent)) {\
			GETTER(NEW_SIBLING,parent)[0]=GETTER(NODE,parent)[0];\
		}\
	}
/*  Friend getter and setter
 */
#   define FRIEND(NODE) GET(NODE,friend)
#   define SET_FRIEND(NODE,NEW_FRIEND) if(NODE && NEW_FRIEND){\
		GETTER(NODE,friend)[0]=NEW_FRIEND;\
	}

/*  A node is meant to own its child and sibling.
 *  It is not owned by its parent, unless it is its first child.
 *  This destructor is for all nodes.
 */
void _synctex_free_node(synctex_node_t node) {
	if(node) {
		(*((node->class)->sibling))(node);
		FREE(SIBLING(node));
		FREE(CHILD(node));
		free(node);
	}
	return;
}

/*  A node is meant to own its child and sibling.
 *  It is not owned by its parent, unless it is its first child.
 *  This destructor is for nodes with no child.
 */
void _synctex_free_leaf(synctex_node_t node) {
	if(node) {
		FREE(SIBLING(node));
		free(node);
	}
	return;
}

/*  The synctex scanner is the root object.
 *  Is is initiated with the contents of a text file.
 *  The buffer_? are first used to parse the text.
 */
struct __synctex_scanner_t {
	unsigned char * buffer_ptr;   /* current location in the buffer */
	unsigned char * buffer_start; /* start of the buffer */
	unsigned char * buffer_end;   /* end of the buffer */
	char * output;                /* dvi or pdf, not yet used */
	int version;                  /* 1, not yet used */
	int pre_magnification;        /* magnification from the synctex preamble */
	int pre_unit;                 /* unit from the synctex preamble */
	int pre_x_offset;             /* X offste from the synctex preamble */
	int pre_y_offset;             /* Y offset from the synctex preamble */
	int count;                    /* Number of records, from the synctex postamble */
	float unit;                   /* real unit, from synctex preamble or post scriptum */
	float x_offset;               /* X offset, from synctex preamble or post scriptum */
	float y_offset;               /* Y Offset, from synctex preamble or post scriptum */
	synctex_node_t sheet;             /* The first sheet node */
	synctex_node_t input;             /* The first input node */
	int number_of_lists;          /* The number of friend lists */
	synctex_node_t * lists_of_friends;/* The friend lists */
	_synctex_class_t class[synctex_node_type_last]; /* The classes of the nodes of the scanner */
};

/*  PTR, START and END are convenient shortcuts
 */
#   define PTR (scanner->buffer_ptr)
#   define START (scanner->buffer_start)
#   define END (scanner->buffer_end)

#   pragma mark -
#   pragma mark OBJECTS, their creators and destructors.

/*  The sheet is a first level node.
 *  It has no parent (the parent is the scanner itself)
 *  Its sibling points to another sheet.
 *  Its child points to its first child, in general a box.
 *  A sheet node contains only one synctex information: the page.
 *  This is the 1 based page index as given by TeX.
 */
typedef struct {
	synctex_class_t class;
	int implementation[2+1];/* child, sibling
	                         * PAGE */
} synctex_sheet_t;

/*  The next macros is used to access the node info
 *  for example, the page of a sheet is stored in INFO(sheet)[PAGE]
 */
#   define PAGE 0

/*  This macro defines implementation offsets
 */
#   define MAKE_GET(GETTER,OFFSET)\
synctex_node_t * GETTER (synctex_node_t node) {\
	return node?(synctex_node_t *)((&((node)->implementation))+OFFSET):NULL;\
}
MAKE_GET(_synctex_implementation_0,0)
MAKE_GET(_synctex_implementation_1,1)
MAKE_GET(_synctex_implementation_2,2)
MAKE_GET(_synctex_implementation_3,3)
MAKE_GET(_synctex_implementation_4,4)

synctex_node_t _synctex_new_sheet(synctex_scanner_t scanner);
void _synctex_display_sheet(synctex_node_t sheet);
void _synctex_log_sheet(synctex_node_t sheet);

static const _synctex_class_t synctex_class_sheet = {
	NULL,                       /* No scanner yet */
	synctex_node_type_sheet,    /* Node type */
	&_synctex_new_sheet,        /* creator */
	&_synctex_free_node,        /* destructor */
	&_synctex_log_sheet,        /* log */
	&_synctex_display_sheet,    /* display */
	NULL,                       /* No parent */
	&_synctex_implementation_0, /* child */
	&_synctex_implementation_1, /* sibling */
	NULL,                       /* No friend */
	(_synctex_int_getter_t)&_synctex_implementation_2  /* info */
};

/*  sheet node creator */
synctex_node_t _synctex_new_sheet(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_sheet_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_sheet:(synctex_class_t)&synctex_class_sheet;
	}
	return node;
}

/*  A box node contains navigation and synctex information
 *  There are different kind of boxes.
 *  Only horizontal boxes are treated differently because of their visible size.
 */
typedef struct {
	synctex_class_t class;
	int implementation[4+3+5]; /* parent,child,sibling,friend,
						        * TAG,LINE,COLUMN,
								* HORIZ,VERT,WIDTH,HEIGHT,DEPTH */
} synctex_vert_box_node_t;

#   define TAG 0
#   define LINE (TAG+1)
#   define COLUMN (LINE+1)
#   define HORIZ (COLUMN+1)
#   define VERT (HORIZ+1)
#   define WIDTH (VERT+1)
#   define HEIGHT (WIDTH+1)
#   define DEPTH (HEIGHT+1)

synctex_node_t _synctex_new_vbox(synctex_scanner_t scanner);
void _synctex_log_box(synctex_node_t sheet);
void _synctex_display_vbox(synctex_node_t node);

/*  These are static class objects, each scanner will make a copy of them and setup the scanner field.
 */
static const _synctex_class_t synctex_class_vbox = {
	NULL,                       /* No scanner yet */
	synctex_node_type_vbox,     /* Node type */
	&_synctex_new_vbox,         /* creator */
	&_synctex_free_node,        /* destructor */
	&_synctex_log_box,          /* log */
	&_synctex_display_vbox,     /* display */
	&_synctex_implementation_0, /* parent */
	&_synctex_implementation_1, /* child */
	&_synctex_implementation_2, /* sibling */
	&_synctex_implementation_3, /* friend */
	(_synctex_int_getter_t)&_synctex_implementation_4
};

/*  vertical box node creator */
synctex_node_t _synctex_new_vbox(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_vert_box_node_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_vbox:(synctex_class_t)&synctex_class_vbox;
	}
	return node;
}

/*  Horizontal boxes must contain visible size, because 0 width does not mean emptiness */
typedef struct {
	synctex_class_t class;
	int implementation[4+3+5+5]; /*parent,child,sibling,friend,
						* TAG,LINE,COLUMN,
						* HORIZ,VERT,WIDTH,HEIGHT,DEPTH,
						* HORIZ_V,VERT_V,WIDTH_V,HEIGHT_V,DEPTH_V*/
} synctex_horiz_box_node_t;

#   define HORIZ_V (DEPTH+1)
#   define VERT_V (HORIZ_V+1)
#   define WIDTH_V (VERT_V+1)
#   define HEIGHT_V (WIDTH_V+1)
#   define DEPTH_V (HEIGHT_V+1)

synctex_node_t _synctex_new_hbox(synctex_scanner_t scanner);
void _synctex_display_hbox(synctex_node_t node);
void _synctex_log_horiz_box(synctex_node_t sheet);


static const _synctex_class_t synctex_class_hbox = {
	NULL,                       /* No scanner yet */
	synctex_node_type_hbox,     /* Node type */
	&_synctex_new_hbox,         /* creator */
	&_synctex_free_node,        /* destructor */
	&_synctex_log_horiz_box,    /* log */
	&_synctex_display_hbox,     /* display */
	&_synctex_implementation_0, /* parent */
	&_synctex_implementation_1, /* child */
	&_synctex_implementation_2, /* sibling */
	&_synctex_implementation_3, /* friend */
	(_synctex_int_getter_t)&_synctex_implementation_4
};

/*  horizontal box node creator */
synctex_node_t _synctex_new_hbox(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_horiz_box_node_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_hbox:(synctex_class_t)&synctex_class_hbox;
	}
	return node;
}

/*  This void box node implementation is either horizontal or vertical
 *  It does not contain a child field.
 */
typedef struct {
	synctex_class_t class;
	int implementation[3+3+5]; /* parent,sibling,friend,
	                  * TAG,LINE,COLUMN,
					  * HORIZ,VERT,WIDTH,HEIGHT,DEPTH*/
} synctex_void_box_node_t;

synctex_node_t _synctex_new_void_vbox(synctex_scanner_t scanner);
void _synctex_log_void_box(synctex_node_t sheet);
void _synctex_display_void_vbox(synctex_node_t node);

static const _synctex_class_t synctex_class_void_vbox = {
	NULL,                       /* No scanner yet */
	synctex_node_type_void_vbox,/* Node type */
	&_synctex_new_void_vbox,    /* creator */
	&_synctex_free_node,        /* destructor */
	&_synctex_log_void_box,     /* log */
	&_synctex_display_void_vbox,/* display */
	&_synctex_implementation_0, /* parent */
	NULL,                       /* No child */
	&_synctex_implementation_1, /* sibling */
	&_synctex_implementation_2, /* friend */
	(_synctex_int_getter_t)&_synctex_implementation_3
};

/*  vertical void box node creator */
synctex_node_t _synctex_new_void_vbox(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_void_box_node_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_void_vbox:(synctex_class_t)&synctex_class_void_vbox;
	}
	return node;
}

synctex_node_t _synctex_new_void_hbox(synctex_scanner_t scanner);
void _synctex_display_void_hbox(synctex_node_t node);

static const _synctex_class_t synctex_class_void_hbox = {
	NULL,                       /* No scanner yet */
	synctex_node_type_void_hbox,/* Node type */
	&_synctex_new_void_hbox,    /* creator */
	&_synctex_free_node,        /* destructor */
	&_synctex_log_void_box,     /* log */
	&_synctex_display_void_hbox,/* display */
	&_synctex_implementation_0, /* parent */
	NULL,                       /* No child */
	&_synctex_implementation_1, /* sibling */
	&_synctex_implementation_2, /* friend */
	(_synctex_int_getter_t)&_synctex_implementation_3
};

/*  horizontal void box node creator */
synctex_node_t _synctex_new_void_hbox(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_void_box_node_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_void_hbox:(synctex_class_t)&synctex_class_void_hbox;
	}
	return node;
}

/*  The medium nodes correspond to kern, glue and math nodes.  */
typedef struct {
	synctex_class_t class;
	int implementation[3+3+3]; /* parent,sibling,friend,
	                  * TAG,LINE,COLUMN,
					  * HORIZ,VERT,WIDTH */
} synctex_medium_node_t;

synctex_node_t _synctex_new_math(synctex_scanner_t scanner);
void _synctex_log_medium_node(synctex_node_t sheet);
void _synctex_display_math(synctex_node_t node);

static const _synctex_class_t synctex_class_math = {
	NULL,                       /* No scanner yet */
	synctex_node_type_math,     /* Node type */
	&_synctex_new_math,         /* creator */
	&_synctex_free_leaf,        /* destructor */
	&_synctex_log_medium_node,  /* log */
	&_synctex_display_math,     /* display */
	&_synctex_implementation_0, /* parent */
	NULL,                       /* No child */
	&_synctex_implementation_1, /* sibling */
	&_synctex_implementation_2, /* friend */
	(_synctex_int_getter_t)&_synctex_implementation_3
};

/*  math node creator */
synctex_node_t _synctex_new_math(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_medium_node_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_math:(synctex_class_t)&synctex_class_math;
	}
	return node;
}

synctex_node_t _synctex_new_glue(synctex_scanner_t scanner);
void _synctex_display_glue(synctex_node_t node);

static const _synctex_class_t synctex_class_glue = {
	NULL,                       /* No scanner yet */
	synctex_node_type_glue,     /* Node type */
	&_synctex_new_glue,         /* creator */
	&_synctex_free_leaf,        /* destructor */
	&_synctex_log_medium_node,  /* log */
	&_synctex_display_glue,     /* display */
	&_synctex_implementation_0, /* parent */
	NULL,                       /* No child */
	&_synctex_implementation_1, /* sibling */
	&_synctex_implementation_2, /* friend */
	(_synctex_int_getter_t)&_synctex_implementation_3
};
/*  glue node creator */
synctex_node_t _synctex_new_glue(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_medium_node_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_glue:(synctex_class_t)&synctex_class_glue;
	}
	return node;
}

synctex_node_t _synctex_new_kern(synctex_scanner_t scanner);
void _synctex_display_kern(synctex_node_t node);

static const _synctex_class_t synctex_class_kern = {
	NULL,                       /* No scanner yet */
	synctex_node_type_kern,     /* Node type */
	&_synctex_new_kern,         /* creator */
	&_synctex_free_leaf,        /* destructor */
	&_synctex_log_medium_node,  /* log */
	&_synctex_display_kern,     /* display */
	&_synctex_implementation_0, /* parent */
	NULL,                       /* No child */
	&_synctex_implementation_1, /* sibling */
	&_synctex_implementation_2, /* friend */
	(_synctex_int_getter_t)&_synctex_implementation_3
};

/*  kern node creator */
synctex_node_t _synctex_new_kern(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_medium_node_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_kern:(synctex_class_t)&synctex_class_kern;
	}
	return node;
}

/*  Input nodes only know about their sibling, which is another input node.
 *  The synctex information is the TAG and NAME*/
typedef struct {
	synctex_class_t class;
	int implementation[1+2]; /* sibling,
	                          * TAG,NAME */
} synctex_input_t;

/*  The TAG previously defined is also used here */
#   define NAME 1

synctex_node_t _synctex_new_input(synctex_scanner_t scanner);
void _synctex_display_input(synctex_node_t node);
void _synctex_log_input(synctex_node_t sheet);

static const _synctex_class_t synctex_class_input = {
	NULL,                       /* No scanner yet */
	synctex_node_type_input,    /* Node type */
	&_synctex_new_input,        /* creator */
	&_synctex_free_leaf,        /* destructor */
	&_synctex_log_input,        /* log */
	&_synctex_display_input,    /* display */
	NULL,                       /* No parent */
	NULL,                       /* No child */
	&_synctex_implementation_0, /* sibling */
	NULL,                       /* No friend */
	(_synctex_int_getter_t)&_synctex_implementation_1
};

synctex_node_t _synctex_new_input(synctex_scanner_t scanner) {
	synctex_node_t node = _synctex_malloc(sizeof(synctex_input_t));
	if(node) {
		node->class = scanner?scanner->class+synctex_node_type_input:(synctex_class_t)&synctex_class_input;
	}
	return node;
}

#pragma mark -
#pragma mark Navigation
synctex_node_t synctex_node_parent(synctex_node_t node)
{
	return PARENT(node);
}
synctex_node_t synctex_node_sheet(synctex_node_t node)
{
	while(node && node->class->type != synctex_node_type_sheet) {
		node = PARENT(node);
	}
	/* exit the while loop either when node is NULL or node is a sheet */
	return node;
}
synctex_node_t synctex_node_child(synctex_node_t node)
{
	return CHILD(node);
}
synctex_node_t synctex_node_sibling(synctex_node_t node)
{
	return SIBLING(node);
}
synctex_node_t synctex_node_next(synctex_node_t node) {
	if(CHILD(node)) {
		return CHILD(node);
	}
sibling:
	if(SIBLING(node)) {
		return SIBLING(node);
	}
	if(node = PARENT(node)) {
		if(node->class->type == synctex_node_type_sheet) {// EXC_BAD_ACCESS?
			return NULL;
		}
		goto sibling;
	}
	return NULL;
}
#pragma mark -
#pragma mark CLASS

/*  Public node accessor: the type  */
synctex_node_type_t synctex_node_type(synctex_node_t node) {
	if(node) {
		return (((node)->class))->type;
	}
	return synctex_node_type_error;
}

/*  Public node accessor: the human readable type  */
const char * synctex_node_isa(synctex_node_t node) {
static const char * isa[synctex_node_type_last] =
		{"Not a node","sheet","vbox","void vbox","hbox","void hbox","kern","glue","math","input"};
	return isa[synctex_node_type(node)];
}

#   pragma mark -
#   pragma mark LOG

#   define LOG(NODE) MSG_SEND(NODE,log)

/*  Public node logger  */
void synctex_node_log(synctex_node_t node) {
	LOG(node);
}

void _synctex_log_sheet(synctex_node_t sheet) {
	if(sheet) {
		printf("%s:%i\n",synctex_node_isa(sheet),INFO(sheet)[PAGE]);
		printf("SELF:0x%x",sheet);
		printf(" PARENT:0x%x",PARENT(sheet));
		printf(" CHILD:0x%x",CHILD(sheet));
		printf(" SIBLING:0x%x",SIBLING(sheet));
		printf(" FRIEND:0x%x\n",FRIEND(sheet));
	}
}

void _synctex_log_medium_node(synctex_node_t node) {
	printf("%s:%i,%i:%i,%i:%i\n",
		synctex_node_isa(node),
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT],
		INFO(node)[WIDTH]);
	printf("SELF:0x%x",node);
	printf(" PARENT:0x%x",PARENT(node));
	printf(" CHILD:0x%x",CHILD(node));
	printf(" SIBLING:0x%x",SIBLING(node));
	printf(" FRIEND:0x%x\n",FRIEND(node));
}

void _synctex_log_void_box(synctex_node_t node) {
	int * info = INFO(node);
	printf("%s",synctex_node_isa(node));
	printf(":%i",info[TAG]);
	printf(",%i",info[LINE]);
	printf(",%i",0);
	printf(":%i",info[HORIZ]);
	printf(",%i",info[VERT]);
	printf(":%i",info[WIDTH]);
	printf(",%i",info[HEIGHT]);
	printf(",%i",info[DEPTH]);
	printf("\nSELF:0x%x",node);
	printf(" PARENT:0x%x",PARENT(node));
	printf(" CHILD:0x%x",CHILD(node));
	printf(" SIBLING:0x%x",SIBLING(node));
	printf(" FRIEND:0x%x\n",FRIEND(node));
}

void _synctex_log_box(synctex_node_t node) {
	int * info = INFO(node);
	printf("%s",synctex_node_isa(node));
	printf(":%i",info[TAG]);
	printf(",%i",info[LINE]);
	printf(",%i",0);
	printf(":%i",info[HORIZ]);
	printf(",%i",info[VERT]);
	printf(":%i",info[WIDTH]);
	printf(",%i",info[HEIGHT]);
	printf(",%i",info[DEPTH]);
	printf("\nSELF:0x%x",node);
	printf(" PARENT:0x%x",PARENT(node));
	printf(" CHILD:0x%x",CHILD(node));
	printf(" SIBLING:0x%x",SIBLING(node));
	printf(" FRIEND:0x%x\n",FRIEND(node));
}

void _synctex_log_horiz_box(synctex_node_t node) {
	int * info = INFO(node);
	printf("%s",synctex_node_isa(node));
	printf(":%i",info[TAG]);
	printf(",%i",info[LINE]);
	printf(",%i",0);
	printf(":%i",info[HORIZ]);
	printf(",%i",info[VERT]);
	printf(":%i",info[WIDTH]);
	printf(",%i",info[HEIGHT]);
	printf(",%i",info[DEPTH]);
	printf(":%i",info[HORIZ_V]);
	printf(",%i",info[VERT_V]);
	printf(":%i",info[WIDTH_V]);
	printf(",%i",info[HEIGHT_V]);
	printf(",%i",info[DEPTH_V]);
	printf("\nSELF:0x%x",node);
	printf(" PARENT:0x%x",PARENT(node));
	printf(" CHILD:0x%x",CHILD(node));
	printf(" SIBLING:0x%x",SIBLING(node));
	printf(" FRIEND:0x%x\n",FRIEND(node));
}

void _synctex_log_input(synctex_node_t node) {
	int * info = INFO(node);
	printf("%s",synctex_node_isa(node));
	printf(":%i",info[TAG]);
	printf(",%s",info[NAME]);
	printf(" SIBLING:0x%x",SIBLING(node));
}

#   define DISPLAY(NODE) MSG_SEND(NODE,display)

void _synctex_display_sheet(synctex_node_t sheet) {
	if(sheet) {
		printf("....{%i\n",INFO(sheet)[PAGE]);
		DISPLAY(CHILD(sheet));
		printf("....}\n");
		DISPLAY(SIBLING(sheet));
	}
}

void _synctex_display_vbox(synctex_node_t node) {
	printf("....[%i,%i:%i,%i:%i,%i,%i\n",
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT],
		INFO(node)[WIDTH],
		INFO(node)[HEIGHT],
		INFO(node)[DEPTH]);
	DISPLAY(CHILD(node));
	printf("....]\n");
	DISPLAY(SIBLING(node));
}

void _synctex_display_hbox(synctex_node_t node) {
	printf("....(%i,%i:%i,%i:%i,%i,%i\n",
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT],
		INFO(node)[WIDTH],
		INFO(node)[HEIGHT],
		INFO(node)[DEPTH]);
	DISPLAY(CHILD(node));
	printf("....)\n");
	DISPLAY(SIBLING(node));
}

void _synctex_display_void_vbox(synctex_node_t node) {
	printf("....v%i,%i;%i,%i:%i,%i,%i\n",
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT],
		INFO(node)[WIDTH],
		INFO(node)[HEIGHT],
		INFO(node)[DEPTH]);
	DISPLAY(SIBLING(node));
}

void _synctex_display_void_hbox(synctex_node_t node) {
	printf("....h%i,%i:%i,%i:%i,%i,%i\n",
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT],
		INFO(node)[WIDTH],
		INFO(node)[HEIGHT],
		INFO(node)[DEPTH]);
	DISPLAY(SIBLING(node));
}

void _synctex_display_glue(synctex_node_t node) {
	printf("....glue:%i,%i:%i,%i\n",
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT]);
	DISPLAY(SIBLING(node));
}

void _synctex_display_math(synctex_node_t node) {
	printf("....math:%i,%i:%i,%i\n",
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT]);
	DISPLAY(SIBLING(node));
}

void _synctex_display_kern(synctex_node_t node) {
	printf("....kern:%i,%i:%i,%i:%i\n",
		INFO(node)[TAG],
		INFO(node)[LINE],
		INFO(node)[HORIZ],
		INFO(node)[VERT],
		INFO(node)[WIDTH]);
	DISPLAY(SIBLING(node));
}

void _synctex_display_input(synctex_node_t node) {
	printf("....Input:%i:%s\n",
		INFO(node)[TAG],
		INFO(node)[NAME]);
	DISPLAY(SIBLING(node));
}

#   pragma mark -
#   pragma mark SCANNER

#define SYNCTEX_NOERR 0

/*  Used when parsing the synctex file.
 *  Advance to the next character starting a line.
 *  Actually, only \n is recognized as end of line marker. */
int _synctex_next_line(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return -1;
	}
	while(PTR<END) {
		if(*PTR == '\n') {
			++PTR;
			return 0;
		}
		++PTR;
	}
	return 1;
}

/*  Used when parsing the synctex file.
 *  Decode an integer.
 *  First file separators are skipped
 */
int _synctex_decode_int(synctex_scanner_t scanner, int* valueRef) {
	if(NULL == scanner) return -1;
	if(PTR>=END) return -1;
	unsigned char * ptr = PTR;
	if(*ptr==':' || *ptr==',') {
		++ptr;
	}
	unsigned char * end;
	int result = (int)strtol((char *)ptr, (char **)&end, 10);
	if(end>ptr) {
		PTR = end;
		if(valueRef) {
			* valueRef = result;
		}
		return 0;
	}
	return -1;
}

int _synctex_decode_string(synctex_scanner_t scanner, char ** valueRef) {
	if(NULL == scanner || NULL == valueRef) return -1;
	if(PTR>=END) return -1;
	char * end = (char *)PTR;
	while(end<(char *)END && *end != '\n') {
		++end;
	}
	size_t len = end - (char *)PTR;
	if(* valueRef = malloc(len+1)) {
		if(memcpy((*valueRef),(synctex_node_t)PTR,len)) {
			(* valueRef)[len]='\0';
			PTR += len;
			return 0;
		}
		free(* valueRef);
		* valueRef = NULL;
	}
	return -1;
}

/*  Used when parsing the synctex file.
 *  Read an Input record.
 */
int _synctex_scan_input(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return -1;
	}
	if(0 == strncmp((char *)PTR,"Input:",6)) {
		PTR += 6;
		synctex_node_t input = _synctex_new_input(scanner);
		if(_synctex_decode_int(scanner,INFO(input)+TAG)
				|| (++PTR,_synctex_decode_string(scanner,(char **)(INFO(input)+NAME)))
				|| _synctex_next_line(scanner)) {
			FREE(input);
			return -1;
		}
		SET_SIBLING(input,scanner->input)
		scanner->input = input;
		return 0;
	}
	return 1;
}

/*  Used when parsing the synctex file.
 *  Read the settings as part of the preamble.
 */
int _synctex_scan_settings(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return -1;
	}
	while(strncmp((char *)PTR,"Output:",7)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	PTR += 7;
	if(_synctex_decode_string(scanner,&(scanner->output))
			|| _synctex_next_line(scanner)) {
		return -1;
	}
	while(strncmp((char *)PTR,"Magnification:",14)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	PTR += 14;
	if(_synctex_decode_int(scanner,(int *)&(scanner->pre_magnification))
			|| _synctex_next_line(scanner)) {
		return -1;
	}
	while(strncmp((char *)PTR,"Unit:",5)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	PTR += 5;
	if(_synctex_decode_int(scanner,(int *)&(scanner->pre_unit))
			|| _synctex_next_line(scanner)) {
		return -1;
	}
	while(strncmp((char *)PTR,"X Offset:",9)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	PTR += 9;
	if(_synctex_decode_int(scanner,&(scanner->pre_x_offset))
			|| _synctex_next_line(scanner)) {
		return -1;
	}
	while(strncmp((char *)PTR,"Y Offset:",9)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	PTR += 9;
	if(_synctex_decode_int(scanner,&(scanner->pre_y_offset))
			|| _synctex_next_line(scanner)) {
		return -1;
	}
	return 0;
}

/*  Used when parsing the synctex file.
 *  Read the preamplesss.
 */
int _synctex_scan_preamble(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return -1;
	}
	if(strncmp((char *)PTR,"SyncTeX Version:",1)
			|| ((PTR+=16),_synctex_decode_int(scanner,&(scanner->version)))
			|| _synctex_next_line(scanner)) {
		return -1;
	}
	while(SYNCTEX_NOERR == _synctex_scan_input(scanner)) {
	}
	return _synctex_scan_settings(scanner);
}

/*  parse the post scriptum */
#include "xlocale.h"
int _synctex_scan_post_scriptum(synctex_scanner_t scanner) {
	while(strncmp((char *)PTR,"Post scriptum:",14)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	PTR += 14;
	/* Scanning the information */
	/* initialize the offset with a fake value */
	scanner->x_offset = 6.027e23;
	scanner->y_offset = scanner->x_offset;
	/* By default, C programs start in the "C" locale
	 * But we are in a library, so we cannot assume that. */
	locale_t locale = NULL; /*newlocale(LC_ALL_MASK, NULL, NULL);*/
	/* "man xlocale" says that
		If a NULL locale_t is passed, the C locale will be used.
	   so we don't need to actually create one here. */
/*
	if(NULL == locale) {
		fprintf(stderr, "failed to create locale\n");
		return -1;
	}
*/
	int status = 0;
next_record:
	if(0 == strncmp((char *)PTR,"Magnification:",14)) {
		PTR += 14;
		if(PTR<END) {
			scanner->unit = strtof_l((char *)PTR,(char **)&PTR,locale);
next_line:
			status = _synctex_next_line(scanner);
			if(status<0) {
				/*freelocale(locale);*/
				return -1;
			} else if(0 == status) {
				goto next_record;
			}
		}
		/*freelocale(locale);*/
		return 0;
	} else if(0 == strncmp((char *)PTR,"X Offset:",9)) {
		PTR += 9;
		if(PTR<END) {
			unsigned char * end = NULL;
			float f = strtof_l((char *)PTR,(char **)&end,locale);
			if(end>PTR) {
				/* scan the x offset */
				//72/72.27/65536*16384+0.5;
				PTR = end;
				if(strncmp((char *)PTR,"in",2) == 0) {
					f *= 72.27*65536;
				}
				else if(strncmp((char *)PTR,"cm",2) == 0) {
					f *= 72.27*65536/2.54;
				}
				else if(strncmp((char *)PTR,"mm",2) == 0) {
					f *= 72.27*65536/25.4;
				}
				else if(strncmp((char *)PTR,"pt",2) == 0) {
					f *= 65536.0;
				}
				else if(strncmp((char *)PTR,"bp",2) == 0) {
					f *= 72.27/72*65536;
				} 
				else if(strncmp((char *)PTR,"pc",2) == 0) {
					f *= 12.0*65536;
				} 
				else if(strncmp((char *)PTR,"sp",2) == 0) {
					f *= 1.0;
				} 
				else if(strncmp((char *)PTR,"dd",2) == 0) {
					f *= 1238.0/1157*65536;
				} 
				else if(strncmp((char *)PTR,"cc",2) == 0) {
					f *= 14856.0/1157*65536;
				}
				else if(strncmp((char *)PTR,"nd",2) == 0) {
					f *= 685.0/642*65536;
				} 
				else if(strncmp((char *)PTR,"nc",2) == 0) {
					f *= 1370.0/107*65536;
				}
				else {
					f *= 1.0;
					PTR -= 2;
				}
				scanner->x_offset = f;
			}
			goto next_line;
		}
		/*freelocale(locale);*/
		return 0;
	} else if(0 == strncmp((char *)PTR,"Y Offset:",9)) {
		PTR += 9;
		if(PTR<END) {
			unsigned char * end = NULL;
			float f = strtof_l((char *)PTR,(char **)&end,locale);
			if(end>PTR) {
				/* scan the x offset */
				//72/72.27/65536*16384+0.5;
				PTR = end;
				if(strncmp((char *)PTR,"in",2) == 0) {
					f *= 72.27*65536;
				}
				else if(strncmp((char *)PTR,"cm",2) == 0) {
					f *= 72.27*65536/2.54;
				}
				else if(strncmp((char *)PTR,"mm",2) == 0) {
					f *= 72.27*65536/25.4;
				}
				else if(strncmp((char *)PTR,"pt",2) == 0) {
					f *= 65536.0;
				}
				else if(strncmp((char *)PTR,"bp",2) == 0) {
					f *= 72.27/72*65536;
				} 
				else if(strncmp((char *)PTR,"pc",2) == 0) {
					f *= 12.0*65536;
				} 
				else if(strncmp((char *)PTR,"sp",2) == 0) {
					f *= 1.0;
				} 
				else if(strncmp((char *)PTR,"dd",2) == 0) {
					f *= 1238.0/1157*65536;
				} 
				else if(strncmp((char *)PTR,"cc",2) == 0) {
					f *= 14856.0/1157*65536;
				}
				else if(strncmp((char *)PTR,"nd",2) == 0) {
					f *= 685.0/642*65536;
				} 
				else if(strncmp((char *)PTR,"nc",2) == 0) {
					f *= 1370.0/107*65536;
				}
				else {
					f *= 1.0;
					PTR -= 2;
				}
				scanner->y_offset = f;
			}
			goto next_line;
		}
		/*freelocale(locale);*/
		return 0;
	}
	goto next_line;
}

int _synctex_scan_postamble(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return -1;
	}
	if(strncmp((char *)PTR,"Postamble:",10)
			|| ((PTR += 10),_synctex_next_line(scanner))) {
		return -1;
	}
	if(!strncmp((char *)PTR,"Count:",6)) {
		PTR += 6;
		if(_synctex_decode_int(scanner,&(scanner->count))
				|| _synctex_next_line(scanner)) {
			return -1;
		}
	}
	while(_synctex_scan_post_scriptum(scanner)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	return 0;
}

/*  Horizontal boxes also have visible size.
 *  Visible size are bigger than real size.
 *  For example 0 width boxes may contain text.
 *  At creation time, the visible size is set to the values of the real size.
 */
int _synctex_setup_visible_box(synctex_node_t box) {
	if(NULL == box || box->class->type != synctex_node_type_hbox) {
		return -1;
	}
	int * info = INFO(box);
	if(info) {
		info[HORIZ_V] = info[HORIZ];
		info[VERT_V] = info[VERT];
		info[WIDTH_V] = info[WIDTH];
		info[HEIGHT_V] = info[HEIGHT];
		info[DEPTH_V] = info[DEPTH];
		return 0;
	}
	return -1;
}

/*  This method is sent to an horizontal box to setup the visible size
 *  Some box have 0 width but do contain text material.
 *  With this method, one can enlarge the box to contain the given point (h,v).
 */
int _synctex_horiz_box_setup_visible(synctex_node_t node,int h, int v) {
	if(NULL == node || node->class->type != synctex_node_type_hbox) {
		return -1;
	}
	int * itsINFO = INFO(node);
	int itsBtm, itsTop;
	if(itsINFO[WIDTH_V]<0) {
		itsBtm = itsINFO[HORIZ_V]+itsINFO[WIDTH_V];
		itsTop = itsINFO[HORIZ_V];
		if(h<itsBtm) {
			itsBtm -= h;
			itsINFO[WIDTH_V] -= itsBtm;
		}
		if(h>itsTop) {
			h -= itsTop;
			itsINFO[WIDTH_V] -= h;
			itsINFO[HORIZ_V] += h;
		}
	} else {
		itsBtm = itsINFO[HORIZ_V];
		itsTop = itsINFO[HORIZ_V]+itsINFO[WIDTH_V];
		if(h<itsBtm) {
			itsBtm -= h;
			itsINFO[HORIZ_V] -= itsBtm;
			itsINFO[WIDTH_V] += itsBtm;
		}
		if(h>itsTop) {
			h -= itsTop;
			itsINFO[WIDTH_V] += h;
		}
	}
	return 0;
}

/*  Used when parsing the synctex file.
 *  The parent is a newly created sheet node that will hold the contents.
 *  Something is returned in case of error.
 */
int _synctex_scan_sheet(synctex_scanner_t scanner, synctex_node_t parent) {
	if((NULL == scanner) || (NULL == parent))return -1;
	synctex_node_t child = NULL;
	synctex_node_t sibling = NULL;
	int friend_index = 0;
	int * info = NULL;
	int curh, curv;
vertical_loop:
	if(PTR<END) {
		if(*PTR == '[') {
			++PTR;
			if((child = _synctex_new_vbox(scanner)) && (info = INFO(child))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_CHILD(parent,child);
				parent = child;
				child = NULL;
				goto vertical_loop;
			} else {
				return -1;
			}
		} else if(*PTR == ']') {
			++PTR;
			if(parent && parent->class->type == synctex_node_type_vbox) {
				#define UPDATE_BOX_FRIEND(NODE)\
				friend_index = ((INFO(NODE))[TAG]+(INFO(NODE))[LINE])%(scanner->number_of_lists);\
				SET_FRIEND(NODE,(scanner->lists_of_friends)[friend_index]);\
				(scanner->lists_of_friends)[friend_index] = NODE;
				if(NULL == CHILD(parent)) {
					/* only void boxes are friends */
					UPDATE_BOX_FRIEND(parent);
				}
				child = parent;
				parent = PARENT(child);
			} else {
				printf("Unexpected ]\n");
			}
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto horizontal_loop;
		} else if(*PTR == '(') {
			++PTR;
			if((child = _synctex_new_hbox(scanner)) && (info = INFO(child))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_setup_visible_box(child)
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_CHILD(parent,child);
				parent = child;
				child = NULL;
				goto vertical_loop;
			} else {
				return -1;
			}
		} else if(*PTR == ')') {
			++PTR;
			if(parent && parent->class->type == synctex_node_type_hbox) {
				if(NULL == child) {
					UPDATE_BOX_FRIEND(parent);
				}
				child = parent;
				parent = PARENT(child);
			} else {
				printf("Unexpected )\n");
			}
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto horizontal_loop;
		} else if(*PTR == 'v') {
			++PTR;
			if((child = _synctex_new_void_vbox(scanner)) && (info = INFO(child))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_CHILD(parent,child);
				#define UPDATE_FRIEND(NODE)\
				friend_index = (info[TAG]+info[LINE])%(scanner->number_of_lists);\
				SET_FRIEND(NODE,(scanner->lists_of_friends)[friend_index]);\
				(scanner->lists_of_friends)[friend_index] = NODE;
				UPDATE_FRIEND(child);
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == 'h') {
			++PTR;
			if((child = _synctex_new_void_hbox(scanner)) && (info = INFO(child))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_CHILD(parent,child);
				UPDATE_FRIEND(child);
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child),synctex_node_v(child));
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child)+synctex_node_width(child),synctex_node_v(child));
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == 'k') {
			++PTR;
			if((child = _synctex_new_kern(scanner)) && (info = INFO(child))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_CHILD(parent,child);
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child),synctex_node_v(child));
				UPDATE_FRIEND(child);
				if(!parent) {
					printf("Child is:");
					synctex_node_log(child);
					return -1;
				}
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == 'x') {
			++PTR;
			if(_synctex_decode_int(scanner,NULL)
						|| _synctex_decode_int(scanner,NULL)
						|| _synctex_decode_int(scanner,&curh)
						|| _synctex_decode_int(scanner,&curv)
						|| _synctex_next_line(scanner)) {
				return -1;
			}
			_synctex_horiz_box_setup_visible(parent,curh,curv);
			goto vertical_loop;
		} else if(*PTR == 'g') {
			++PTR;
			if((child = _synctex_new_glue(scanner)) && (info = INFO(child))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_CHILD(parent,child);
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child),synctex_node_v(child));
				UPDATE_FRIEND(child);
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == '$') {
			++PTR;
			if((child = _synctex_new_math(scanner)) && (info = INFO(child))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_CHILD(parent,child);
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child),synctex_node_v(child));
				UPDATE_FRIEND(child);
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == '}') {
			++PTR;
			if(!parent || parent->class->type != synctex_node_type_sheet
					|| _synctex_next_line(scanner)) {
				return -1;
			}
			return 0;
		} else if(*PTR == '!') {
			++PTR;
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto vertical_loop;
		} else {
			//printf("Ignored:<%c>\n",*PTR);
			++PTR;
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto vertical_loop;
		}
	} else {
		return -1;
	}
	synctex_bail();
horizontal_loop:
	if(PTR<END) {
		if(*PTR == '[') {
			++PTR;
			if((sibling = _synctex_new_vbox(scanner)) && (info = INFO(sibling))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_SIBLING(child,sibling);
				parent = sibling;
				child = NULL;
				goto vertical_loop;
			} else {
				return -1;
			}
		} else if(*PTR == ']') {
			++PTR;
			if(parent && parent->class->type == synctex_node_type_vbox) {
				if(NULL == child) {
					UPDATE_BOX_FRIEND(parent);
				}
				child = parent;
				parent = PARENT(child);
			} else {
				printf("Unexpected ]\n");
			}
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto horizontal_loop;
		} else if(*PTR == '(') {
			++PTR;
			if((sibling = _synctex_new_hbox(scanner)) && (info = INFO(sibling))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_setup_visible_box(sibling)
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_SIBLING(child,sibling);
				parent = sibling;
				child = NULL;
				goto vertical_loop;
			} else {
				return -1;
			}
		} else if(*PTR == ')') {
			++PTR;
			if(parent && parent->class->type == synctex_node_type_hbox) {
				if(NULL == child) {
					UPDATE_BOX_FRIEND(parent);
				}
				child = parent;
				parent = PARENT(child);
			} else {
				printf("Unexpected )\n");
			}
			if(_synctex_next_line(scanner)) {
				LOG(child);
				return -1;
			}
			goto horizontal_loop;
		} else if(*PTR == 'v') {
			++PTR;
			if((sibling = _synctex_new_void_vbox(scanner)) && (info = INFO(sibling))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_SIBLING(child,sibling);
				UPDATE_FRIEND(sibling);
				child = sibling;
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == 'h') {
			++PTR;
			if((sibling = _synctex_new_void_hbox(scanner)) && (info = INFO(sibling))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_decode_int(scanner,(int*)(info+HEIGHT))
						|| _synctex_decode_int(scanner,(int*)(info+DEPTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_SIBLING(child,sibling);
				UPDATE_FRIEND(sibling);
				child = sibling;
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child),synctex_node_v(child));
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child)+synctex_node_width(child),synctex_node_v(child));
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == 'x') {
			++PTR;
			if(_synctex_decode_int(scanner,NULL)
						|| _synctex_decode_int(scanner,NULL)
						|| _synctex_decode_int(scanner,&curh)
						|| _synctex_decode_int(scanner,&curv)
						|| _synctex_next_line(scanner)) {
					return -1;
			}
			_synctex_horiz_box_setup_visible(parent,curh,curv);
			goto horizontal_loop;
		} else if(*PTR == 'k') {
			++PTR;
			if((sibling = _synctex_new_kern(scanner)) && (info = INFO(sibling))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_decode_int(scanner,(int*)(info+WIDTH))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_SIBLING(child,sibling);
				UPDATE_FRIEND(sibling);
				child = sibling;
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child),synctex_node_v(child));
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == 'g') {
			++PTR;
			if((sibling = _synctex_new_glue(scanner)) && (info = INFO(sibling))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_SIBLING(child,sibling);
				UPDATE_FRIEND(sibling);
				child = sibling;
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(child),synctex_node_v(child));
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == '$') {
			++PTR;
			if((sibling = _synctex_new_math(scanner)) && (info = INFO(sibling))) {
				if(_synctex_decode_int(scanner,(int*)(info+TAG))
						|| _synctex_decode_int(scanner,(int*)(info+LINE))
						|| _synctex_decode_int(scanner,(int*)(info+HORIZ))
						|| _synctex_decode_int(scanner,(int*)(info+VERT))
						|| _synctex_next_line(scanner)) {
					return -1;
				}
				SET_SIBLING(child,sibling);
				_synctex_horiz_box_setup_visible(parent,synctex_node_h(sibling),synctex_node_v(sibling));
				UPDATE_FRIEND(sibling);
				child = sibling;
				goto horizontal_loop;
			} else {
				return -1;
			}
		} else if(*PTR == '}') {
			++PTR;
			if(!parent || parent->class->type != synctex_node_type_sheet
					|| _synctex_next_line(scanner)) {
				return -1;
			}
			return 0;
		} else if(*PTR == '!') {
			++PTR;
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto horizontal_loop;
		} else {
			//printf("SyncTeX: Ignored record %c\n",*PTR);
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto horizontal_loop;
		}
	} else {
		return -1;
	}
}

/*  Used when parsing the synctex file
 */
int _synctex_scan_content(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return -1;
	}
	/* set up the lists of friends */
	if(NULL == scanner->lists_of_friends) {
		scanner->number_of_lists = 1024;
		scanner->lists_of_friends = (synctex_node_t *)_synctex_malloc(scanner->number_of_lists*sizeof(synctex_node_t));
		if(NULL == scanner->lists_of_friends) {
			printf("malloc:2\n");
			return -1;
		}
	}
	/* Find where this section starts */
	while(strncmp((char *)PTR,"Content:",8)) {
		if(_synctex_next_line(scanner)) {
			return -1;
		}
	}
	PTR += 8;
	if(_synctex_next_line(scanner)) {
		return -1;
	}
next_sheet:
	if(*PTR != '{') {
		if(_synctex_scan_postamble(scanner)) {
			if(_synctex_next_line(scanner)) {
				return -1;
			}
			goto next_sheet;
		}
		return 0;
	}
	++PTR;
	/* Create a new sheet node */
	synctex_node_t sheet = _synctex_new_sheet(scanner);
	if(_synctex_decode_int(scanner,INFO(sheet)+PAGE)
			|| _synctex_next_line(scanner)) {
bail:
		FREE(sheet);
		return -1;
	}
	if(_synctex_scan_sheet(scanner,sheet)) {
		goto bail;
	}
	SET_SIBLING(sheet,scanner->sheet);
	scanner->sheet = sheet;
	while(SYNCTEX_NOERR == _synctex_scan_input(scanner)) {
	}
	goto next_sheet;
}

/*  Where the synctex scanner is created.
 *  name is the full path of the synctex file. */
synctex_scanner_t synctex_scanner_new_with_contents_of_file(const char * name) {
	synctex_scanner_t scanner = (synctex_scanner_t)_synctex_malloc(
		sizeof(_synctex_scanner_t) + synctex_node_type_last * sizeof(_synctex_class_t));
	if(NULL == scanner) {
		return NULL;
	}
	scanner->pre_magnification = 1000;
	scanner->pre_unit = 8192;
	scanner->pre_x_offset = scanner->pre_y_offset = 578;
	scanner->class[synctex_node_type_sheet] = synctex_class_sheet;
	(scanner->class[synctex_node_type_sheet]).scanner = scanner;
	scanner->class[synctex_node_type_vbox] = synctex_class_vbox;
	(scanner->class[synctex_node_type_vbox]).scanner = scanner;
	scanner->class[synctex_node_type_void_vbox] = synctex_class_void_vbox;
	(scanner->class[synctex_node_type_void_vbox]).scanner = scanner;
	scanner->class[synctex_node_type_hbox] = synctex_class_hbox;
	(scanner->class[synctex_node_type_hbox]).scanner = scanner;
	scanner->class[synctex_node_type_void_hbox] = synctex_class_void_hbox;
	(scanner->class[synctex_node_type_void_hbox]).scanner = scanner;
	scanner->class[synctex_node_type_kern] = synctex_class_kern;
	(scanner->class[synctex_node_type_kern]).scanner = scanner;
	scanner->class[synctex_node_type_glue] = synctex_class_glue;
	(scanner->class[synctex_node_type_glue]).scanner = scanner;
	scanner->class[synctex_node_type_math] = synctex_class_math;
	(scanner->class[synctex_node_type_math]).scanner = scanner;
	scanner->class[synctex_node_type_input] = synctex_class_input;
	(scanner->class[synctex_node_type_input]).scanner = scanner;

	FILE * F = fopen(name,"r");
	if(NULL == F) {
		printf("SyncTeX: could not open %s, error %i\n",name,errno);
bail:
		synctex_scanner_free(scanner);
		return NULL;
	}
	if(fseek(F, 0, SEEK_END)) {
		close(F);
		goto bail;
	}
	size_t size = ftell(F);
	rewind(F);
	START = (unsigned char *)malloc(size+1);
	if(NULL == START) {
		printf("malloc error\n");
		close(F);
		goto bail;
	}
	if(size != fread((void *)START, 1, size, F)) {
bailey:
		close(F);
		goto bail;
	}
	close(F);
	START[size] = '\0'; /* ensure null termination */
	/* first read the beginning */
	END = START + size;
	PTR = START;
	if(_synctex_scan_preamble(scanner)
			|| _synctex_scan_content(scanner)) {
		goto bailey;
	}
	free((void *)START);
	START = PTR = END = NULL;
	/* Everything is finished, final tuning */
	/* 1 pre_unit = (scanner->pre_unit)/65536 pt = (scanner->pre_unit)/65781.76 bp
	 * 1 pt = 65536 sp */
	if(scanner->pre_unit<=0) {
		scanner->pre_unit = 8192;
	}
	if(scanner->pre_magnification<=0) {
		scanner->pre_magnification = 1000;
	}
	if(scanner->unit <= 0) {
		/* no post magnification */
		scanner->unit = scanner->pre_unit / 65781.76;/* 65781.76 or 65536.0*/
	} else {
		/* post magnification */
		scanner->unit *= scanner->pre_unit / 65781.76;
	}
	scanner->unit *= scanner->pre_magnification / 1000.0;
	if(scanner->x_offset > 6e23) {
		/* no post offset */
		scanner->x_offset = scanner->pre_x_offset * (scanner->pre_unit / 65781.76);
		scanner->y_offset = scanner->pre_y_offset * (scanner->pre_unit / 65781.76);
	} else {
		/* post offset */
		scanner->x_offset /= 65781.76;
		scanner->y_offset /= 65781.76;
	}
	return scanner;
}

/*  The scanner destructor
 */
void synctex_scanner_free(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return;
	}
	FREE(scanner->sheet);
	free(START);
	free(scanner->output);
	free(scanner->input);
	free(scanner->lists_of_friends);
	free(scanner);
}

/*  Scanner accessors.
 */
int synctex_scanner_pre_x_offset(synctex_scanner_t scanner){
	return scanner?scanner->pre_x_offset:0;
}
int synctex_scanner_pre_y_offset(synctex_scanner_t scanner){
	return scanner?scanner->pre_y_offset:0;
}
int synctex_scanner_x_offset(synctex_scanner_t scanner){
	return scanner?scanner->x_offset:0;
}
int synctex_scanner_y_offset(synctex_scanner_t scanner){
	return scanner?scanner->y_offset:0;
}
float synctex_scanner_magnification(synctex_scanner_t scanner){
	return scanner?scanner->unit:1;
}
void synctex_scanner_display(synctex_scanner_t scanner) {
	if(NULL == scanner) {
		return;
	}
	printf("The scanner:\noutput:%s\nversion:%i\n",scanner->output,scanner->version);
	printf("pre_unit:%i\nx_offset:%i\nx_offset:%i\n",scanner->pre_unit,scanner->pre_x_offset,scanner->pre_y_offset);
	printf("count:%i\npost_magnification:%f\npost_x_offset:%i\npost_x_offset:%i\n",
		scanner->count,scanner->unit,scanner->x_offset,scanner->y_offset);
	printf("The input:\n");
	DISPLAY(scanner->input);
	if(scanner->count<1000) {
		printf("The sheets:\n");
		DISPLAY(scanner->sheet);
		printf("The friends:\n");
		if(scanner->lists_of_friends) {
			int i = scanner->number_of_lists;
			synctex_node_t node;
			while(i--) {
				printf("Friend index:%i\n",i);
				node = (scanner->lists_of_friends)[i];
				while(node) {
					printf("%s:%i,%i\n",
						synctex_node_isa(node),
						INFO(node)[TAG],
						INFO(node)[LINE]
					);
					node = FRIEND(node);
				}
			}
		}
	} else {
		printf("Too many objects");
	}
}
/*  Public*/
const char * synctex_scanner_get_name(synctex_scanner_t scanner,int tag) {
	if(NULL == scanner) {
		return "";
	}
	synctex_node_t input = scanner->input;
	do {
		if(tag == INFO(input)[TAG]) {
			return (char *)(INFO(input)[NAME]);
		}
	} while(input = SIBLING(input));
	return 0;
}
int synctex_scanner_get_tag(synctex_scanner_t scanner,const char * name) {
	if(NULL == scanner) {
		return 0;
	}
	synctex_node_t input = scanner->input;
	do {
		if((strlen(name) == strlen((char *)(INFO(input)[NAME]))) &&
				(0 == strncmp(name,(char *)(INFO(input)[NAME]),strlen(name)))) {
			return INFO(input)[TAG];
		}
	} while(input = SIBLING(input));
	return 0;
}
synctex_node_t synctex_scanner_input(synctex_scanner_t scanner) {
	return scanner?scanner->input:NULL;
}
#pragma mark -
#pragma mark Public node attributes
float synctex_node_h(synctex_node_t node){
	if(!node) {
		return 0;
	}
	return (float)INFO(node)[HORIZ];
}
float synctex_node_v(synctex_node_t node){
	if(!node) {
		return 0;
	}
	return (float)INFO(node)[VERT];
}
float synctex_node_width(synctex_node_t node){
	if(!node) {
		return 0;
	}
	return (float)INFO(node)[WIDTH];
}
float synctex_node_box_h(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type != synctex_node_type_vbox)
	&& (node->class->type != synctex_node_type_void_vbox)
	&& (node->class->type != synctex_node_type_hbox)
	&& (node->class->type != synctex_node_type_void_hbox)) {
		node = PARENT(node);
	}
	return (node->class->type == synctex_node_type_sheet)?0:(float)(INFO(node)[HORIZ]);
}
float synctex_node_box_v(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type != synctex_node_type_vbox)
	&& (node->class->type != synctex_node_type_void_vbox)
	&& (node->class->type != synctex_node_type_hbox)
	&& (node->class->type != synctex_node_type_void_hbox)) {
		node = PARENT(node);
	}
	return (node->class->type == synctex_node_type_sheet)?0:(float)(INFO(node)[VERT]);
}
float synctex_node_box_width(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type != synctex_node_type_vbox)
	&& (node->class->type != synctex_node_type_void_vbox)
	&& (node->class->type != synctex_node_type_hbox)
	&& (node->class->type != synctex_node_type_void_hbox)) {
		node = PARENT(node);
	}
	return (node->class->type == synctex_node_type_sheet)?0:(float)(INFO(node)[WIDTH]);
}
float synctex_node_box_height(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type != synctex_node_type_vbox)
	&& (node->class->type != synctex_node_type_void_vbox)
	&& (node->class->type != synctex_node_type_hbox)
	&& (node->class->type != synctex_node_type_void_hbox)) {
		node = PARENT(node);
	}
	return (node->class->type == synctex_node_type_sheet)?0:(float)(INFO(node)[HEIGHT]);
}
float synctex_node_box_depth(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type != synctex_node_type_vbox)
	&& (node->class->type != synctex_node_type_void_vbox)
	&& (node->class->type != synctex_node_type_hbox)
	&& (node->class->type != synctex_node_type_void_hbox)) {
		node = PARENT(node);
	}
	return (node->class->type == synctex_node_type_sheet)?0:(float)(INFO(node)[DEPTH]);
}
#pragma mark -
#pragma mark Public node visible attributes
float synctex_node_visible_h(synctex_node_t node){
	if(!node) {
		return 0;
	}
	return INFO(node)[HORIZ]*node->class->scanner->unit+node->class->scanner->x_offset;
}
float synctex_node_visible_v(synctex_node_t node){
	if(!node) {
		return 0;
	}
	return INFO(node)[VERT]*node->class->scanner->unit+node->class->scanner->y_offset;
}
float synctex_node_visible_width(synctex_node_t node){
	if(!node) {
		return 0;
	}
	return INFO(node)[WIDTH]*node->class->scanner->unit;
}
float synctex_node_box_visible_h(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type == synctex_node_type_vbox)
	|| (node->class->type == synctex_node_type_void_hbox)
	|| (node->class->type == synctex_node_type_void_vbox)) {
result:
		return INFO(node)[WIDTH]<0?
			(INFO(node)[HORIZ]+INFO(node)[WIDTH])*node->class->scanner->unit+node->class->scanner->x_offset:
			INFO(node)[HORIZ]*node->class->scanner->unit+node->class->scanner->x_offset;
	}
	if(node->class->type != synctex_node_type_hbox) {
		node = PARENT(node);
	}
	if(node->class->type == synctex_node_type_sheet) {
		return 0;
	}
	if(node->class->type == synctex_node_type_vbox) {
		goto result;
	}
	return INFO(node)[WIDTH_V]<0?
		(INFO(node)[HORIZ_V]+INFO(node)[WIDTH_V])*node->class->scanner->unit+node->class->scanner->x_offset:
		INFO(node)[HORIZ_V]*node->class->scanner->unit+node->class->scanner->x_offset;
}
float synctex_node_box_visible_v(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type == synctex_node_type_vbox)
	|| (node->class->type == synctex_node_type_void_hbox)
	|| (node->class->type == synctex_node_type_void_vbox)) {
result:
		return (float)(INFO(node)[VERT])*node->class->scanner->unit+node->class->scanner->y_offset;
	}
	if((node->class->type != synctex_node_type_vbox)
	&& (node->class->type != synctex_node_type_hbox)) {
		node = PARENT(node);
	}
	if(node->class->type == synctex_node_type_sheet) {
		return 0;
	}
	if(node->class->type == synctex_node_type_vbox) {
		goto result;
	}
	return INFO(node)[VERT_V]*node->class->scanner->unit+node->class->scanner->y_offset;
}
float synctex_node_box_visible_width(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type == synctex_node_type_vbox)
	|| (node->class->type == synctex_node_type_void_hbox)
	|| (node->class->type == synctex_node_type_void_vbox)) {
result:
		return INFO(node)[WIDTH]<0?
			-INFO(node)[WIDTH]*node->class->scanner->unit:
			INFO(node)[WIDTH]*node->class->scanner->unit;
	}
	if(node->class->type != synctex_node_type_hbox) {
		node = PARENT(node);
	}
	if(node->class->type == synctex_node_type_sheet) {
		return 0;
	}
	if(node->class->type == synctex_node_type_vbox) {
		goto result;
	}
	return INFO(node)[WIDTH_V]<0?
		-INFO(node)[WIDTH_V]*node->class->scanner->unit:
		INFO(node)[WIDTH_V]*node->class->scanner->unit;
}
float synctex_node_box_visible_height(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type == synctex_node_type_vbox)
	|| (node->class->type == synctex_node_type_void_hbox)
	|| (node->class->type == synctex_node_type_void_vbox)) {
result:
		return (float)(INFO(node)[HEIGHT])*node->class->scanner->unit;
	}
	if(node->class->type != synctex_node_type_hbox) {
		node = PARENT(node);
	}
	if(node->class->type == synctex_node_type_sheet) {
		return 0;
	}
	if(node->class->type == synctex_node_type_vbox) {
		goto result;
	}
	return INFO(node)[HEIGHT_V]*node->class->scanner->unit;
}
float synctex_node_box_visible_depth(synctex_node_t node){
	if(!node) {
		return 0;
	}
	if((node->class->type == synctex_node_type_vbox)
	|| (node->class->type == synctex_node_type_void_hbox)
	|| (node->class->type == synctex_node_type_void_vbox)) {
result:
		return (float)(INFO(node)[DEPTH])*node->class->scanner->unit;
	}
	if(node->class->type != synctex_node_type_hbox) {
		node = PARENT(node);
	}
	if(node->class->type == synctex_node_type_sheet) {
		return 0;
	}
	if(node->class->type == synctex_node_type_vbox) {
		goto result;
	}
	return INFO(node)[DEPTH_V]*node->class->scanner->unit;
}
#pragma mark -
#pragma mark Other public node attributes
int synctex_node_page(synctex_node_t node){
	if(!node) {
		return -1;
	}
	synctex_node_t parent = PARENT(node);
	while(parent) {
		node = parent;
		parent = PARENT(node);
	}
	if(node->class->type == synctex_node_type_sheet) {
		return INFO(node)[PAGE];
	}
	return -1;
}
int synctex_node_tag(synctex_node_t node) {
	return node?INFO(node)[TAG]:-1;
}
int synctex_node_line(synctex_node_t node) {
	return node?INFO(node)[LINE]:-1;
}
int synctex_node_column(synctex_node_t node) {
	return -1;
}
#pragma mark -
#pragma mark Query

synctex_node_t synctex_sheet_content(synctex_scanner_t scanner,int page) {
	if(scanner) {
		synctex_node_t sheet = scanner->sheet;
		while(sheet) {
			if(page == INFO(sheet)[PAGE]) {
				return CHILD(sheet);
			}
			sheet = SIBLING(sheet);
		}
	}
	return NULL;
}

int synctex_display_query(synctex_scanner_t scanner,const char * name,int line,int column) {
	int tag = synctex_scanner_get_tag(scanner,name);
	if(tag == 0) {
		printf("No tag for %s\n",name);
		return -1;
	}
	free(START);
	PTR = END = START = NULL;
	size_t size = 0;
	int friend_index = (tag+line)%(scanner->number_of_lists);
	synctex_node_t node = (scanner->lists_of_friends)[friend_index];
	while(node) {
		if((tag == INFO(node)[TAG]) && (line == INFO(node)[LINE])) {
			if(PTR == END) {
				size += 16;
				END = realloc(START,size*sizeof(synctex_node_t *));
				PTR += END - START;
				START = END;
				END = START + size*sizeof(synctex_node_t *);
			}			
			*(int *)PTR = (int)node;
			PTR += sizeof(int);
		}
		node = FRIEND(node);
	}
	END = PTR;
	PTR = NULL;
	return END-START;
}

int _synctex_node_is_box(synctex_node_t node) {
	switch(synctex_node_type(node)) {
		case synctex_node_type_hbox:
		case synctex_node_type_void_hbox:
		case synctex_node_type_vbox:
		case synctex_node_type_void_vbox:
			return -1;
	}
	return 0;
}

int _synctex_point_in_visible_box(float h, float v, synctex_node_t node) {
	if(_synctex_node_is_box(node)) {
		v -= synctex_node_box_visible_v(node);
		if((v<=-synctex_node_box_visible_height(node)
					&& v>=synctex_node_box_visible_depth(node))
				|| (v>=-synctex_node_box_visible_height(node)
					&& v<= synctex_node_box_visible_depth(node))) {
			h -= synctex_node_box_visible_h(node);
			if((h<=0 && h>=synctex_node_box_visible_width(node))
					|| (h>=0 && h<=synctex_node_box_visible_width(node))) {
				return -1;
			}
		}
	}
	return 0;
}

int synctex_edit_query(synctex_scanner_t scanner,int page,float h,float v) {
	if(NULL == scanner) {
		return 0;
	}
	free(START);
	START = END = PTR = NULL;
	synctex_node_t sheet = scanner->sheet;
	while(INFO(sheet)[PAGE] != page) {
		sheet = SIBLING(sheet);
	}
	if(NULL == sheet) {
		return -1;
	}
	/* Now sheet points to the sheet node with proper page number */
	/* Declare memory storage, a buffer to hold found nodes */
	synctex_node_t * start = NULL;
	synctex_node_t * end = NULL;
	synctex_node_t * ptr = NULL;
	size_t size = 0;
	synctex_node_t node = CHILD(sheet); /* start with the child of the sheet */
	synctex_node_t next;
has_node_any_child:
	if(next = CHILD(node)) {
		/* node is a non void box */
		if(_synctex_point_in_visible_box(h,v,node)) {
			/* we found a non void box containing the point */
			if(ptr == end) {
				/* not enough room to store the result, add 16 more node records */
				size += 16;
				end = realloc(start,size*sizeof(synctex_node_t));
				if(end == NULL) {
					return -1;
				}
				ptr += end - start;
				start = end;
				end = start + size;
			}			
			*ptr = node;
			/* Does an included box also contain the hit point?
			 * If this is the case, ptr will be overriden later
			 * This is why we do not increment ptr yet.
			 * ptr will be incremented (registered) later if
			 * no enclosing box contains the hit point */
		}
		node = next;
		goto has_node_any_child;
	}
	/* node has no child */
node_has_no_child:
	if(_synctex_point_in_visible_box(h,v,node)) {
		/* we found a void box containing the hit point */
		if(ptr == end) {
			/* not enough room to store the result, add 16 more node records */
			size += 16;
			end = realloc(start,size*sizeof(synctex_node_t));
			if(end == NULL) {
				return -1;
			}
			ptr += end - start;
			start = end;
			end = start + size*sizeof(synctex_node_t);
		}			
		*ptr = node;
		/* Increment ptr to definitely register the node */
		++ptr;
		/* Ensure that it is the last node */
		*ptr = NULL;
	}
next_sibling:
	if(next = SIBLING(node)) {
		node = next;
		goto has_node_any_child;
	}
		/* This is the last node at this level
		 * The next step is the parent's sibling */
		next = PARENT(node);
		if(ptr && *ptr == next) {
			/* No included box does contain the point
			 * next was already tagged to contain the hit point
			 * but was not fully registered at that time, now we can increment ptr */
			++ptr;
			*ptr = NULL;
		} else if(next == sheet) {
we_are_done:
		end = ptr;
		ptr = NULL;
		/* Up to now, we have found a list of boxes enclosing the hit point. */
		if(end == start) {
			/* No match found */
			return 0;
		}
		/* If there are many different boxes containing the hit point, put the smallest one front.
		 * This is in general the expected box in LaTeX picture environment. */
		ptr = start;
		node = *ptr;
		float best = synctex_node_box_visible_width(node);
		float candidate;
		synctex_node_t * best_node_ref = NULL;
		while(node = *(++ptr)) {
			candidate = synctex_node_box_visible_width(node);
			if(candidate<best) {
				best = candidate;
				best_node_ref = ptr;
			}
		}
		if(best_node_ref) {
			node = *best_node_ref;
			*best_node_ref = *start;
			*start = node;
		}
		/* We do need to check children to find out the node closest to the hit point.
		 * Working with boxes is not very accurate because in general boxes are created asynchronously.
		 * The glue, kern, math are more appropriate for synchronization. */
		if(node = CHILD(*start)) {
			best = INFINITY;
			synctex_node_t best_node = NULL;
			do {
				switch((node->class)->type) {
					default:
						candidate = fabsf(synctex_node_visible_h(node)-h);
						if(candidate<best) {
							best = candidate;
							best_node = node;
						}
					case synctex_node_type_hbox:
					case synctex_node_type_vbox:
						break;
				}			
			} while(node = SIBLING(node));
			if(best_node) {
				if(START = malloc(sizeof(synctex_node_t))) {
					* (synctex_node_t *)START = best_node;
					END = START + sizeof(synctex_node_t);
					PTR = NULL;
					free(start);
					return (END-START)/sizeof(synctex_node_t);
				}
			}
		}
		START = (unsigned char *)start;
		END = (unsigned char *)end;
		PTR = NULL;
		return (END-START)/sizeof(synctex_node_t);
	} else if(NULL == next) {
		/* What? a node with no parent? */
		printf("This is an error\n");
		goto we_are_done;
	}
	node = next;
	goto next_sibling;
}

synctex_node_t synctex_next_result(synctex_scanner_t scanner) {
	if(NULL == PTR) {
		PTR = START;
	} else {
		PTR+=sizeof(synctex_node_t);
	}
	if(PTR<END) {
		return *(synctex_node_t*)PTR;
	} else {
		return NULL;
	}
}

int synctex_bail(void) {
		printf("*** ERROR\n");
		return -1;
}
#pragma mark -
#pragma mark TESTS
/*  This is not public, it is not up to date */
synctex_scanner_t synctex_scanner_new_with_data(const void * bytes, unsigned int length) {
	synctex_scanner_t scanner = (synctex_scanner_t)_synctex_malloc(sizeof(_synctex_scanner_t));
	if(NULL != scanner) {
		START = (void *)bytes;
		if(UINT_MAX-length<(int)(START)) {
bail:
			free(scanner);
			return NULL;
		}
		END = START+length;
		scanner->pre_unit = 8192;
		scanner->pre_x_offset = scanner->pre_y_offset = 578;
		unsigned char the_char;
		if(_synctex_scan_preamble(scanner)) {
			goto bail;
		}
		do {
			if(the_char == '{') {
				if(_synctex_scan_content(scanner)) {
					goto bail;
				}
				break;
			}
		} while(SYNCTEX_NOERR == _synctex_scan_next_line_header(scanner,&the_char));
	}
	return scanner;
}

int _synctex_scan_next_line_header(synctex_scanner_t scanner, unsigned char * valueRef) {
	if(NULL == scanner || NULL == valueRef) return -1;
	/* read until the next '\0' byte, return the following one */
	while(PTR<END) {
		if(*PTR=='\0') {
			if(++PTR<END) {
				*valueRef = *(PTR++);
				return 0;
			}
		} else {
			++PTR;
		}
	}
	return -1;
}
int synctex_jump(synctex_scanner_t scanner)
{
	unsigned char the_char;
	int n;
	unsigned char * offset = END;
	while(offset>scanner->buffer_start) {
		if(*(--offset)) {
			continue;
		}
		PTR = offset;
		if(SYNCTEX_NOERR == _synctex_scan_next_line_header(scanner,&the_char)) {
			if(the_char == '}') {
				offset = PTR-1;
next:
				if(SYNCTEX_NOERR == _synctex_decode_int(scanner,&n)) {
					printf("}%i\n",n);
					if(offset - n>START) {
						offset -= n;
						the_char = *offset;
						if(the_char == '{') {
							scanner->buffer_ptr=offset+1;
							if(SYNCTEX_NOERR == _synctex_decode_int(scanner,&n)) {
								printf("{%i\n",n);
								if(offset - n>START) {
									offset -= n;
									the_char = *offset;
									if(the_char == '}') {
										PTR=offset+1;
										goto next;
									} else {
										printf("This is not an '}', this is an %c\n", the_char);
									}
								} else {
									printf("The start is reached %i\n",n);
								}									
							}
						} else {
							printf("This is not an '{', this is an %c\n", the_char);
						}
					} else {
						printf("ERROR: Too long %i\n",n);
					}									
				}
				break;
			}
		}
	}
	return 0;
}
int synctex_test(void) {
	printf("sizeof(synctex_sheet_t):%i\n",sizeof(synctex_sheet_t));
	printf("sizeof(synctex_vert_box_node_t):%i\n",sizeof(synctex_vert_box_node_t));
	printf("sizeof(synctex_horiz_box_node_t):%i\n",sizeof(synctex_horiz_box_node_t));
	printf("sizeof(synctex_void_box_node_t):%i\n",sizeof(synctex_void_box_node_t));
	printf("sizeof(synctex_medium_node_t):%i\n",sizeof(synctex_medium_node_t));
	/* TESTING */
	synctex_node_t node;
	synctex_node_t child;
	synctex_node_t next;
	#define TEST(LABEL,constructor)\
	printf("TESTING constructor:%s\n",LABEL);\
	node = constructor(NULL);\
	LOG(node);\
	FREE(node);
	TEST(("_synctex_new_sheet"),_synctex_new_sheet);
	TEST(("_synctex_new_vbox"),_synctex_new_vbox);
	TEST(("_synctex_new_hbox"),_synctex_new_hbox);
	TEST(("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST(("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST(("_synctex_new_math"),_synctex_new_math);
	TEST(("_synctex_new_kern"),_synctex_new_kern);
	TEST(("_synctex_new_glue"),_synctex_new_glue);
	printf("...... ALL constructor tested\n\n\n\n");
	#undef TEST
	#define TEST(PARENT_LABEL,PARENT_CONSTRUCTOR,CHILD_LABEL,CHILD_CONSTRUCTOR)\
	printf("TESTING parent:%s -> child:%s\n",PARENT_LABEL,CHILD_LABEL);\
	node = PARENT_CONSTRUCTOR(NULL);\
	child = CHILD_CONSTRUCTOR(NULL);\
	DISPLAY(node);\
	DISPLAY(child);\
	SET_CHILD(node,child);\
	DISPLAY(node);\
	DISPLAY(child);\
	if((CHILD(node) != child) || (node != PARENT(child))) {\
		return synctex_bail();\
	}\
	FREE(node);
	TEST("_synctex_new_sheet",_synctex_new_sheet,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_sheet",_synctex_new_sheet,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_sheet",_synctex_new_sheet,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_sheet",_synctex_new_sheet,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_sheet",_synctex_new_sheet,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_sheet",_synctex_new_sheet,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_sheet",_synctex_new_sheet,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_glue"),_synctex_new_glue);
//	TEST("_synctex_new_sheet","",_synctex_new_sheet,);
//	TEST("","",,);
	#undef TEST
	#define TEST(CHILD_LABEL,CHILD_CONSTRUCTOR,SIBLING_LABEL,SIBLING_CONSTRUCTOR)\
	printf("TESTING child:%s -> next:%s\n",CHILD_LABEL,SIBLING_LABEL);\
	node = _synctex_new_sheet(NULL);\
	child = CHILD_CONSTRUCTOR(NULL);\
	next = SIBLING_CONSTRUCTOR(NULL);\
	DISPLAY(node);\
	DISPLAY(child);\
	DISPLAY(next);\
	SET_CHILD(node,child);\
	SET_SIBLING(child,next);\
	DISPLAY(node);\
	DISPLAY(child);\
	DISPLAY(next);\
	if((SIBLING(child) != next) || (node != PARENT(next))) {\
		return synctex_bail();\
	}\
	FREE(node);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_vbox",_synctex_new_vbox,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_hbox",_synctex_new_hbox,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_void_vbox",_synctex_new_void_vbox,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_void_vbox",_synctex_new_void_vbox,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_void_vbox",_synctex_new_void_vbox,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_void_vbox",_synctex_new_void_vbox,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_void_vbox",_synctex_new_void_vbox,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_void_vbox",_synctex_new_void_vbox,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_void_vbox",_synctex_new_void_vbox,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_void_hbox",_synctex_new_void_hbox,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_void_hbox",_synctex_new_void_hbox,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_void_hbox",_synctex_new_void_hbox,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_void_hbox",_synctex_new_void_hbox,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_void_hbox",_synctex_new_void_hbox,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_void_hbox",_synctex_new_void_hbox,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_void_hbox",_synctex_new_void_hbox,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_math",_synctex_new_math,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_math",_synctex_new_math,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_math",_synctex_new_math,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_math",_synctex_new_math,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_math",_synctex_new_math,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_math",_synctex_new_math,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_math",_synctex_new_math,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_kern",_synctex_new_kern,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_kern",_synctex_new_kern,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_kern",_synctex_new_kern,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_kern",_synctex_new_kern,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_kern",_synctex_new_kern,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_kern",_synctex_new_kern,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_kern",_synctex_new_kern,("_synctex_new_glue"),_synctex_new_glue);
	TEST("_synctex_new_glue",_synctex_new_glue,("_synctex_new_vbox"),_synctex_new_vbox);
	TEST("_synctex_new_glue",_synctex_new_glue,("_synctex_new_hbox"),_synctex_new_hbox);
	TEST("_synctex_new_glue",_synctex_new_glue,("_synctex_new_void_vbox"),_synctex_new_void_vbox);
	TEST("_synctex_new_glue",_synctex_new_glue,("_synctex_new_void_hbox"),_synctex_new_void_hbox);
	TEST("_synctex_new_glue",_synctex_new_glue,("_synctex_new_math"),_synctex_new_math);
	TEST("_synctex_new_glue",_synctex_new_glue,("_synctex_new_kern"),_synctex_new_kern);
	TEST("_synctex_new_glue",_synctex_new_glue,("_synctex_new_glue"),_synctex_new_glue);

	return 0;

}

