#include <core.h>
#include <brotli/encode.h>
#include <brotli/decode.h>

#ifndef BROTLI_DEFAULT_QUALITY
  #define BROTLI_DEFAULT_QUALITY (11)
#endif

#ifndef BROTLI_DEFAULT_WINDOW
  #define BROTLI_DEFAULT_WINDOW (22)
#endif

#ifndef BROTLI_DEFAULT_MODE
  #define BROTLI_DEFAULT_MODE (0)
#endif

/* 压缩 */
static int brcompress(lua_State *L) {
  size_t input_size;
  const uint8_t* input_buffer = (const uint8_t*)luaL_checklstring(L, 1, &input_size);
  if (!input_buffer || input_size < 1)
    return luaL_error(L, "Invalid input_buffer.");
  
  /* 计算输出缓冲区大小并初始化 */
  size_t out_size = BrotliEncoderMaxCompressedSize(input_size);
  uint8_t* out_buffer = (uint8_t*)lua_newuserdata(L, out_size);

  if (BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, input_size, input_buffer, &out_size, out_buffer) == BROTLI_FALSE)
    return luaL_error(L, "brotli compression error or output buffer is too small.");
  
  /* 将压缩好的内容返回给调用者 */
  lua_pushlstring(L, (const char *)out_buffer, out_size);
  return 1;
}

/* 解压 */
static int bruncompress(lua_State *L) {
  size_t input_size;
  const uint8_t* input_buffer = (const uint8_t*)luaL_checklstring(L, 1, &input_size);
  if (!input_buffer || input_size < 1)
    return luaL_error(L, "Invalid input_buffer.");

  BrotliDecoderState *dec = BrotliDecoderCreateInstance(NULL, NULL, NULL);

  luaL_Buffer B;
  luaL_buffinit(L, &B);
  size_t total_out = 0;
  size_t in_size = input_size;
  size_t outsize = input_size;
  size_t out_size = input_size;
  uint8_t* out_buffer = NULL;
  for(;;) {
    out_buffer = xrealloc(out_buffer, out_size);
    uint8_t* ptr = out_buffer + total_out;
    BrotliDecoderResult code = BrotliDecoderDecompressStream(dec, &in_size, &input_buffer, &outsize, (uint8_t**)&ptr, &total_out);
    // printf("code = %d, total = %zu, in_size = %zu, out_size = %zu\n", code, total_out, in_size, out_size);
    /* 无效的解压字符串 */
    if (BROTLI_DECODER_RESULT_ERROR == code){
      BrotliDecoderDestroyInstance(dec);
      xfree(out_buffer);
      return luaL_error(L, "BrotliDecoderError: %s", BrotliDecoderErrorString(BrotliDecoderGetErrorCode(dec)));
    }
    /* 如果已经解压完毕 */
    if (BROTLI_DECODER_RESULT_SUCCESS == code) {
      luaL_addlstring(&B, (const char *)out_buffer, total_out);
      luaL_pushresult(&B);
      BrotliDecoderDestroyInstance(dec);
      xfree(out_buffer);
      break;
    }
    outsize = input_size;
    out_size = total_out + input_size;
  }
  return 1;
}

LUAMOD_API int luaopen_lbr(lua_State *L) {
  luaL_checkversion(L);
  luaL_Reg brotli_libs[] = {
    { "compress", brcompress },
    { "uncompress", bruncompress },
    {NULL, NULL},
  };
  luaL_newlib(L, brotli_libs);
  return 1;
}
