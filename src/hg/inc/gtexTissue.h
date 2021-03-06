/* gtexTissue.h was originally generated by the autoSql program, which also 
 * generated gtexTissue.c and gtexTissue.sql.  This header links the database and
 * the RAM representation of objects. */

#ifndef GTEXTISSUE_H
#define GTEXTISSUE_H

#define GTEXTISSUE_NUM_COLS 4

extern char *gtexTissueCommaSepFieldNames;

struct gtexTissue
/* GTEx tissue information */
    {
    struct gtexTissue *next;  /* Next in singly linked list. */
    unsigned id;	/* internal id */
    char *name;	/* short UCSC identifier */
    char *description;	/* GTEX tissue type detail */
    char *organ;	/* GTEX tissue collection area */
    };

void gtexTissueStaticLoad(char **row, struct gtexTissue *ret);
/* Load a row from gtexTissue table into ret.  The contents of ret will
 * be replaced at the next call to this function. */

struct gtexTissue *gtexTissueLoad(char **row);
/* Load a gtexTissue from row fetched with select * from gtexTissue
 * from database.  Dispose of this with gtexTissueFree(). */

struct gtexTissue *gtexTissueLoadAll(char *fileName);
/* Load all gtexTissue from whitespace-separated file.
 * Dispose of this with gtexTissueFreeList(). */

struct gtexTissue *gtexTissueLoadAllByChar(char *fileName, char chopper);
/* Load all gtexTissue from chopper separated file.
 * Dispose of this with gtexTissueFreeList(). */

#define gtexTissueLoadAllByTab(a) gtexTissueLoadAllByChar(a, '\t');
/* Load all gtexTissue from tab separated file.
 * Dispose of this with gtexTissueFreeList(). */

struct gtexTissue *gtexTissueCommaIn(char **pS, struct gtexTissue *ret);
/* Create a gtexTissue out of a comma separated string. 
 * This will fill in ret if non-null, otherwise will
 * return a new gtexTissue */

void gtexTissueFree(struct gtexTissue **pEl);
/* Free a single dynamically allocated gtexTissue such as created
 * with gtexTissueLoad(). */

void gtexTissueFreeList(struct gtexTissue **pList);
/* Free a list of dynamically allocated gtexTissue's */

void gtexTissueOutput(struct gtexTissue *el, FILE *f, char sep, char lastSep);
/* Print out gtexTissue.  Separate fields with sep. Follow last field with lastSep. */

#define gtexTissueTabOut(el,f) gtexTissueOutput(el,f,'\t','\n');
/* Print out gtexTissue as a line in a tab-separated file. */

#define gtexTissueCommaOut(el,f) gtexTissueOutput(el,f,',',',');
/* Print out gtexTissue as a comma separated list including final comma. */

/* -------------------------------- End autoSql Generated Code -------------------------------- */

void gtexTissueCreateTable(struct sqlConnection *conn, char *table);
/* Create expression record format table of given name. */

#endif /* GTEXTISSUE_H */

