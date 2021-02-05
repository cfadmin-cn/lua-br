#define LUA_LIB

#include <core.h>
#include <brotli/encode.h>
#include <brotli/decode.h>

#ifndef BROTLI_MIN_QUALITY
  #define BROTLI_MIN_QUALITY (0)
#endif

#ifndef BROTLI_MAX_QUALITY
  #define BROTLI_MAX_QUALITY (11)
#endif

#ifndef BROTLI_DEFAULT_QUALITY
  #define BROTLI_DEFAULT_QUALITY (11)
#endif

#ifndef BROTLI_MIN_WINDOW_BITS
  #define BROTLI_MIN_WINDOW_BITS (10)
#endif

#ifndef BROTLI_MAX_WINDOW_BITS
  #define BROTLI_MAX_WINDOW_BITS (24)
#endif

#ifndef BROTLI_DEFAULT_WINDOW
  #define BROTLI_DEFAULT_WINDOW (22)
#endif

#ifndef BROTLI_DEFAULT_MODE
  #define BROTLI_DEFAULT_MODE (0)
#endif

static int br_default_quality = 6;
static int br_default_window = 22;

/* 压缩 */
static int brcompress(lua_State *L) {
  size_t input_size;
  const uint8_t* input_buffer = (const uint8_t*)luaL_checklstring(L, 1, &input_size);
  if (!input_buffer || input_size < 1)
    return luaL_error(L, "Invalid input_buffer.");
  
  /* 计算输出缓冲区大小并初始化 */
  size_t out_size = BrotliEncoderMaxCompressedSize(input_size);
  uint8_t* out_buffer = (uint8_t*)lua_newuserdata(L, out_size);

  if (BrotliEncoderCompress(br_default_quality, br_default_window, BROTLI_DEFAULT_MODE, input_size, input_buffer, &out_size, out_buffer) == BROTLI_FALSE)
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

  size_t times = 5;
  size_t out_size = input_size * 3;
  size_t outsize = out_size;
  for(;;) {
    /* 每次申请新内存块 */
    uint8_t* out_buffer = lua_newuserdata(L, out_size);
    /* 目前不考虑失败的情况, 失败五次直接当无法解压. */
    if (BrotliDecoderDecompress(input_size, input_buffer, &outsize, out_buffer) == BROTLI_DECODER_RESULT_SUCCESS) {
      lua_pushlstring(L, (const char *)out_buffer, outsize);
      break;
    }
    if (!times)
      return 0;
    /* 每次尝试按照最好的方式跳转内存块申请大小 */
    lua_settop(L, lua_gettop(L) - 1);
    outsize = out_size = out_size * 3;
    times--;
  }
  return 1;
}

// 设置window大小
static int setbrwindow(lua_State *L) {
  lua_Integer window = luaL_checkinteger(L, 1);
  if (window < BROTLI_MIN_WINDOW_BITS || window > BROTLI_MAX_WINDOW_BITS)
    return 0;
  br_default_window = window;
  return 1;
}

// 设置quality大小
static int setbrquality(lua_State *L) {
  lua_Integer quality = luaL_checkinteger(L, 1);
  if (quality < BROTLI_MIN_QUALITY || quality > BROTLI_MAX_QUALITY)
    return 0;
  br_default_quality = quality;
  return 1;
}

LUAMOD_API int luaopen_lbr(lua_State *L) {
  luaL_checkversion(L);
  luaL_Reg brotli_libs[] = {
    { "compress", brcompress },
    { "uncompress", bruncompress },
    { "set_window", setbrwindow },
    { "set_quality", setbrquality },
    {NULL, NULL},
  };
  luaL_newlib(L, brotli_libs);
  return 1;
}
