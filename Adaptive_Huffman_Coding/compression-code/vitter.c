/* fork of https://github.com/fffaraz/Multimedia/blob/master/Adaptive_Huffman_Coding/compression-code/vitter.c */

#include "vitter_not_me.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>


//  please address any bugs discovered in this code
//  to the author: karl malbrain, karl_m@acm.org


//  This code is adapted from Professor Vitter's
//  article, Design and Analysis of Dynamic Huffman Codes,
//  which appeared in JACM October 1987

//  A design trade-off has been made to simplify the
//  code:  a node's block is determined dynamically,
//  and the implicit tree structure is maintained,
//  e.g. explicit node numbers are also implicit.

//  Dynamic huffman table weight ranking
//  is maintained per Professor Vitter's
//  invariant (*) for algorithm FGK:

//  leaves preceed internal nodes of the
//  same weight in a non-decreasing ranking
//  of weights using implicit node numbers:

//  1) leaves slide over internal nodes, internal nodes
//  swap over groups of leaves, leaves are swapped
//  into group leader position, but two internal
//  nodes never change positions relative
//  to one another.

//  2) weights are incremented by 2:
//  leaves always have even weight values;
//  internal nodes always have odd values.

//  3) even node numbers are always right children;
//  odd numbers are left children in the tree.

//  node 2 * HuffSize - 1 is always the tree root;
//  node HuffEsc is the escape node;

//  the tree is initialized by creating an
//  escape node as the root.

//  each new leaf symbol is paired with a new escape
//  node into the previous escape node in the tree,
//  until the last symbol which takes over the
//  tree position of the escape node, and
//  HuffEsc is left at zero.

//  overall table size: 2 * HuffSize

//  huff_init(alphabet_size, potential symbols used)
//  huff_encode(next_symbol)
//  next_symbol = huff_decode()


static void TruncatedBinary(unsigned size, unsigned *k, unsigned *u) {
/* If n is a power of two, then the coded value for 0 ≤ x < n is the simple binary code for x of length log2(n).
    Otherwise let k = floor(log2(n)), such that 2k < n < 2k+1 and let u = 2k+1 − n.  */
	// Set k = floor(log2(n)), i.e., k such that 2^k <= n < 2^(k+1).
	unsigned kk = 0, t = size;
	while (t > 1) { kk++;  t >>= 1; }
   *k=kk;
	// Set u to the number of unused codewords = 2^(k+1) - n.
	*u = (1 << kk + 1) - size;
}


//  huff_scale(by_bits) -- scale weights and rebalance tree

void huff_init (HCoder * huff, unsigned size)
{
    //  default: all alphabet symbols are used

    TruncatedBinary(size, &huff->k, &huff->u);
    unsigned root = size;
    root = size;
    //  create the initial escape node
    //  at the tree root

    if( root <<= 1 )
        root--;

    huff->table = calloc (size*2, sizeof(HTable) );
    huff->stack = calloc (size*2, sizeof(bool) );

    if( huff->size = size )
        huff->map = calloc (size, sizeof(unsigned));

    huff->esc = huff->root = root;

}

void huff_release(HCoder * huff) {
    free(huff->table);
    free(huff->stack);
    free(huff->map);
}

// split escape node to incorporate new symbol

static unsigned huff_split (HCoder *huff, unsigned symbol)
{
unsigned pair, node;

    //  is the tree already full???

    if( pair = huff->esc )
        huff->esc--;
    else
        return 0;

    //  if this is the last symbol, it moves into
    //  the escape node's old position, and
    //  huff->esc is set to zero.

    //  otherwise, the escape node is promoted to
    //  parent a new escape node and the new symbol.

    if( node = huff->esc ) {
        huff->table[pair].down = node;
        huff->table[pair].weight = 1;
        huff->table[node].up = pair;
        huff->esc--;
    } else
        pair = 0, node = 1;

    //  initialize the new symbol node

    huff->table[node].symbol = symbol;
    huff->table[node].weight = 0;
    huff->table[node].down = 0;
    huff->map[symbol] = node;

    //  initialize a new escape node.

    huff->table[huff->esc].weight = 0;
    huff->table[huff->esc].down = 0;
    huff->table[huff->esc].up = pair;
    return node;
}

//  swap leaf to group leader position
//  return symbol's new node

static unsigned huff_leader (HCoder *huff, unsigned node)
{
unsigned weight = huff->table[node].weight;
unsigned leader = node, prev, symbol;

    while( weight == huff->table[leader + 1].weight )
        leader++;

    if( leader == node )
        return node;

    // swap the leaf nodes

    symbol = huff->table[node].symbol;
    prev = huff->table[leader].symbol;

    huff->table[leader].symbol = symbol;
    huff->table[node].symbol = prev;
    huff->map[symbol] = leader;
    huff->map[prev] = node;
    return leader;
}

//  slide internal node up over all leaves of equal weight;
//  or exchange leaf with next smaller weight internal node

//  return node's new position

static unsigned huff_slide (HCoder *huff, unsigned node)
{
unsigned next = node;
HTable swap[1];

    *swap = huff->table[next++];

    // if we're sliding an internal node, find the
    // highest possible leaf to exchange with

    if( swap->weight & 1 )
      while( swap->weight > huff->table[next + 1].weight )
          next++;

    //  swap the two nodes

    huff->table[node] = huff->table[next];
    huff->table[next] = *swap;

    huff->table[next].up = huff->table[node].up;
    huff->table[node].up = swap->up;

    //  repair the symbol map and tree structure

    if( swap->weight & 1 ) {
        huff->table[swap->down].up = next;
        huff->table[swap->down - 1].up = next;
        huff->map[huff->table[node].symbol] = node;
    } else {
        huff->table[huff->table[node].down - 1].up = node;
        huff->table[huff->table[node].down].up = node;
        huff->map[swap->symbol] = next;
    }

    return next;
}

//  increment symbol weight and re balance the tree.

static void huff_increment (HCoder *huff, unsigned node)
{
unsigned up;

  //  obviate swapping a parent with its child:
  //    increment the leaf and proceed
  //    directly to its parent.

  //  otherwise, promote leaf to group leader position in the tree

  if( huff->table[node].up == node + 1 )
    huff->table[node].weight += 2, node++;
  else
    node = huff_leader (huff, node);

  //  increase the weight of each node and slide
  //  over any smaller weights ahead of it
  //  until reaching the root

  //  internal nodes work upwards from
  //  their initial positions; while
  //  symbol nodes slide over first,
  //  then work up from their final
  //  positions.

  while( huff->table[node].weight += 2, up = huff->table[node].up ) {
    while( huff->table[node].weight > huff->table[node + 1].weight )
        node = huff_slide (huff, node);

    if( huff->table[node].weight & 1 )
        node = up;
    else
        node = huff->table[node].up;
  }
}


//  send the bits for an escaped symbol

static void huff_sendid (HCoder *huff, unsigned symbol)
{
    assert(huff->put_symbol);
    huff->put_symbol(huff,symbol);
}

//  encode the next symbol

void huff_encode (HCoder *huff, unsigned symbol)
{
unsigned emit = 1, bit;
unsigned up, idx, node;

    if( symbol < huff->size )
        node = huff->map[symbol];
    else
        return;

    //  for a new symbol, direct the receiver to the escape node
    //  but refuse input if table is already full.

    if( !(idx = node) )
      if( !(idx = huff->esc) )
        return;



    bool*stack=huff->stack;

    //  accumulate the code bits by
    //  working up the tree from
    //  the node to the root

    while( up = huff->table[idx].up ) {
      //   emit <<= 1, emit |= idx & 1, idx = up;
      *stack++=idx & 1;
      idx = up;
    }
    //  send the code, root selector bit first

    assert(huff->put_bit);

    while (--stack >= huff->stack ) {
        huff->put_bit(huff, *stack);
    }

    //  send identification and incorporate
    //  new symbols into the tree

    if( !node )
        huff_sendid(huff, symbol), node = huff_split(huff, symbol);

    //  adjust and re-balance the tree

    huff_increment (huff, node);
}

//  read the identification bits
//  for an escaped symbol

static inline unsigned huff_readid (HCoder *huff)
{

    return  huff->get_symbol(huff);

}

//  decode the next symbol

unsigned huff_decode (HCoder *huff)
{
unsigned node = huff->root;
unsigned symbol, down;

    //  work down the tree from the root
    //  until reaching either a leaf
    //  or the escape node.  A one
    //  bit means go left, a zero
    //  means go right.

    while( down = huff->table[node].down )
      if( huff->get_bit(huff) )
        node = down - 1;  // the left child preceeds the right child
      else
        node = down;

    //  sent to the escape node???
    //  refuse to add to a full tree

    if( node == huff->esc )
      if( huff->esc )
        symbol = huff_readid (huff), node = huff_split (huff, symbol);
      else
        return 0;
    else
        symbol = huff->table[node].symbol;

    //  increment weights and rebalance
    //  the coding tree

    huff_increment (huff, node);
    return symbol;
}

