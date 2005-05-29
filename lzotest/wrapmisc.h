/* wrapmisc.h -- misc wrapper functions for the test driver

   This file is part of the LZO data compression library.

   Copyright (C) 1996-2005 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */


/*************************************************************************
// compression levels of zlib
**************************************************************************/

#if defined(ALG_ZLIB)

#if !defined(ZLIB_VERNUM)
/* assume 1.1.4 */
#define ZLIB_VERNUM 0x1140
#endif


#if (USHRT_MAX > 0xffffffffL)
#define ZLIB_MEM_COMPRESS      1200000L
#define ZLIB_MEM_DECOMPRESS     240000L
#elif (ULONG_MAX > 0xffffffffL)
#define ZLIB_MEM_COMPRESS       600000L
#define ZLIB_MEM_DECOMPRESS     120000L
#else
#define ZLIB_MEM_COMPRESS       300000L
#define ZLIB_MEM_DECOMPRESS      60000L
#endif

#if (ZLIB_VERNUM >= 0x1200)
#undef ZLIB_MEM_DECOMPRESS
#if (USHRT_MAX > 0xffffffffL)
#define ZLIB_MEM_DECOMPRESS      28672
#elif (USHRT_MAX > 0xffffL) || (UINT_MAX > 0xffffffffL)
#define ZLIB_MEM_DECOMPRESS      14336
#else
#define ZLIB_MEM_DECOMPRESS       7168  /* actually 7080 on ARCH_I386 */
#endif
#endif


#undef ZLIB_USE_MALLOC
#ifndef ZLIB_USE_MALLOC
static m_bytep zlib_heap_ptr = NULL;
static m_uint32 zlib_heap_used = 0;
static m_uint32 zlib_heap_size = 0;

static
voidpf zlib_zalloc ( voidpf opaque, uInt items, uInt size )
{
    m_uint32 bytes = (m_uint32) items * size;
    voidpf ptr = (voidpf) zlib_heap_ptr;

    bytes = (bytes + 15) & ~(m_uint32)15;
    if (zlib_heap_used + bytes > zlib_heap_size)
        return 0;

    zlib_heap_ptr  += bytes;
    zlib_heap_used += bytes;
    LZO_UNUSED(opaque);
    return ptr;
}

static
void zlib_zfree ( voidpf opaque, voidpf ptr )
{
    LZO_UNUSED(opaque); LZO_UNUSED(ptr);
}
#endif

static
void zlib_alloc_init ( z_stream *strm, m_voidp wrkmem, m_uint32 s )
{
#ifndef ZLIB_USE_MALLOC
    zlib_heap_ptr  = (m_bytep) wrkmem;
    zlib_heap_size = s;
    zlib_heap_used = 0;

    /*strm->zalloc = (alloc_func) zlib_zalloc;*/
    /*strm->zfree = (free_func) zlib_zfree;*/
    strm->zalloc = zlib_zalloc;
    strm->zfree = zlib_zfree;
#else
    strm->zalloc = (alloc_func) 0;
    strm->zfree = (free_func) 0;
#endif
}


int zlib_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem,
                                int c, int level )
{
    /* use the undocumented feature to suppress the zlib header */
    z_stream stream;
    int err = Z_OK;
    int flush = Z_FINISH;
    int windowBits = opt_dict ? MAX_WBITS : -(MAX_WBITS);

#if 0
    stream.next_in = (Bytef *) src;         /* UNCONST */
#else
    {
        union { const m_bytep cp; m_bytep p; } u;
        u.cp = src;
        stream.next_in = (Bytef *) u.p;     /* UNCONST */
    }
#endif
    stream.avail_in = src_len;
    stream.next_out = (Bytef *) dst;
    stream.avail_out = *dst_len;
    *dst_len = 0;

    zlib_alloc_init(&stream,wrkmem,ZLIB_MEM_COMPRESS);

#if 0
    err = deflateInit(&stream, level);
#else
    err = deflateInit2(&stream, level, c, windowBits,
                       MAX_MEM_LEVEL > 8 ? 8 : MAX_MEM_LEVEL,
                       Z_DEFAULT_STRATEGY);
#endif
    if (err == Z_OK && opt_dict && dict.ptr)
        err = deflateSetDictionary(&stream, dict.ptr, dict.len);
    if (err == Z_OK)
    {

        err = deflate(&stream, flush);
        if (err != Z_STREAM_END)
        {
            deflateEnd(&stream);
            err = (err == Z_OK) ? Z_BUF_ERROR : err;
        }
        else
        {
            *dst_len = (m_uint) stream.total_out;
            err = deflateEnd(&stream);
        }
    }
    LZO_UNUSED(windowBits);
    return err;
}


M_PRIVATE(int)
zlib_decompress         ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{
    /* use the undocumented feature to suppress the zlib header */
    z_stream stream;
    int err = Z_OK;
    int flush = Z_FINISH;
    int windowBits = opt_dict ? MAX_WBITS : -(MAX_WBITS);

#if 0
    stream.next_in = (Bytef *) src;         /* UNCONST */
#else
    {
        union { const m_bytep cp; m_bytep p; } u;
        u.cp = src;
        stream.next_in = (Bytef *) u.p;     /* UNCONST */
    }
#endif
    stream.avail_in = src_len;
    stream.next_out = (Bytef *) dst;
    stream.avail_out = *dst_len;
    *dst_len = 0;

    zlib_alloc_init(&stream,wrkmem,ZLIB_MEM_DECOMPRESS);

#if (ZLIB_VERNUM < 0x1200)
    if (windowBits < 0)
        stream.avail_in++;  /* inflate requires an extra "dummy" byte */
#endif
    err = inflateInit2(&stream, windowBits);
    while (err == Z_OK)
    {
        err = inflate(&stream, flush);
        if (flush == Z_FINISH && err == Z_OK)
            err = Z_BUF_ERROR;
        if (err == Z_STREAM_END)
        {
            *dst_len = (m_uint) stream.total_out;
            err = inflateEnd(&stream);
            break;
        }
        else if (err == Z_NEED_DICT && opt_dict && dict.ptr)
            err = inflateSetDictionary(&stream, dict.ptr, dict.len);
        else if (err != Z_OK)
        {
            (void) inflateEnd(&stream);
            break;
        }
    }
    LZO_UNUSED(windowBits);
    return err;
}


M_PRIVATE(int)
zlib_8_1_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,1); }

M_PRIVATE(int)
zlib_8_2_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,2); }

M_PRIVATE(int)
zlib_8_3_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,3); }

M_PRIVATE(int)
zlib_8_4_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,4); }

M_PRIVATE(int)
zlib_8_5_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,5); }

M_PRIVATE(int)
zlib_8_6_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,6); }

M_PRIVATE(int)
zlib_8_7_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,7); }

M_PRIVATE(int)
zlib_8_8_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,8); }

M_PRIVATE(int)
zlib_8_9_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,9); }


#endif /* ALG_ZLIB */


/*************************************************************************
// compression levels of bzip2
**************************************************************************/

#if defined(ALG_BZIP2)

#endif /* ALG_BZIP2 */


/*************************************************************************
// other wrappers (pseudo compressors)
**************************************************************************/

#if defined(ALG_ZLIB)

M_PRIVATE(int)
zlib_adler32_x_compress ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{
    uLong adler;
    adler = adler32(1L, src, src_len);
    *dst_len = src_len;
    LZO_UNUSED(adler);
    LZO_UNUSED(dst);
    LZO_UNUSED(wrkmem);
    return 0;
}


M_PRIVATE(int)
zlib_crc32_x_compress   ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{
    uLong crc;
    crc = crc32(0L, src, src_len);
    *dst_len = src_len;
    LZO_UNUSED(crc);
    LZO_UNUSED(dst);
    LZO_UNUSED(wrkmem);
    return 0;
}

#endif /* ALG_ZLIB */


/*
vi:ts=4:et
*/

