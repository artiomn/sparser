//------------------------------------------------------------------------------
// CHANGED by Artiom N..
// Handler is call, when section appears.
//------------------------------------------------------------------------------

/* inih -- simple .INI file parser

inih is released under the New BSD license (see LICENSE.txt). Go to the project
home page for more info:

http://code.google.com/p/inih/

*/

//------------------------------------------------------------------------------
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "ini.h"
#include "utils.h"

#define INI_MAX_LINE 512
#define INI_MAX_SECTION 50
#define INI_MAX_NAME 50

//------------------------------------------------------------------------------
/* Return pointer to first char c or ';' comment in given string, or pointer to
   null at end of string if neither found. ';' must be prefixed by a whitespace
   character to register as a comment. */
static char* find_char_or_comment(const char* s, char c)
{
   int was_whitespace = 0;

   assert(s != NULL);

   while (*s && *s != c && !(was_whitespace && *s == ';'))
   {
       was_whitespace = isspace(*s);
       s++;
   }
   return((char*)s);
}
//------------------------------------------------------------------------------
/* See documentation in header file. */
int ini_parse_file(FILE* file,
                   int (*handler)(void*, const char*, const char*,
                                  const char*),
                   void* user)
{
    /* Uses a fair bit of stack (use heap instead if you need to) */
   char  line[INI_MAX_LINE];
   char  section[INI_MAX_SECTION] = "";
   char  prev_name[INI_MAX_NAME] = "";

   char* start;
   char* end;
   char* name;
   char* value;
   int   lineno = 0;
   int   error = 0;

   assert(file    != NULL);
   assert(handler != NULL);

    /* Scan through file line by line */
   while (fgets(line, sizeof(line), file) != NULL)
   {
       lineno++;
       start = skip_spaces(rstrip(line));

       if (*start == ';' || *start == '#')
       {
           /* Per Python ConfigParser, allow '#' comments at start of line */
       }
#if INI_ALLOW_MULTILINE
       else if (*prev_name && *start && start > line)
       {
           /* Non-black line with leading whitespace, treat as continuation
              of previous name's value (as per Python ConfigParser). */
           if (!handler(user, section, prev_name, start) && !error)
               error = lineno;
       }
#endif
       else if (*start == '[')
       {
           /* A "[section]" line */
           end = find_char_or_comment(start + 1, ']');
           if (*end == ']')
           {
               *end = '\0';
               strncpy0(section, start + 1, sizeof(section));
               *prev_name = '\0';
               if (!handler(user, section, NULL, NULL) && !error)
                   error = lineno;
           }
           else if (!error)
           {
               /* No ']' found on section line */
               error = lineno;
           }
       }
       else if (*start && *start != ';')
       {
           /* Not a comment, must be a name[=:]value pair */
           end = find_char_or_comment(start, '=');
           if (*end != '=')
           {
              end = find_char_or_comment(start, ':');
           }
           if (*end == '=' || *end == ':')
           {
               *end = '\0';
               name = rstrip(start);
               value = skip_spaces(end + 1);
               end = find_char_or_comment(value, '\0');
               if (*end == ';') *end = '\0';
               rstrip(value);

               /* Valid name[=:]value pair found, call handler */
               strncpy0(prev_name, name, sizeof(prev_name));
               if (!handler(user, section, name, value) && !error)
                   error = lineno;
           }
           else if (!error)
           {
               /* No '=' or ':' found on name[=:]value line */
               error = lineno;
           }
       }
   }

   return(error);
}
//------------------------------------------------------------------------------
/* See documentation in header file. */
int ini_parse(const char* filename,
              int (*handler)(void*, const char*, const char*, const char*),
              void* user)
{
    FILE*   file;
    int     error;

    assert(filename != NULL);

    file = fopen(filename, "r");
    if (!file) return(-1);
    error = ini_parse_file(file, handler, user);
    fclose(file);
    return error;
}
//------------------------------------------------------------------------------
