ACM SIGSPATIAL GIS Cup 2018 Submission
=======================================

The following is the largely untouched submission for the [GIS Cup 2018](https://sigspatial2018.sigspatial.org/giscup2018/) by Zach Goldthorpe, Jason Cannon, and Jesse Farebrother. An elaboration on how the algorithm works at the higher level is provided in our paper "Using Biconnected Components for Efficient Identification of Upstream Features in Large Spatial Networks (GIS Cup".

DELIVERABLES
=============
- `Makefile`
- `upstream_features.cpp`
- `read_graph.h`
- `json.h`
- `json_fast.h`


REQUIREMENTS TO COMPILE
========================
`C++11` and its STL, namely:
- `cstdio`
- `cstdint`
- `fstream`
- `string`
- `stack`
- `unordered_map`
- `unordered_set`
- `vector`


HOW TO COMPILE
===============
If the format of the JSON file is "sufficiently nice" (see the [input assumptions](#json-file) for the JSON file), simply call
```bash
$ make
```
or
```bash
$ make fast
```
However, if the JSON file requires the robust reader, compile with
```bash
$ make robust
```

HOW TO RUN
===========
The executable takes three inline arguments: the JSON file, the file containing the starting points, and the name of the output file, so run it with
```bash
$ ./giscup-2018 /path/to/data.json /path/to/startingpoints.txt /path/to/answer
```
The names of the input files are unimportant beyond actually existing.

The solution summary can be found at the top of the source code in `upstream_features.cpp`.


INPUT ASSUMPTIONS
==================

JSON FILE
----------
The JSON file is assumed to be organised in the following way:
```
{
    "rows" : [
        {
            "viaGlobalId" :  <string>,
            "fromGlobalId" : <string>,
            "toGlobalId" :   <string>
        },
        // repeat the above
    ],
    "controllers" : [
        {
            "globalId": <string>
        },
        // repeat as above
    ]
}
```
The whitespace is unnecessary, and there can be other keys in the input. If the JSON file has these keys in this exact order (relative to each other, irrespective of the positioning of unmentioned keys), then the input is "sufficiently nice" to use our fast reader. If all these keys exist, but may not be in this order, then the robust reader is necessary (see [how to compile](#how-to-compile)).

STARTING POINTS FILE
---------------------
The file should contain names (one per line, without quotation marks) indicating the global ID of features that appear in the JSON file indicating which features are starting points.


OUTPUT ASSUMPTIONS
===================
The output file does not output the upstream features in any particular order, and keys may be output more than once.