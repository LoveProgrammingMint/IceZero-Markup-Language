#ifndef IZML_RUNTIME_H
#define IZML_RUNTIME_H

#include <stdint.h>

#if defined(_WIN32)
#  if defined(IZMLRUNTIME_EXPORTS)
#    define IZMLRUNTIME_API __declspec(dllexport)
#  else
#    define IZMLRUNTIME_API __declspec(dllimport)
#  endif
#else
#  define IZMLRUNTIME_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct izmlrt_archive izmlrt_archive;

IZMLRUNTIME_API izmlrt_archive* izmlrt_open(const char* path);
IZMLRUNTIME_API izmlrt_archive* izmlrt_open_memory(const uint8_t* data, uint64_t length);
IZMLRUNTIME_API void izmlrt_close(izmlrt_archive* archive);

IZMLRUNTIME_API uint16_t izmlrt_version(const izmlrt_archive* archive);
IZMLRUNTIME_API uint16_t izmlrt_min_version(const izmlrt_archive* archive);
IZMLRUNTIME_API uint32_t izmlrt_root_count(const izmlrt_archive* archive);

IZMLRUNTIME_API int32_t izmlrt_read_u8(izmlrt_archive* archive, uint8_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_i8(izmlrt_archive* archive, int8_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_u16(izmlrt_archive* archive, uint16_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_i16(izmlrt_archive* archive, int16_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_u32(izmlrt_archive* archive, uint32_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_i32(izmlrt_archive* archive, int32_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_u64(izmlrt_archive* archive, uint64_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_i64(izmlrt_archive* archive, int64_t* value);
IZMLRUNTIME_API int32_t izmlrt_read_f32(izmlrt_archive* archive, float* value);
IZMLRUNTIME_API int32_t izmlrt_read_f64(izmlrt_archive* archive, double* value);
IZMLRUNTIME_API int32_t izmlrt_read_bool(izmlrt_archive* archive, int32_t* value);

IZMLRUNTIME_API int32_t izmlrt_read_string(izmlrt_archive* archive, const char** data, uint32_t* length);
IZMLRUNTIME_API int32_t izmlrt_read_bytes(izmlrt_archive* archive, const uint8_t** data, uint32_t* length);

IZMLRUNTIME_API const char* izmlrt_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
