#! /bin/sh

# Small shell script to generate test files.
# It is a test file in itself, somehow.
#
# We generate everything from the sin.dat file.

(cat sin.dat; echo; echo; echo; echo) > newlines.dat
cat sin.dat | iconv -t UTF-16 > utf16.dat
cat sin.dat | iconv -t UCS2 > ucs2.dat
