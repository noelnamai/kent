/* annoFormatTab -- collect fields from all inputs and print them out, tab-separated. */

/* Copyright (C) 2013 The Regents of the University of California 
 * See README in this or parent directory for licensing information. */

#include "annoFormatTab.h"
#include "annoGratorQuery.h"
#include "dystring.h"

struct annoFormatTab
    {
    struct annoFormatter formatter;     // External interface
    char *fileName;                     // Output file name, can be "stdout"
    FILE *f;                            // Output file handle
    struct hash *columnVis;             // Hash of columns that have been explicitly selected
                                        // or deselected by user.
    boolean needHeader;			// TRUE if we should print out the header
    };

static void makeFullColumnName(char *fullName, size_t size, char *sourceName, char *colName)
/* If sourceName is non-empty, make fullName sourceName.colName, otherwise just colName. */
{
if (isNotEmpty(sourceName))
    safef(fullName, size, "%s.%s", sourceName, colName);
else
    safecpy(fullName, size, colName);
}

void annoFormatTabSetColumnVis(struct annoFormatter *vSelf, char *sourceName, char *colName,
                               boolean enabled)
/* Explicitly include or exclude column in output.  sourceName must be the same
 * as the corresponding annoStreamer source's name. */
{
struct annoFormatTab *self = (struct annoFormatTab *)vSelf;
if (! self->columnVis)
    self->columnVis = hashNew(0);
char fullName[PATH_LEN];
makeFullColumnName(fullName, sizeof(fullName), sourceName, colName);
hashAddInt(self->columnVis, fullName, enabled);
}

static boolean columnIsIncluded(struct annoFormatTab *self, char *sourceName, char *colName)
// Return TRUE if column has not been explicitly deselected.
{
if (self->columnVis)
    {
    char fullName[PATH_LEN];
    makeFullColumnName(fullName, sizeof(fullName), sourceName, colName);
    int vis = hashIntValDefault(self->columnVis, fullName, 1);
    if (vis == 0)
        return FALSE;
    }
return TRUE;
}

static void printHeaderColumns(struct annoFormatTab *self, struct annoStreamer *source,
                               boolean isFirst)
/* Print names of included columns from this source. */
{
FILE *f = self->f;
char *sourceName = source->name;
char fullName[PATH_LEN];
if (source->rowType == arWig)
    {
    // Fudge in the row's chrom, start, end as output columns even though they're not in autoSql
    if (isFirst)
	{
        makeFullColumnName(fullName, sizeof(fullName), sourceName, "chrom");
        fprintf(f, "#%s", fullName);
	isFirst = FALSE;
	}
    makeFullColumnName(fullName, sizeof(fullName), sourceName, "start");
    fprintf(f, "\t%s", fullName);
    makeFullColumnName(fullName, sizeof(fullName), sourceName, "end");
    fprintf(f, "\t%s", fullName);
    }
struct asColumn *col;
int i;
for (col = source->asObj->columnList, i = 0;  col != NULL;  col = col->next, i++)
    {
    if (columnIsIncluded(self, sourceName, col->name))
        {
        if (isFirst)
            {
            fputc('#', f);
            isFirst = FALSE;
            }
        else
            fputc('\t', f);
        makeFullColumnName(fullName, sizeof(fullName), sourceName, col->name);
        fputs(fullName, f);
        }
    }
}

static void aftInitialize(struct annoFormatter *vSelf, struct annoStreamer *primary,
			  struct annoStreamer *integrators)
/* Print header, regardless of whether we get any data after this. */
{
struct annoFormatTab *self = (struct annoFormatTab *)vSelf;
if (self->needHeader)
    {
    char *primaryHeader = primary->getHeader(primary);
    if (isNotEmpty(primaryHeader))
	fprintf(self->f, "# Header from primary input:\n%s", primaryHeader);
    printHeaderColumns(self, primary, TRUE);
    struct annoStreamer *grator;
    for (grator = integrators;  grator != NULL;  grator = grator->next)
	printHeaderColumns(self, grator, FALSE);
    fputc('\n', self->f);
    self->needHeader = FALSE;
    }
}

static double wigRowAvg(struct annoRow *row)
/* Return the average value of floats in row->data. */
{
float *vector = row->data;
int len = row->end - row->start;
double sum = 0.0;
int i;
for (i = 0;  i < len;  i++)
    sum += vector[i];
return sum / (double)len;
}

static char **wordsFromWigRowAvg(struct annoRow *row)
/* Return an array of strings with a single string containing the average of values in row. */
{
double avg = wigRowAvg(row);
char **words;
AllocArray(words, 1);
char avgStr[32];
safef(avgStr, sizeof(avgStr), "%lf", avg);
words[0] = cloneString(avgStr);
return words;
}

static char **wordsFromWigRowVals(struct annoRow *row)
/* Return an array of strings with a single string containing comma-sep per-base wiggle values. */
{
float *vector = row->data;
int len = row->end - row->start;
struct dyString *dy = dyStringNew(10*len);
int i;
for (i = 0;  i < len;  i++)
    dyStringPrintf(dy, "%g,", vector[i]);
char **words;
AllocArray(words, 1);
words[0] = dyStringCannibalize(&dy);
return words;
}

static char **wordsFromRow(struct annoRow *row, struct annoStreamer *source,
			   boolean *retFreeWhenDone)
/* If source->rowType is arWords, return its words.  Otherwise translate row->data into words. */
{
if (row == NULL)
    return NULL;
boolean freeWhenDone = FALSE;
char **words = NULL;
if (source->rowType == arWords)
    words = row->data;
else if (source->rowType == arWig)
    {
    freeWhenDone = TRUE;
    //#*** config options: avg? more stats? list of values?
    boolean doAvg = FALSE;
    if (doAvg)
	words = wordsFromWigRowAvg(row);
    else
	words = wordsFromWigRowVals(row);
    }
else
    errAbort("annoFormatTab: unrecognized row type %d from source %s",
	     source->rowType, source->name);
if (retFreeWhenDone != NULL)
    *retFreeWhenDone = freeWhenDone;
return words;
}

static void printColumns(struct annoFormatTab *self, struct annoStreamer *streamer,
			 struct annoRow *row, boolean isFirst)
/* Print columns in streamer's row (if NULL, print the right number of empty fields). */
{
FILE *f = self->f;
char *sourceName = streamer->name;
boolean freeWhenDone = FALSE;
char **words = wordsFromRow(row, streamer, &freeWhenDone);
if (streamer->rowType == arWig)
    {
    // Fudge in the row's chrom, start, end as output columns even though they're not in autoSql
    if (isFirst)
	{
	if (row != NULL)
	    fputs(row->chrom, f);
	isFirst = FALSE;
	}
    if (row != NULL)
	fprintf(f, "\t%u\t%u", row->start, row->end);
    else
	fputs("\t\t", f);
    }
struct asColumn *col;
int i;
for (col = streamer->asObj->columnList, i = 0;  col != NULL;  col = col->next, i++)
    {
    if (columnIsIncluded(self, sourceName, col->name))
        {
        if (isFirst)
            isFirst = FALSE;
        else
            fputc('\t', f);
        if (words != NULL)
            fputs((words[i] ? words[i] : ""), f);
        }
    }
if (freeWhenDone)
    {
    freeMem(words[0]);
    freeMem(words);
    }
}

static void aftComment(struct annoFormatter *fSelf, char *content)
/* Print out a comment line. */
{
if (strchr(content, '\n'))
    errAbort("aftComment: no multi-line input");
struct annoFormatTab *self = (struct annoFormatTab *)fSelf;
fprintf(self->f, "# %s\n", content);
}

static void aftFormatOne(struct annoFormatter *vSelf, struct annoStreamRows *primaryData,
			 struct annoStreamRows *gratorData, int gratorCount)
/* Print out tab-separated columns that we have gathered in prior calls to aftCollect,
 * and start over fresh for the next line of output. */
{
struct annoFormatTab *self = (struct annoFormatTab *)vSelf;
// Got one row from primary; what's the largest # of rows from any grator?
int maxRows = 1;
int iG;
for (iG = 0;  iG < gratorCount;  iG++)
    {
    int gratorRowCount = slCount(gratorData[iG].rowList);
    if (gratorRowCount > maxRows)
	maxRows = gratorRowCount;
    }
// Print out enough rows to make sure that all grator rows are included.
int iR;
for (iR = 0;  iR < maxRows;  iR++)
    {
    printColumns(self, primaryData->streamer, primaryData->rowList, TRUE);
    for (iG = 0;  iG < gratorCount;  iG++)
	{
	struct annoRow *gratorRow = slElementFromIx(gratorData[iG].rowList, iR);
	printColumns(self, gratorData[iG].streamer, gratorRow, FALSE);
	}
    fputc('\n', self->f);
    }
}

static void aftClose(struct annoFormatter **pVSelf)
/* Close file handle, free self. */
{
if (pVSelf == NULL)
    return;
struct annoFormatTab *self = *(struct annoFormatTab **)pVSelf;
freeMem(self->fileName);
carefulClose(&(self->f));
annoFormatterFree(pVSelf);
}

struct annoFormatter *annoFormatTabNew(char *fileName)
/* Return a formatter that will write its tab-separated output to fileName. */
{
struct annoFormatTab *aft;
AllocVar(aft);
struct annoFormatter *formatter = &(aft->formatter);
formatter->getOptions = annoFormatterGetOptions;
formatter->setOptions = annoFormatterSetOptions;
formatter->initialize = aftInitialize;
formatter->comment = aftComment;
formatter->formatOne = aftFormatOne;
formatter->close = aftClose;
aft->fileName = cloneString(fileName);
aft->f = mustOpen(fileName, "w");
aft->needHeader = TRUE;
return (struct annoFormatter *)aft;
}
