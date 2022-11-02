(* https://www.geeksforgeeks.org/adaptive-huffman-coding-and-decoding/ *)
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct TagDHNode;
typedef struct TagDHNode  {
   unsigned weight;
   struct TagDHNode *Parent, *Left, *Right;
   unsigned symbol;
} DHNode_t, *DHNode;

struct TagDHTree;
typedef struct TagDHTree {
   DHNode root;
   DHNode nyt;
   DHNode * symbolsIndex;
   unsigned size;
   unsigned e;
   unsigned r;
   bool * stack;
   void (*put_bit)(void*,bool);
   void (*put_symbol)(void*,unsigned);
   bool (*get_bit)(void *);
   unsigned (*get_symbol)(void *);
   unsigned current_size;
} DHTree_t, *DHTree;

static void calculate_er(unsigned size, unsigned *e, unsigned *r);

void init_dh(DHTree ctx, void (*put_bit)(void*,bool),void (*put_symbol)(void*,unsigned), unsigned size) {
   //initialization of tree
   assert(size > 0);
   ctx->root = ctx->nyt = calloc(sizeof(DHNode_t),1);
   ctx->put_bit = put_bit;
   ctx->put_symbol = put_symbol;
   ctx->size = size;
   ctx->stack = calloc(sizeof(bool), size * 2);
   ctx->symbolsIndex = calloc(sizeof(DHNode), size);
   ctx->current_size = 0;
   calculate_er(size,&ctx->e,&ctx->r);
}

static void calculate_er(unsigned size, unsigned *e, unsigned *r) {
  int E = 0;
  int R = -1;
  for(unsigned M=1; M<=size; M++) {
      R = R + 1;
      if (2 * R == M) {
        E = E + 1;
        R = 0;
      }
  }
  *e = (unsigned)E;
  *r = (unsigned)R;
}

static void  update_dh(DHTree ctx,unsigned symbol) {
   DHNode cur = ctx->symbolsIndex[symbol], nyt = ctx->nyt;
   bool isFirst = cur == NULL;
   if (isFirst) {
      DHNode left = ctx->nyt->Left = calloc(sizeof(DHNode_t),1), right = ctx->nyt->Right = calloc(sizeof(DHNode_t),1);
      left->Parent = right->Parent = nyt;
      ctx->nyt = left;
      ctx->symbolsIndex[symbol] = cur = right;
      ctx->current_size += 1;
      cur->symbol = symbol;
   }
   /* update tree */
   cur->weight+=1;
   for (DHNode update = cur->Parent;update;update = update->Parent) {
      update->weight += 1;
      if (update->Left->weight > update->Right->weight) {
         DHNode tmp = update->Left;
         update->Left = update->Right;
         update->Right = tmp;
      }
   }
}

void encode_dh_symb(DHTree ctx, unsigned symbol) {
   assert(ctx);
   assert(ctx->size);
   assert(symbol < ctx->size);
   assert(ctx->symbolsIndex);
   assert(ctx->root);
   assert(ctx->nyt);
   assert(ctx->stack);
   assert(ctx->put_bit);
   assert(ctx->put_symbol);
   DHNode cur = ctx->symbolsIndex[symbol], nyt = ctx->nyt;
   bool isFirst = cur == NULL;
   if (isFirst) {
      DHNode left = ctx->nyt->Left = calloc(sizeof(DHNode_t),1), right = ctx->nyt->Right = calloc(sizeof(DHNode_t),1);
      left->Parent = right->Parent = nyt;
      ctx->nyt = left;
      ctx->symbolsIndex[symbol] = cur = right;
      ctx->current_size += 1;
      cur->symbol = symbol;
   }
   /* get code of current node*/
   bool*stack=ctx->stack;
   for (DHNode path = isFirst ? nyt : cur; path->Parent; path=path->Parent) {
      assert(path == path->Parent->Left || path == path->Parent->Right);
      *stack++ = path->Parent->Right == path;
   }
   while (--stack >= ctx->stack ) {
      ctx->put_bit(ctx, *stack);
   }
   if (isFirst) {
      ctx->put_symbol(ctx, symbol);
   }
   /* update tree */
   cur->weight+=1;
   for (DHNode update = cur->Parent;update;update = update->Parent) {
      update->weight += 1;
      if (update->Left->weight > update->Right->weight) {
         DHNode tmp = update->Left;
         update->Left = update->Right;
         update->Right = tmp;
      }
   }
}

unsigned decode_dh_symb(DHTree ctx) {
   assert(ctx);
   assert(ctx->size);
   assert(ctx->symbolsIndex);
   assert(ctx->root);
   assert(ctx->nyt);
   assert(ctx->stack);
   assert(ctx->get_bit);
   assert(ctx->get_symbol);
   DHNode walk = ctx->root;
   while ( walk->Left || walk->Right) {
      walk = ctx->get_bit(ctx) ? walk->Right : walk->Left;
   }
   unsigned symbol = walk == ctx->nyt ? ctx->get_symbol(ctx) : walk->symbol;

   update_dh(ctx, symbol);
   return symbol;
}

void put_bit(void*tree, bool bit) {
   printf("%d", bit ? 1 : 0);
}

void print2(unsigned bin, int n) {
   while (n--) {
      printf("%u", bin >> n &1 );
   }
}


void put_symbol(void*tree, unsigned symbol) {
   printf("(k=%u)",symbol+1);
   if (symbol <= ((DHTree)tree)->r*2 )
      print2(symbol, ((DHTree)tree)->e+1 );
   else
      print2(symbol-((DHTree)tree)->r, ((DHTree)tree)->e );
   printf("%c ",symbol+'a');
}

void get_symbol(void*tree) {
   bool (*get_bit)(void*) = ((DHTree)tree)->get_bit;
   unsigned symbol = 0;
   for (int n = ((DHTree)tree)->e; n; n--) {
      symbol = symbol << 1 | get_bit(tree);
   }
   if (symbol < ((DHTree)tree)->r)
      return symbol;
}

int main() {
   DHTree_t dh = {0};
   //initialization of tree
   // dh.root = dh.nyt = calloc(sizeof(DHNode_t),1);
   // dh.put_bit = put_bit;
   // dh.put_symbol = put_symbol;
   // dh.size = 256;
   // dh.stack = calloc(sizeof(bool), dh.size * 2);
   // dh.symbolsIndex = calloc(sizeof(DHNode), dh.size);
   const char *const test_str = "aardvark";

   init_dh(&dh,put_bit, put_symbol, 26);
   printf("m=%u, e=%u, r=%u\n",dh.size,dh.e,dh.r);
   const char * cp = test_str;
   while (*cp) {
      encode_dh_symb(&dh, *cp++ - 'a');
      printf(" ");
   }
   printf("used %d / 26\n", dh.current_size);
   printf("\n");

}
