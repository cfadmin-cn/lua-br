#define LUA_LIB

#include <core.h>
#include <brotli/encode.h>
#include <brotli/decode.h>

static int br_block           = 512;
static int br_default_mode    = BROTLI_DEFAULT_MODE;
static int br_default_quality = BROTLI_DEFAULT_QUALITY;
static int br_default_window  = BROTLI_DEFAULT_WINDOW;

/* 压缩 */
static int brcompress(lua_State *L) {
  size_t input_size;
  const uint8_t* input_buffer = (const uint8_t*)luaL_checklstring(L, 1, &input_size);
  if (!input_buffer || input_size < 1)
    return luaL_error(L, "[Brotli ERROR]: Invalid compress buffer.");
  
  /* 计算输出缓冲区大小并初始化 */
  size_t out_size = BrotliEncoderMaxCompressedSize(input_size);
  uint8_t* out_buffer = (uint8_t*)lua_newuserdata(L, out_size);

  if (BrotliEncoderCompress(br_default_quality, br_default_window, br_default_mode, input_size, input_buffer, &out_size, out_buffer) == BROTLI_FALSE)
    return luaL_error(L, "[Brotli ERROR]: output buffer too small.");
  
  /* 将压缩好的内容返回给调用者 */
  lua_pushlstring(L, (const char *)out_buffer, out_size);
  return 1;
}

/* 解压 */
static int bruncompress(lua_State *L) {
  size_t input_size;
  const uint8_t* input_buffer = (const uint8_t*)luaL_checklstring(L, 1, &input_size);
  if (!input_buffer || input_size < 1)
    return luaL_error(L, "[Brotli ERROR]: Invalid uncompress buffer.");

  BrotliDecoderState *state = BrotliDecoderCreateInstance(NULL, NULL, NULL);

  luaL_Buffer B;
  luaL_buffinit(L, &B);

  size_t out_size = br_block;
  uint8_t *buffer = alloca(br_block);
  uint8_t *out = buffer;

  while (1)
  {
    switch (BrotliDecoderDecompressStream(state, &input_size, &input_buffer, &out_size, &out, NULL))
    {
      case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT: /* 输出缓冲区内存不足 */
        luaL_addlstring(&B, (const char*)buffer, br_block - out_size);
        out_size = br_block;
        out = buffer;
        break;
      case BROTLI_DECODER_RESULT_SUCCESS:  /* 解码完成 */
        luaL_addlstring(&B, (const char*)buffer, br_block - out_size);
        luaL_pushresult(&B);
        BrotliDecoderDestroyInstance(state);
        return 1;
      default:
        lua_pushboolean(L, 0);
        lua_pushfstring(L, "[Brotli ERROR]: corrupted input buffer or memory not enough.");
        BrotliDecoderDestroyInstance(state);
        return 2;
    }
  }
  /* end */
}

// 设置window大小
static int set_window(lua_State *L) {
  lua_Integer window = luaL_checkinteger(L, 1);
  if (window < BROTLI_MIN_WINDOW_BITS || window > BROTLI_MAX_WINDOW_BITS)
    return 0;
  br_default_window = window;
  return 1;
}

// 设置quality大小
static int set_quality(lua_State *L) {
  lua_Integer quality = luaL_checkinteger(L, 1);
  if (quality < BROTLI_MIN_QUALITY || quality > BROTLI_MAX_QUALITY)
    return 0;
  br_default_quality = quality;
  return 1;
}

// 设置压缩mode
static int set_mode(lua_State *L) {
  lua_Integer mode = luaL_checkinteger(L, 1);
  if (mode < 1 || mode > 2)
    return 0;
  br_default_mode = mode;
  return 1;
}

// 设置每次解压块大小
static int set_block(lua_State *L) {
  lua_Integer block = luaL_checkinteger(L, 1);
  if (block < 1 || block > 65535)
    return 0;
  br_block = block;
  return 1;
}

LUAMOD_API int luaopen_lbr(lua_State *L) {
  luaL_checkversion(L);
  luaL_Reg brotli_libs[] = {
    { "compress",     brcompress },
    { "uncompress",   bruncompress },
    { "set_window",   set_window },
    { "set_quality",  set_quality },
    { "set_block",    set_block },
    { "set_mode",     set_mode },
    {NULL, NULL},
  };
  luaL_newlib(L, brotli_libs);
  return 1;
}
