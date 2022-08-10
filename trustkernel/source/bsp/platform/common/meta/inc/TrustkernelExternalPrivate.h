#ifndef TRUSTKERNEL_EXTERNAL_H_
#define TRUSTKERNEL_EXTERNAL_H_

#define KPHA_ERR_TYPE_SHIFT     (20)
#define KPHA_ERR_TYPE_MASK      (0xFFF)

#define KPHA_ERR_SUBERR_SHIFT   (12)
#define KPHA_ERR_SUBERR_MASK    (0xFFU)

#define KPHA_ERR_LINENO_SHIFT   (0)
#define KPHA_ERR_LINENO_MASK    (0xFFFU)

#define KPHA_ERRNO_EXT(T, I) \
    ((int) ((((uint32_t) (T)) << KPHA_ERR_TYPE_SHIFT) | \
    ((((uint32_t) (I)) & KPHA_ERR_SUBERR_MASK) << KPHA_ERR_SUBERR_SHIFT) | \
    ((__LINE__) & KPHA_ERR_LINENO_MASK)))

#define KPHA_ERRNO(T) KPHA_ERRNO_EXT((T), 0)

#define KPHA_ERR_SUCCESS        (0x00000000)

#define KPHA_ERR_BAD_PARAMETERS KPHA_ERRNO(0xF06)
#define KPHA_ERR_OUT_OF_MEMORY  KPHA_ERRNO(0xF0C)
#define KPHA_ERR_SHORT_BUFFER   KPHA_ERRNO(0xF10)

#define KPHA_ERR_BAD_WRITE      KPHA_ERRNO(0xF20)
#define KPHA_ERR_BAD_EXEC       KPHA_ERRNO(0xF21)
#define KPHA_ERR_UNDEFINED      KPHA_ERRNO(0xF22)

#define KPHA_ERR_EXEC_BASE      (0xFFDU)
#define KPHA_ERR_EXEC(err)      KPHA_ERRNO_EXT(KPHA_ERR_EXEC_BASE, (err))

#define KPHA_ERR_POSIX_BASE     (0xFFEU)
#define KPHA_ERR_POSIX(err)     KPHA_ERRNO_EXT(KPHA_ERR_POSIX_BASE, (err))

#define KPHA_ERR_CUSTOM_BASE    (0xFFFU)
#define KPHA_ERR_CUSTOM         KPHA_ERRNO(KPHA_ERR_CUSTOM_BASE)

#define KPHA_ERR_TYPE(err)      (((uint32_t) (err)) >> KPHA_ERR_TYPE_SHIFT)
#define KPHA_ERR_SUBERR(err)    ((((uint32_t) (err)) >> KPHA_ERR_SUBERR_SHIFT) & KPHA_ERR_SUBERR_MASK)

#define KPHA_META_RESULT_SUCCESS        (static_cast<unsigned char>(0))
#define KPHA_META_RESULT_FAILED         (static_cast<unsigned char>(1))
#define KPHA_META_RESULT_BAD_BUFFER     (static_cast<unsigned char>(2))
#define KPHA_META_RESULT_BAD_COMMAND    (static_cast<unsigned char>(3))


static void __kpha_debugprint(const char *func, int lineno, const char *prefix, const char *fmt, ...);

#define kpha_log(...) __kpha_debugprint(__func__, __LINE__, "INFO", __VA_ARGS__)
#define kpha_err(...) __kpha_debugprint(__func__, __LINE__, "ERR", __VA_ARGS__)

#include "MetaPub.h"

#pragma pack(1)

typedef struct {
    uint32_t ret;
    uint32_t bufsize;
    uint64_t bufptr;
} meta_buf_header_t;

#pragma pack()

#define FILL_META_BUF_RETURN_VALUE(__buf__, __rv__) \
    do { \
        meta_buf_header_t *__mbh = reinterpret_cast<meta_buf_header_t *>(__buf__); \
        __mbh->ret = static_cast<uint32_t>(__rv__); \
    } while(0)

#define CALL_META_FUNC_ARGS(__fn, ...) \
    do { \
        int capacity; \
        capacity = size < (int) sizeof(meta_buf_header_t) ? \
            (int) sizeof(meta_buf_header_t) : size; \
        if (size < capacity) { \
            if ((buf = (char *) malloc(capacity)) == NULL) \
                return -1; \
            memcpy(buf, *pbuf, size); free(*pbuf); *pbuf = buf; size = capacity;\
        } \
        if ((ret = __fn(__VA_ARGS__))) {\
            *r = KPHA_META_RESULT_FAILED; \
            *psize = sizeof(meta_buf_header_t); \
            FILL_META_BUF_RETURN_VALUE(buf, ret); \
        } else { \
            *psize = size; \
            *r = KPHA_META_RESULT_SUCCESS; \
        } \
    } while(0)

#endif
