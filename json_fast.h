/* JSON File Parser (fast)
 * author: Zach Goldthorpe
 *
 * The file provides minimal functionality for parsing JSON files provided in
 * the GIS CUP 2018 specifications, aimed to read in a single sweep while
 * avoiding as much overhead computation as possible.
 *
 * read_to_key:    parse the input until the specified key is encountered
 * begin_field /
 * end_field:      scan the input until a field is entered or exited
 * begin_list /
 * end_list:       scan the input until a list is entered or exited
 * extract_string: scan and extract a string from the file
 */

#ifndef _json_h_fast
#define _json_h_fast
#include <cstdio>
#include <cstdint>
#include <string>

/* scans the json @file until it reads the entire @key
 *
 * returns true if the key is successfully read
 */
bool read_to_key(FILE *file, const char *key);

/* these scan the json @file until the field (enclosed by braces {...}) has been
 * entered or exited.
 *
 * begin_field returns true if a new field has been entered.
 */
bool begin_field(FILE *file);
void end_field(FILE *file);

/* similarly, these scan the json @file until the list (enclosed by brackets
 * [...]) has been entered or exited.
 *
 * begin_list returns true if a new list has been entered.
 */
bool begin_list(FILE *file);
void end_list(FILE *file);

/* scans the json @file for the next string (enclosed by double quotes "...")
 * and writes it to @out
 */
void extract_string(FILE *file, std::string &out);


// ==== DEFINITIONS ==== //

static uint_fast32_t braces, brackets; // tracks the brace and bracket levels
static bool instring; // tracks whether inside a string or not

/* reads the next character of the json @file, while also tracking bookkeeping
 * information (braces, brackets, instring defined above)
 *
 * returns the read character
 */
static char readc(FILE *file) {
    char c = fgetc_unlocked(file);
    if (c == '"') {
        // entered or exited a string
        instring = !instring;
        return c;
    }
    if (instring)
        return c; // character holds no other meaning if inside a string

    switch (c) {
        case '{':
        ++braces; // entered a new field
        break;
        case '}':
        --braces; // exited a field
        break;
        case '[':
        ++brackets; // entered a new list
        break;
        case ']':
        --brackets; // exited a list
        break;
        default: // otherwise it is an ordinary character, so do nothing
        break;
    }
    return c;
}

void extract_string(FILE *file, std::string &out) {
    while (!instring) readc(file); // read to next string
    out.clear();
    for (char c = readc(file); instring; c = readc(file))
        out.push_back(c); // extract string
    out.push_back('\n');
}

bool read_to_key(FILE *file, const char *key) {
    uint_fast32_t level = braces, match = 0;
    while (key[match] && level <= braces) {
        if (readc(file) == key[match])
            ++match;
        else
            match = 0;
    }
    return !key[match];
}

bool begin_field(FILE *file) {
    uint_fast32_t level = braces, stop = brackets;
    while (level == braces && stop <= brackets)
        readc(file); // read until new field entered or list level exited
    return stop <= brackets;
}
void end_field(FILE *file) {
    uint_fast32_t level = braces;
    while (level <= braces)
        readc(file); // read until field exited
}

bool begin_list(FILE *file) {
    uint_fast32_t level = brackets, stop = braces;
    while (level == brackets && stop <= braces)
        readc(file); // read until new list entered or field level exited
    return stop <= braces;
}
void end_list(FILE *file) {
    uint_fast32_t level = brackets;
    while (level <= brackets)
        readc(file); // read until list exited
}


#endif
