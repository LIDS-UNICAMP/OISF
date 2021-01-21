/*  
    FILE: UAIArgs.h
    
    This file is a single-header library for parsing command line arguments, 
    which covers up most necessities found in standalone applications. It is
    possible to split this header in a .c and a .h file by "cutting" the file, 
    commenting the necessary #define's and uncommenting the necessary #include.
    In order to maintain decoupled, simple and portable, the UAIArgs library 
    will only be modified for bug corrections or major needs which were not 
    considered before.

    The arguments are expected to be defined as "--<label>[=<value>]" in which
    <label> and <value> are strings composed of alphanumeric characters. 
    Unknown parameters (thus, unknown tokens and/or preffixes) are simply 
    ignored.  Note that "--" and "=" are special and, therefore, their 
    existence within <label> or <value> may lead to undefined behavior. 
  
    DATE: Nov 2020 (v1.0.0)

    AUTHOR: Felipe Belem
    EMAIL: felipe.belem@ic.unicamp.br
    LICENSE: Public Domain (Unlicensed)
*/

#ifndef UAI_ARGS_HEADER
#define UAI_ARGS_HEADER

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

/*
  Checks if the argument defined in <token> exists in the set of arguments.

  argc - Number of parameters
  argv - Array of strings/arguments given by the user through the cli
  token - Specific argument to be found

  ATTENTION:
    - <token> must NOT contain the prefix "--" nor the attribution char '='.
    - It does NOT check if <token> has a value defined
    - It returns 1, if it exists; 0, otherwise.
*/
int UAIArgsExists
(const int argc, const char **argv, const char *token);

/*
  Gets the string value associated with the argument, defined in <token>,
  within the set of arguments.

  argc - Number of parameters
  argv - Array of strings/arguments given by the user through the cli
  token - Specific argument to be found

  ATTENTION:
    - <token> must NOT contain the prefix "--" nor the attribution char '='.
    - It does NOT copy the <token>'s value, only REFERENCES it
    - If <token> do not exists in <argv>, it returns NULL
*/
const char *UAIArgsGet
(const int argc, const char **argv, const char *token);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //UAI_ARGS_HEADER

/*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*\
8< --------------------------------- CUT HERE ---------------------------------
\*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

//#include "UAIArgs.h" // <- Uncomment for splitting in .c/.h files
#ifdef UAI_ARGS_SOURCE // <- Comment for splitting in .c/.h files

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define _UAI_ARGS_PREFIX_ "--" // Parameter prefix
#define _UAI_ARGS_ATTRIB_ '=' // Parameter attribution character
#define _UAI_ARGS_EMPTY_ '\0' // NULL char

int UAIArgsExists
(const int argc, const char **argv, const char *token)
{
  int exists;

  exists = 0;
  //-------------------------------------------------------------------------//
  if(argc <= 0) goto exit;
  else if(argv == NULL || *argv == NULL) goto exit;
  else if(token == NULL) goto exit;
  //-------------------------------------------------------------------------//
  const int PREFIX_SIZE = strlen(_UAI_ARGS_PREFIX_);
  int i, token_size;

  token_size = strlen(token);

  i = 0; 
  while(i < argc && !exists)
  {
    int cmp, is_attrib, is_empty;

    cmp = strncmp(argv[i], _UAI_ARGS_PREFIX_, PREFIX_SIZE * sizeof(char));

    if(cmp == 0) // Has prefix
    {
      cmp = strncmp(&(argv[i][PREFIX_SIZE]), token, token_size * sizeof(char));

      is_attrib = argv[i][token_size + PREFIX_SIZE] == _UAI_ARGS_ATTRIB_;
      is_empty = argv[i][token_size + PREFIX_SIZE] == _UAI_ARGS_EMPTY_;

      if(cmp == 0 && (is_attrib || is_empty)) exists = 1; // Has token
    }
    ++i;
  }

  //-------------------------------------------------------------------------//
  exit:
  return exists;
}

const char *UAIArgsGet
(const int argc, const char **argv, const char *token)
{
  const char *STR_VAL = NULL;

  //-------------------------------------------------------------------------//
  if(argc <= 0) goto exit;
  else if(argv == NULL || *argv == NULL) goto exit;
  else if(token == NULL) goto exit;
  //-------------------------------------------------------------------------//
  const int PREFIX_SIZE = strlen(_UAI_ARGS_PREFIX_);
  int i, token_size, exists;

  token_size = strlen(token);

  i = 0; exists = 0;
  while(i < argc && !exists)
  {
    int cmp;

    cmp = strncmp(argv[i], _UAI_ARGS_PREFIX_, PREFIX_SIZE * sizeof(char));

    if(cmp == 0) // Has prefix
    {
      int is_attrib, is_empty;

      cmp = strncmp(&(argv[i][PREFIX_SIZE]), token, token_size * sizeof(char));

      is_attrib = argv[i][token_size + PREFIX_SIZE] == _UAI_ARGS_ATTRIB_;
      is_empty = argv[i][token_size + PREFIX_SIZE] == _UAI_ARGS_EMPTY_;

      if(cmp == 0 && is_attrib && !is_empty) // Has token and attr sign
      {
        int diff_size;

        exists = 1;
        diff_size = strlen(argv[i]) - (PREFIX_SIZE + token_size + 1);
        
        if(diff_size > 0) // Has non-empty value
          STR_VAL = &(argv[i][PREFIX_SIZE + token_size + 1]);
      }
    }
    ++i;
  }

  //-------------------------------------------------------------------------//
  exit:
  return STR_VAL;
}

#undef UAI_ARGS_SOURCE // <- Comment for splitting in .c/.h files
#endif // UAI_ARGS_SOURCE // <- Comment for splitting in .c/.h files