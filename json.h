/* JSON File Parser
 * author: Zach Goldthorpe
 *
 * The file provides minimal functionality for parsing JSON files provided in
 * the GIS CUP 2018 specifications, aimed to read in a single sweep while
 * avoiding as much overhead computation as possible.
 *
 * scan_field:     parse a key-value dictionary (field) for specific keywords
 *                 and perform prescribed actions based on which keyword is read
 * scan_list:      run through a list of items performing a prescribed action
 * extract_string: scan and extract a string from the file
 */

#ifndef _json_h_
#define _json_h_
#include <cstdio>
#include <cstdint>
#include <string>

/* reads the next field of the json @file (enclosed by braces "{...}") and scans
 * for the trigger words provided by the array @keys. Once a key has been fully
 * read, the function calls the function-type @action passing the index of the
 * key read as its only argument.
 *
 * returns true if it ever reached a field to scan
 *
 * template param N: number of keys
 * template param FUNC: function-type accepting a single int and returns nothing
 */
template<uint_fast32_t N, typename FUNC>
bool scan_field(FILE *file, const char *const (&keys)[N], const FUNC &action);

/* reads the next list of the json @file (enclosed by brackets "[...]") and
 * repeatedly calls the function-type @action until it exits the list.
 *
 * returns true if it ever reached a list to scan
 *
 * template param FUNC: function-type accepting no arguments and returns nothing
 */
template<typename FUNC>
bool scan_list(FILE *file, const FUNC &action);

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

template<uint_fast32_t N, typename FUNC>
bool scan_field(FILE *file, const char *const (&keys)[N], const FUNC &action) {
    uint_fast32_t pos[N], bracelevel = braces, bracketlevel = brackets;
    // pos tracks string matching with each key
    // bracelevel/bracketlevel store the initial state of the file before scan

    for (uint_fast32_t i = 0; i < N; ++i)
        pos[i] = 0;
    while (bracelevel == braces) {
        readc(file); // read into next field
        if (bracketlevel > brackets)
            return false; // no field to read
    }
    while (bracelevel < braces) {
        char c = readc(file);
        if (bracelevel+1 < braces || bracketlevel < brackets)
            continue;
        for (uint_fast32_t i = 0; i < N; ++i) {
            if (c == keys[i][pos[i]]) {
                ++pos[i]; // update match with key i
                if (keys[i][pos[i]] == '\0')
                    action(i); // key fully matched
            } else pos[i] = 0; // otherwise, no match so restart
        }
    }
    return true;
}

template<typename FUNC>
bool scan_list(FILE *file, const FUNC &action) {
    uint_fast32_t bracketlevel = brackets, bracelevel = braces;
    // store initial state of the file before scan
    while (bracketlevel == brackets) {
        readc(file); // read into next list
        if (bracelevel > braces)
            return false; // no list to read
    }
    while (bracketlevel < brackets)
        action(); // repeatedly call until list exited
    return true;
}

#endif
