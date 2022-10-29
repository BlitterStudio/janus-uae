
/*

    _____ ____  __       ______            _____
   / ___// __ \/ /      / ____/___  ____  / __(_)___ _
   \__ \/ / / / /      / /   / __ \/ __ \/ /_/ / __  /
  ___/ / /_/ / /___   / /___/ /_/ / / / / __/ / /_/ /
 /____/_____/_____/   \____/\____/_/ /_/_/ /_/\__, /
                                             /____/


 Library for reading and writing configuration files in a easy, cross-platform way.

 - Author: Hubert "Koshmaar" Rutkowski
 - Contact: koshmaar@poczta.onet.pl
 - Version: 1.2
 - Homepage: http://sdl-cfg.sourceforge.net/
 - License: GNU LGPL

*/

#ifndef _SDL_CONFIG_AUTO_TESTS_H
#define _SDL_CONFIG_AUTO_TESTS_H

#define Abs(x) ( ((x) > 0) ? (x) : -(x))

/*

 File: SDL_Config automated test defines

 They don't form part of SDL_Config API, but are used by test programs to automatically
 determine whether SDL_Config contains bugs in tested functions. It should be included after SDL_config.h.
 
 _It's not linked along with SDL_Config library_!

*/

/* ------------------------------------------------------- */
/*                     Auto tests API                      */
/* ------------------------------------------------------- */


int tests_passed = 0;
int total_tests = 0;

/*

  Define: CFG_WRITE_HEADER

  Outputs initial "greetings text".

*/

#ifdef CFG_COMPILER_VISUALC /* VisualC++ */
 #define CFG_WRITE_HEADER(x) fprintf(stderr, "\n <--- SDL_Config (%u.%u.%u) test #"#x" (VisualC++) --->\n\n" \
                                                  "           Compilation time: "__TIME__"\n\n " \
                                                  "    <------------------------------------>\n", \
                                                  SDL_CFG_MAJOR_VERSION, SDL_CFG_MINOR_VERSION, \
                                                  SDL_CFG_PATCHLEVEL);
#else /* probably GCC */
 #define CFG_WRITE_HEADER(x) fprintf(stderr, "\n <--- SDL_Config (%u.%u.%u) test #"#x" (GCC) --->\n\n" \
                                                  "         Compilation time: "__TIME__"\n\n " \
                                                  "   <---------------------------------->\n", \
                                                  SDL_CFG_MAJOR_VERSION, SDL_CFG_MINOR_VERSION, \
                                                  SDL_CFG_PATCHLEVEL);
#endif

/*
  Define: CFG_BEGIN_AUTO_TEST

  Marks the start of automated test procedure.

*/

#define CFG_BEGIN_AUTO_TEST  fprintf(stderr, "\n\n --------------------------------\n| BEGIN AUTOMATED TEST PROCEDURE |"); \
                            fprintf(stderr, "\n --------------------------------\n\n");


/*
  Define: CFG_AUTO_TEST_HEADER

  Writes the header.

*/

#define CFG_AUTO_TEST_HEADER(x) fprintf(stderr, "\n\n --------------------------------\n| " x " |"); \
                                fprintf(stderr, "\n --------------------------------\n\n");


/*
  Define: CFG_AUTO_TEST_BOOL

  Tests two booleans, if they are different, outputs what error occured.

*/

#define CFG_AUTO_TEST_BOOL(x, y) {  if (x != y) \
                                    fprintf(stderr, "\n!!!\n! Error found in line: %i: bools: %i" \
                                    " and %i are different!\n!!!\n\n", __LINE__, x, y); \
                                    else { fprintf(stderr, "- OK, test in line %i for bools passed...\n\n", __LINE__); \
                                    ++tests_passed; } ++total_tests; }

/*
  Define: CFG_AUTO_TEST_INTEGER

  Tests two integers, if they are different, outputs what error occured.

*/

#define CFG_AUTO_TEST_INTEGER(x, y) { if (x != y) \
                                    fprintf(stderr, "\n!!!\n! Error found in line: %i: integers: %i" \
                                    " and %i are different!\n!!!\n\n", __LINE__, x, y); \
                                    else { fprintf(stderr, "- OK, test in line %i for integers passed...\n\n", __LINE__); \
                                    ++tests_passed; } ++total_tests; }


/*
  Define: CFG_AUTO_TEST_FLOAT

  Tests two floats, if they are different, outputs what error occured.

*/

#define CFG_AUTO_TEST_FLOAT(x, y)   { if (Abs(x - y) > 0.000001f ) \
                                    fprintf(stderr, "\n!!!\n! Error found in line: %i: floats: %f" \
                                    " and %f are different!\n!!!\n\n", __LINE__, x, y); \
                                    else { fprintf(stderr, "- OK, test in line %i for floats passed...\n\n", __LINE__); \
                                    ++tests_passed; } ++total_tests; }

/*
  Define: CFG_AUTO_TEST_TEXT

  Tests two strings, if they are different, outputs what error occured.

*/

#define CFG_AUTO_TEST_TEXT(x, y)    { if (strcmp(x, y)) \
                                    fprintf(stderr, "\n!!!\n! Error found in line: %i: strings: \"%s\"" \
                                    " and \"%s\" are different!\n!!!\n\n", __LINE__, x, y); \
                                    else { fprintf(stderr, "- OK, test in line %i for strings passed...\n\n", __LINE__); \
                                    ++tests_passed; } ++total_tests; }

/*
  Define: CFG_LOG_OTHER_ERROR

  No testing, just outputs what was passed in.

*/

#define CFG_LOG_OTHER_ERROR(x)      { fprintf(stderr, "\n!!!\n! Error found in line: %i, description: %s\n SDL_GetError: %s"\
                                                      "\n\n", __LINE__, x, SDL_GetError()); \
                                    ++total_tests; }


/*
  Define: CFG_AUTO_SUMMARY

  Outputs summary, number of tests passed and total tests. If everything went right, outputs also that all tests passed.

*/

#define CFG_AUTO_SUMMARY { fprintf(stderr, "\n\n ---------------\n| TESTS SUMMARY |"); \
                         fprintf(stderr, "\n ---------------\n\n"); \
                         fprintf(stderr, "Tests passed: %i\n", tests_passed); \
                         fprintf(stderr, "Total tests: %i\n", total_tests); \
                         if ( tests_passed != total_tests ) \
                         fprintf(stderr, "\n! ERROR OCCURED, AT LEAST ONE OF THE TESTS FAILED !"); \
                         else fprintf(stderr, "\n! GREAT, ALL TESTS PASSED !"); }

#endif
