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

Acknowledgments:
----------------
The author received useful remarks from the pdfTeX developers, especially Hahn The Thanh,
and significant help from XeTeX developer Jonathan Kew

Nota Bene:
----------
If you include or use a significant part of the synctex package into a software,
I would appreciate to be listed as contributor and see "SyncTeX" highlighted.

Version 1
Thu Jun 19 09:39:21 UTC 2008

*/

#ifndef __SYNCTEX_PARSER__
#   define __SYNCTEX_PARSER__

/* synctex_node_t is the type for all synctex nodes.
 * The synctex file is parsed into a tree of nodes, either sheet, boxes, math nodes... */
typedef struct _synctex_node * synctex_node_t;

/* The main synctex object is a scanner
 * Its implementation is considered private.
 * The basic workflow is
 * - create a scanner with a file
 * - perform actions on that scanner
 * - free the scanner
 */
typedef struct __synctex_scanner_t _synctex_scanner_t;
typedef _synctex_scanner_t * synctex_scanner_t;

#ifdef __cplusplus
extern "C" {
#endif

/* This is the designated method to create a new synctex scanner object.
 * output is the pdf/dvi/xdv file associated to the synctex file.
 * If relevant, it can be the tex file that originated the synctex file
 * but it might cause problems if the \jobname has a custom value.
 * The last file extension is removed and replaced by the proper extension.
 * Then the private method synctex_scanner_new_with_contents_of_file is called.
 * NULL is returned in case of an error or non existent file.
 */
synctex_scanner_t synctex_scanner_new_with_output_file(const char * output);

/* This is the designated method to delete a synctex scanner object.
 * Frees all the memory.
 */
void synctex_scanner_free(synctex_scanner_t scanner);

/* Display all the information contained in the scanner object.
 * If the records are too numerous, only the first ones are displayed.
 * This is mainly for informatinal purpose to help developers.
 */
void synctex_scanner_display(synctex_scanner_t scanner);

/* The x and y offset of the origin in TeX coordinates. The magnification
   These are used by pdf viewers that want to display the real box size.
   For example, getting the horizontal coordinates of a node would require
   synctex_node_box_h(node)*synctex_scanner_magnification(scanner)+synctex_scanner_x_offset(scanner)
   Getting its TeX width would simply require
   synctex_node_box_width(node)*synctex_scanner_magnification(scanner)
   but direct methods are available for that below.
 */
int synctex_scanner_x_offset(synctex_scanner_t scanner);
int synctex_scanner_y_offset(synctex_scanner_t scanner);
float synctex_scanner_magnification(synctex_scanner_t scanner);

/* Managing the input file names.
 * Given a tag, synctex_scanner_get_name will return the corresponding file name.
 * Conversely, given a file name, synctex_scanner_get_tag will retur, the corresponding tag.
 * The file name must be the very same as understood by TeX.
 * For example, if you \input myDir/foo.tex, the file name is myDir/foo.tex.
 * No automatic path expansion is performed.
 * Finally, synctex_scanner_input is the first input node of the scanner.
 * To browse all the input node, use a loop like
 *
 *     if((input_node = synctex_scanner_input(scanner))){
 *         do {
 *             blah
 *         } while((input_node=synctex_node_sibling(input_node)));
 *     }
 *
 * The output is the name that was used to create the scanner.
 * The synctex is the real name of the synctex file,
 * it was obtained from output by setting the proper file extension.
 */
const char * synctex_scanner_get_name(synctex_scanner_t scanner,int tag);
int synctex_scanner_get_tag(synctex_scanner_t scanner,const char * name);
synctex_node_t synctex_scanner_input(synctex_scanner_t scanner);
const char * synctex_scanner_get_output(synctex_scanner_t scanner);
const char * synctex_scanner_get_synctex(synctex_scanner_t scanner);

/* Browsing the nodes
 * parent, child and sibling are standard names for tree nodes.
 * The parent is one level higher, the child is one level deeper,
 * and the sibling is at the same level.
 * The sheet of a node is the first ancestor, it is of type sheet.
 * A node and its sibling have the same parent.
 * A node is the parent of its child.
 * A node is either the child of its parent,
 * or belongs to the sibling chain of its parent's child.
 * The next node is either the child, the sibling or the parent's sibling,
 * unless the parent is a sheet.
 * This allows to navigate through all the nodes of a given sheet node:
 *
 *     synctex_node_t node = sheet;
 *     while((node = synctex_node_next(node))) {
 *         // do something with node
 *     }
 *
 * With synctex_sheet_content, you can retrieve the sheet node given the page.
 * The page is 1 based, according to TeX standards.
 * Conversely synctex_node_sheet allows to retrieve the sheet containing a given node.
 */
synctex_node_t synctex_node_parent(synctex_node_t node);
synctex_node_t synctex_node_sheet(synctex_node_t node);
synctex_node_t synctex_node_child(synctex_node_t node);
synctex_node_t synctex_node_sibling(synctex_node_t node);
synctex_node_t synctex_node_next(synctex_node_t node);
synctex_node_t synctex_sheet_content(synctex_scanner_t scanner,int page);

/* These are the types of the synctex nodes */
typedef enum {
	synctex_node_type_error = 0,
	synctex_node_type_sheet,
	synctex_node_type_vbox,
	synctex_node_type_void_vbox,
	synctex_node_type_hbox,
	synctex_node_type_void_hbox,
	synctex_node_type_hbox_non_void,
	synctex_node_type_kern,
	synctex_node_type_glue,
	synctex_node_type_math,
	synctex_node_type_input,
	synctex_node_number_of_types
} synctex_node_type_t;

/* synctex_node_type gives the type of a given node,
 * synctex_node_isa gives the same information as a human readable text. */
synctex_node_type_t synctex_node_type(synctex_node_t node);
const char * synctex_node_isa(synctex_node_t node);

/* This is primarily used for debugging purpose.
 * The second one logs information for the node and recursively displays information for its next node */
void synctex_node_log(synctex_node_t node);
void synctex_node_display(synctex_node_t node);

/* Given a node, access to its tag, line and column.
 * The line and column numbers are 1 based.
 * The latter is not yet fully supported in TeX, the default implementation returns 0 which means the whole line.
 * When the tag is known, the scanner of the node will give the corresponding file name.
 * When the tag is known, the scanner of the node will give the name.
 */
int synctex_node_tag(synctex_node_t node);
int synctex_node_line(synctex_node_t node);
int synctex_node_column(synctex_node_t node);

/* This is the page where the node appears.
 * This is a 1 based index as given by TeX.
 */
int synctex_node_page(synctex_node_t node);

/* For quite all nodes, horizontal, vertical coordinates, and width.
 * These are expressed in page coordinates, with origin at the top left corner.
 */
float synctex_node_h(synctex_node_t node);
float synctex_node_v(synctex_node_t node);
float synctex_node_width(synctex_node_t node);

/* For all nodes, dimensions of the enclosing box.
 * These are expressed in page coordinates, with origin at the top left corner.
 * A box is enclosing itself.
 */
float synctex_node_box_h(synctex_node_t node);
float synctex_node_box_v(synctex_node_t node);
float synctex_node_box_width(synctex_node_t node);
float synctex_node_box_height(synctex_node_t node);
float synctex_node_box_depth(synctex_node_t node);

/* For quite all nodes, horizontal, vertical coordinates, and width.
 * The visible dimensions are bigger than real ones to compensate 0 width boxes
 * that do contain nodes.
 * These are expressed in page coordinates, with origin at the top left corner.
 * A box is enclosing itself.
 */
float synctex_node_visible_h(synctex_node_t node);
float synctex_node_visible_v(synctex_node_t node);
float synctex_node_visible_width(synctex_node_t node);
/* For all nodes, visible dimensions of the enclosing box.
 * A box is enclosing itself.
 * The visible dimensions are bigger than real ones to compensate 0 width boxes
 * that do contain nodes.
 */
float synctex_node_box_visible_h(synctex_node_t node);
float synctex_node_box_visible_v(synctex_node_t node);
float synctex_node_box_visible_width(synctex_node_t node);
float synctex_node_box_visible_height(synctex_node_t node);
float synctex_node_box_visible_depth(synctex_node_t node);

/* The main entry points.
 * Given the file name, a line and a column number, synctex_display_query returns the number of nodes
 * satisfying the contrain. Use code like
 *
 *     if(synctex_display_query(scanner,name,line,column)>0) {
 *         synctex_node_t node;
 *         while((node = synctex_next_result(scanner))) {
 *             // do something with node
 *             ...
 *         }
 *     }
 *
 * Given the page and the position in the page, synctex_edit_query returns the number of nodes
 * satisfying the contrain. Use code like
 *
 *     if(synctex_edit_query(scanner,page,h,v)>0) {
 *         synctex_node_t node;
 *         while(node = synctex_next_result(scanner)) {
 *             // do something with node
 *             ...
 *         }
 *     }
 *
 * page is 1 based
 * h and v are coordinates relative to the top left corner of the page.
 * If you make a new query, the result of the previous one is discarded.
 * If one of this function returns a non positive integer,
 * it means that an error occurred.
 *
 * Both methods are conservative, in the sense that matching is weak.
 * If the exact column number is not found, there will be an answer with the whole line.
 */
int synctex_display_query(synctex_scanner_t scanner,const char * name,int line,int column);
int synctex_edit_query(synctex_scanner_t scanner,int page,float h,float v);
synctex_node_t synctex_next_result(synctex_scanner_t scanner);

/* The main synctex updater object.
 * This object is used to append information to the synctex file.
 * Its implementation is considered private.
 * It is used by the synctex command line tool to take into account modifications
 * that could occur while postprocessing files by dvipdf like filters.
 */
typedef struct __synctex_updater_t _synctex_updater_t;
typedef _synctex_updater_t * synctex_updater_t;

/* Designated initializer.
 * Once you are done with your whole job,
 * free the updater */
synctex_updater_t synctex_updater_new_with_output_file(const char * output);

/* Use the next functions to append records to the synctex file,
 * no consistency tests made on the arguments */
void synctex_updater_append_magnification(synctex_updater_t updater, char * magnification);
void synctex_updater_append_x_offset(synctex_updater_t updater, char * x_offset);
void synctex_updater_append_y_offset(synctex_updater_t updater, char * y_offset);

/* You MUST free the updater, once everything is properly appended */
void synctex_updater_free(synctex_updater_t updater);

#ifdef __cplusplus
};
#endif

#endif
