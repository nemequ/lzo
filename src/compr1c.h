
#define LZO_NEED_DICT_H
#include "config1c.h"


#if !defined(COMPRESS_ID)
#define COMPRESS_ID     LZO_CPP_ECONCAT2(DD_BITS,CLEVEL)
#endif


#include "lzo1b_c.ch"


/***********************************************************************
//
************************************************************************/

#define LZO_COMPRESS \
    LZO_CPP_ECONCAT3(lzo1c_,COMPRESS_ID,_compress)

#define LZO_COMPRESS_FUNC \
    LZO_CPP_ECONCAT3(_lzo1c_,COMPRESS_ID,_compress_func)



/***********************************************************************
//
************************************************************************/

const lzo_compress_t LZO_COMPRESS_FUNC = do_compress;

LZO_PUBLIC(int)
LZO_COMPRESS ( const lzo_bytep in,  lzo_uint  in_len,
                     lzo_bytep out, lzo_uintp out_len,
                     lzo_voidp wrkmem )
{
    return _lzo1c_do_compress(in,in_len,out,out_len,wrkmem,do_compress);
}

/*
vi:ts=4:et
*/
