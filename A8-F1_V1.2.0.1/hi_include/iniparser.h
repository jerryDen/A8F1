
/*-------------------------------------------------------------------------*/
/**
   @file    iniparser.h
   @author  N. Devillard
   @date    Mar 2000
   @version 
   @brief   Parser for ini files.
*/
/*--------------------------------------------------------------------------*/

/*
	
	
	
	
*/
/*
	
	
	
	
使用说明：
在使用该配置库的时候，
		1.请注意不要出现相同的配置项，否则会出现一些解析错误。
		2.sec书写时，请确保‘[]’在同一行出现。
		3.sec名前除了空格符合法外，其它的一切字符都将会导至该sec及其子项不可识别。
		4.确保key与其的值在同一行，否则会出现该key不被识别的异常。
		5.在属性值中以双引号作起始并括起的值可以正确的被解析，但双引号后面的字符
		  将被忽略，引号本身也不作为字符串的一部分。若不以双引号作起始符，则整个字符串
		  将被正确的解析，包括引号本身也将作为字符串的一部分。为了避免异常，请不要使用
		  双引号，如果一定要使用，请以单引号或中文双引号代替。
		6.请不要使用诸如'\r','\n','[',']',';''"',等特殊字符，以免这些特殊的字符组合
		  出现不可预见的异常。
		7.在属性值中不要出现';'，因为';'是以注释符存在的，';'后的所有字符都将
		  被认为是注释，如果一定要使用，请以中文'；'代替。
		8. 在编写注释的时候注意不要把英文';'与中文的'；'相混，以免把注释当作正文，
		  把正文当作注释。

*/
#ifndef _INIPARSER_H_
#define _INIPARSER_H_

/*---------------------------------------------------------------------------
   								Includes
 ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <unistd.h> 
*/
/*
 * The following #include is necessary on many Unixes but not Linux.
 * It is not needed for Windows platforms.
 * Uncomment it if needed.
 */


#include "dictionary.h"
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define KVCSIZE             512         /*KEY and VALUE and COMMENT'size is 512*/


int iniparser_add_entry(
    dictionary * d,
    const char * sec,
    const char * key,
    const char * val);/*20060311 blair modify : 添加记录*/

/*-------------------------------------------------------------------------*/
/**
  @brief    Get number of sections in a dictionary
  @param    d   Dictionary to examine
  @return   int Number of sections found in dictionary

  This function returns the number of sections found in a dictionary.
  The test to recognize sections is done on the string stored in the
  dictionary: a section name is given as "section" whereas a key is
  stored as "section:key", thus the test looks for entries that do not
  contain a colon.

  This clearly fails in the case a section name contains a colon, but
  this should simply be avoided.

  This function returns -1 in case of error.
 */
/*--------------------------------------------------------------------------*/

int iniparser_getnsec(const dictionary * d);


/*-------------------------------------------------------------------------*/
/**
  @brief    Get name for section n in a dictionary.
  @param    d   Dictionary to examine
  @param    n   Section number (from 0 to nsec-1).
  @return   Pointer to char string

  This function locates the n-th section in a dictionary and returns
  its name as a pointer to a string statically allocated inside the
  dictionary. Do not free or modify the returned string!

  This function returns NULL in case of error.
 */
/*--------------------------------------------------------------------------*/

char * iniparser_getsecname(const dictionary * d, const int n);


/*-------------------------------------------------------------------------*/
/**
  @brief    Save a dictionary to a loadable ini file
  @param    d   Dictionary to dump
  @param    f   Opened file pointer to dump to
  @return   void

  This function dumps a given dictionary into a loadable ini file.
  It is Ok to specify @c stderr or @c stdout as output files.
 */
/*--------------------------------------------------------------------------*/

void iniparser_dump_ini(const dictionary * d, FILE * f);

/*-------------------------------------------------------------------------*/
/**
  @brief    Dump a dictionary to an opened file pointer.
  @param    d   Dictionary to dump.
  @param    f   Opened file pointer to dump to.
  @return   void

  This function prints out the contents of a dictionary, one element by
  line, onto the provided file pointer. It is OK to specify @c stderr
  or @c stdout as output files. This function is meant for debugging
  purposes mostly.
 */
/*--------------------------------------------------------------------------*/
void iniparser_dump(dictionary * d, FILE * f);

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, return NULL if not found
  @param    d   Dictionary to search
  @param    key Key string to look for
  @return   pointer to statically allocated character string, or NULL.

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  NULL is returned.
  The returned char pointer is pointing to a string allocated in
  the dictionary, do not free or modify it.

  This function is only provided for backwards compatibility with
  previous versions of iniparser. It is recommended to use
  iniparser_getstring() instead.
 */
/*--------------------------------------------------------------------------*/
char * iniparser_getstr(const dictionary * d, const char * key);


/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key
  @param    d       Dictionary to search
  @param    key     Key string to look for
  @param    def     Default value to return if key not found.
  @return   pointer to statically allocated character string

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the pointer passed as 'def' is returned.
  The returned char pointer is pointing to a string allocated in
  the dictionary, do not free or modify it.
 */
/*--------------------------------------------------------------------------*/
char * iniparser_getstring(const dictionary * d, const char * key, char * def);

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, convert to an int
  @param    d Dictionary to search
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   integer

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.
 */
/*--------------------------------------------------------------------------*/
int iniparser_getint(const dictionary * d, const char * key, const int notfound);

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, convert to a double
  @param    d Dictionary to search
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   double

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.
 */
/*--------------------------------------------------------------------------*/
double iniparser_getdouble(const dictionary * d, const char * key, const double notfound);

/*-------------------------------------------------------------------------*/
/**
  @brief    Get the string associated to a key, convert to a boolean
  @param    d Dictionary to search
  @param    key Key string to look for
  @param    notfound Value to return in case of error
  @return   integer

  This function queries a dictionary for a key. A key as read from an
  ini file is given as "section:key". If the key cannot be found,
  the notfound value is returned.

  A true boolean is found if one of the following is matched:

  - A string starting with 'y'
  - A string starting with 'Y'
  - A string starting with 't'
  - A string starting with 'T'
  - A string starting with '1'

  A false boolean is found if one of the following is matched:

  - A string starting with 'n'
  - A string starting with 'N'
  - A string starting with 'f'
  - A string starting with 'F'
  - A string starting with '0'

  The notfound value returned if no boolean is identified, does not
  necessarily have to be 0 or 1.
 */
/*--------------------------------------------------------------------------*/
int iniparser_getboolean(const dictionary * d, const char * key, const int notfound);


/*-------------------------------------------------------------------------*/
/**
  @brief    Set an entry in a dictionary.
  @param    ini     Dictionary to modify.
  @param    entry   Entry to modify (entry name)
  @param    val     New value to associate to the entry.
  @return   int 0 if Ok, -1 otherwise.

  If the given entry can be found in the dictionary, it is modified to
  contain the provided value. If it cannot be found, -1 is returned.
  It is Ok to set val to NULL.
 */
/*--------------------------------------------------------------------------*/

int iniparser_setstr(const dictionary * ini, const char * entry, const char * val);

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete an entry in a dictionary
  @param    ini     Dictionary to modify
  @param    entry   Entry to delete (entry name)
  @return   void

  If the given entry can be found, it is deleted from the dictionary.
 */
/*--------------------------------------------------------------------------*/
void iniparser_unset(dictionary * ini, const char * entry);

/*-------------------------------------------------------------------------*/
/**
  @brief    Finds out if a given entry exists in a dictionary
  @param    ini     Dictionary to search
  @param    entry   Name of the entry to look for
  @return   integer 1 if entry exists, 0 otherwise

  Finds out if a given entry exists in the dictionary. Since sections
  are stored as keys with NULL associated values, this is the only way
  of querying for the presence of sections in a dictionary.
 */
/*--------------------------------------------------------------------------*/
int iniparser_find_entry(const dictionary * ini, const char * entry) ;

/*-------------------------------------------------------------------------*/
/**
  @brief    Parse an ini file and return an allocated dictionary object
  @param    ininame Name of the ini file to read.
  @return   Pointer to newly allocated dictionary

  This is the parser for ini files. This function is called, providing
  the name of the file to be read. It returns a dictionary object that
  should not be accessed directly, but through accessor functions
  instead.

  The returned dictionary must be freed using iniparser_freedict().

 */
/*--------------------------------------------------------------------------*/
dictionary * iniparser_load(const char * ininame);


/*2006/03/11 blair add : save comment and space Line*/
int iniparser_add_comment(
    dictionary          *d, 
    const char          *sec, 
    const char          *key, 
    const char          *comment,
    const unsigned char place);

    
/*-------------------------------------------------------------------------*/
/**
  @brief    Free all memory associated to an ini dictionary
  @param    d Dictionary to free
  @return   void

  Free all memory associated to an ini dictionary.
  It is mandatory to call this function before the dictionary object
  gets out of the current context.
 */
/*--------------------------------------------------------------------------*/
void iniparser_freedict(dictionary * d);

int iniparser_sec_getNLowLever(const dictionary * d,const char* UperLever);
char* iniparser_sec_getLowLever(const dictionary * d,const char* UpLever, int n);
void iniparser_mem_printf(const dictionary * d);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

