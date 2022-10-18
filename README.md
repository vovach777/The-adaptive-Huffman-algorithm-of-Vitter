# The-adaptive-Huffman-algorithm-of-Vitter
by J. S. Vitter

```
{ Written by J. S. Vitter in Berkeley Pascal and run on a VAX 780 running
  Berkeley UNIX 4.3BSD.  This Pascal program implements Algorithm Lambda,
  described and analyzed in the article by J. S. Vitter entitled
  "Design and Analysis of Dynamic Huffman Codes," JACM, Vol. 34, No. 4,
  October 1987.    There are a couple things to note:
    1. The output is done bit by bit using individual write statements,
       which typically output one byte rather than a bit.  So the "compressed"
       file is typically much larger than the original file, because
       each bit is written as a byte.  This was left
       this way because it makes it easier for the implementor to check
       the algorithm during implementation.  A packing routine for output
       can be added later with an easy modification.
    2. When the program is executed,
       the user is prompted for the following information (in that order):
         a. whether encoding or decoding is desired (reply "en" or "de").
         b. which alphabet to use (reply "keys" for size 96, "ascii" for 256).
         c. input file name
         d. output file name
       Each reply is followed by a carraige return.
       The "keys" alphabet includes the printable ASCII characters, plus
       the newline character.  The programmer would have to modify the code
       to accomodate arbitrary alphabet sizes.  The code for "ascii" alphabet
       uses the fact that in Berkely UNIX, characters are represented by
       8-bit values in two's complement, in the range from -128 to 127.
       Since the program expects alphabet values in the range from 1 to 256,
       some conversion is necessary when characters are read in.
       This is easy to modify for any particular implementation.
    3. Because of a Berkeley Pascal quirk, the program effectively adds
       an end-of-line character to the end of the last line of the file
       if it does not already have one.  The encoded file consists
       of a several line of 0s and 1s (the number of 0s and 1s per line
       is specified by the value of lineLengthLimit),
       terminated with the end-of-line and end-of-file characters.}
```       
