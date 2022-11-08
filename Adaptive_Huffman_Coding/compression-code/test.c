#include "libavutil/common.h"
#include "libavcodec/avcodec.h"
#include "libavutil/file.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/put_bits.h"
#include "libavutil/lfg.h"
#include "libavutil/avassert.h"
#include "vitter.h"

typedef struct
{
   HCoder base;
   PutBitContext pb;
   GetBitContext gb;
} DHTreeImpl, *DHTreeImplRef;

#define CAST2IMPL DHTreeImplRef impl = (DHTreeImplRef) tree;
#define PUTBIT_CTX (&impl->pb)
#define GETBIT_CTX (&impl->gb)


static void check_grow( PutBitContext* pb ) {
      if ( pb->buf_end-pb->buf_ptr < 64 ) {
         size_t sz = pb->buf_end-pb->buf;
         if (sz < 64) sz = 64;
         sz = sz * 2 ;
         rebase_put_bits(pb,realloc(pb->buf, sz +  AV_INPUT_BUFFER_PADDING_SIZE), sz);
      }
}

static void put_bit(HCoder* tree, bool bit) {
   CAST2IMPL
   check_grow(PUTBIT_CTX);
   put_bits(PUTBIT_CTX,1,bit);
  // printf("%u",(unsigned)bit);
}

static void put_symbol(HCoder* tree, unsigned symbol) {
   CAST2IMPL
   check_grow(PUTBIT_CTX);
   const unsigned u = tree->u;
   const unsigned k = tree->k;
   if (symbol < u)
      put_bits(PUTBIT_CTX, k,   symbol);
   else
      put_bits(PUTBIT_CTX,  k+1, symbol+ u );
//   printf("[%u]",symbol);
}

static unsigned get_symbol(HCoder* tree){
   CAST2IMPL
   const unsigned u = tree->u;
   const unsigned k = tree->k;
   unsigned val = get_bits(GETBIT_CTX,k);
   if (val >= u)
      val = ( (val << 1) | get_bits1(GETBIT_CTX) )- u;
//   printf("[%c]", val + 'a');
   return val;
}

static bool get_bit(HCoder* tree) {
   CAST2IMPL
   const bool bit = get_bits1(GETBIT_CTX);
//   printf("%u",(unsigned)bit);
   return bit;
}


int main(int argc, char**argv) {

   printf("test not my vitter alg impl...\n");
   DHTreeImpl impl = {0};
   uint8_t * data;
   size_t data_len;
   int ret = av_file_map(argv[ argc -1], &data, &data_len, 0, NULL);
   av_assert0(ret == 0);


   huff_init(&impl.base,256); //0-60 - словарь. 61 - байпас, 62 - rle, 63 - end
   impl.base.get_bit = get_bit;
   impl.base.get_symbol = get_symbol;
   impl.base.put_bit = put_bit;
   impl.base.put_symbol = put_symbol;

   init_put_bits(&impl.pb,malloc(8+AV_INPUT_BUFFER_PADDING_SIZE),8);


   for (int i=0; i < data_len; i++) {
      huff_encode(&impl.base, data[i]);
   }
   //printf("\n");

   huff_release(&impl.base);

   unsigned bit_count = put_bits_count(&impl.pb);
   printf("compress %zu -> %zu bytes\n", data_len, put_bytes_count(&impl.pb,1));
   flush_put_bits(&impl.pb);
   init_get_bits(&impl.gb,impl.pb.buf,bit_count);

//   printf("\ndecode:\n");

   huff_init(&impl.base,256); //0-60 - словарь. 61 - байпас, 62 - rle, 63 - end

   for (int i=0; i<sizeof(data) && get_bits_left(&impl.gb) > 0; i++) {
      unsigned symb = huff_decode(&impl.base);
      av_assert0(symb == data[i]);
//      printf(" ");
//      printf("%u -> %u%s", data[i], symb, data[i] == symb ? "\n" : " fail!\n");
   }
   huff_release(&impl.base);
//   printf("\n");

}
