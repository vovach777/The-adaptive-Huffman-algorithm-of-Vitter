#pragma once
#include <stdbool.h>
/*
      huff = huff_init (256, 256);
      huff_encode(huff, symbol);
    //  decompression

      huff = huff_init (256, size);


        symbol = huff_decode(huff);
            continue;
*/

typedef struct {
    unsigned up,      // next node up the tree
        down,         // pair of down nodes
        symbol,       // node symbol value
        weight;       // node weight
} HTable;

typedef struct HCoder_tag HCoder;


struct HCoder_tag {
    unsigned esc,     // the current tree height
        k,  //truncated
        u,  //binary constants
        root,         // the root of the tree
        size,         // the alphabet size
        *map;         // mapping for symbols to nodes
        bool *stack;  //nodeid reverse
    HTable * table;  // the coding table starts here
   void (*put_bit)(HCoder*,bool);
   void (*put_symbol)(HCoder*,unsigned);
   bool (*get_bit)(HCoder*);
   unsigned (*get_symbol)(HCoder*);
};

//  initialize an adaptive coder
//  for alphabet size, and count
//  of nodes to be used

void huff_init (HCoder *huff, unsigned size);

//  encode the next symbol
void huff_encode (HCoder *huff, unsigned symbol);

//  decode the next symbol
unsigned huff_decode (HCoder *huff);

void huff_release(HCoder * huff);
