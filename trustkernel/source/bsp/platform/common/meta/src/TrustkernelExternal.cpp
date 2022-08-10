#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/system_properties.h>
#include <unistd.h>

/* for STL */
#include <string>
#include <unordered_map>

#include <pl.h>

#include "TrustkernelExternalPrivate.h"
#include "LogDefine.h"

#define str(x) #x
#define xstr(x) str(x)

#define MAJOR_VERSION   3
#define MIDDL_VERSION   1
#define MINOR_VERSION   7

const char meta_version[] = "metaserver-" xstr(MAJOR_VERSION.MIDDL_VERSION.MINOR_VERSION);

typedef struct {

    uint8_t *buf;
    int length;
} Buffer;

#define COPY_DATA(dst, src, len) \
    do { \
        memcpy(dst, src, len); \
        src += len; \
    } while(0);

static char g_file_name_prefix[256];

#define OUTPUT_PATH     "/sdcard/tee"
#define LOG_FILENAME    "KPHA.log"

#define LOG_BUF_MAX_LENGTH      255

static void mk_wd(void)
{
    int r;
    struct stat st;

    if ((r = stat(OUTPUT_PATH, &st))) {
        if ((r = mkdir(OUTPUT_PATH, 0644))) {
            return;
        }
    }

    if (S_ISDIR(st.st_mode)) {
        return;
    }
}

static void get_file_name(char *buf, int buf_size, const char *filename)
{
    //TODO check for buffer length
    snprintf(buf, buf_size, "%s/%s", OUTPUT_PATH, filename);
}

static void __kpha_debugprint(const char *func, int lineno, const char *prefix, const char *fmt, ...)
{
    FILE *file;
    va_list va_args;

    /* OUTPUT_PATH/LOG_FILENAME */
    char logfile[sizeof(OUTPUT_PATH) + sizeof(LOG_FILENAME)];
    char logbuf[LOG_BUF_MAX_LENGTH + 1] = {0};

    get_file_name(logfile, sizeof(logfile), LOG_FILENAME);

    va_start(va_args, fmt);
    vsnprintf(logbuf, LOG_BUF_MAX_LENGTH + 1, fmt, va_args);
    va_end(va_args);

    META_LOG("[%s]<%s:%d>: %s", prefix, func, lineno, logbuf);

    if ((file = fopen(logfile, "a")) == NULL) {
        return;
    }

    fprintf(file, "[%s]<%s:%d>: %s\n", prefix, func, lineno, logbuf);
    fclose(file);
}

std::unordered_map<uint64_t, uint32_t> cmd_resps;

/* raw means pass the raw output buffer back to client */
static int __runcmd(const char *p, char *meta_resp, size_t meta_resp_size, int *r, bool raw)
{
    FILE *fp;

    int size;
    char linebuf[LOG_BUF_MAX_LENGTH];

    std::string resp;
    if (!raw) {
        resp.assign(p);
        resp += ":\n";
    }

    char *s_resp;

    if (p == NULL || meta_resp == NULL || r == NULL) {
        kpha_err("NULL cmd buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (meta_resp == NULL) {
        kpha_err("NULL meta_resp buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (r == NULL) {
        kpha_err("NULL ret");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (meta_resp_size < sizeof(meta_buf_header_t)) {
        kpha_err("Expecting buffer to be larger/equal than %zuB, got %ZuB",
            sizeof(meta_buf_header_t));
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((fp = popen(p, "r")) == NULL) {
        kpha_err("popen(%s) failed with %s(%d)", p, strerror(errno), errno);
        return KPHA_ERR_POSIX(errno);
    }

    while (fgets(linebuf, LOG_BUF_MAX_LENGTH, fp)) {
        resp += linebuf;
    }

    *r = pclose(fp);

    if (*r == -1) {
        kpha_err("pclose(%s) failed with %s(%d)", p, strerror(errno), errno);
        return KPHA_ERR_POSIX(errno);
    }

    if (raw || *r != 0) {
        uint64_t buf_ptr = 0ULL;
        uint32_t buf_size = static_cast<uint32_t>(resp.length() + 1);
        meta_buf_header_t *mbh = reinterpret_cast<meta_buf_header_t *>(meta_resp);

        if (*r)
            *r = WEXITSTATUS(*r);

        kpha_log("pclose(%s) exited with %d", p, *r);

        if ((s_resp = strdup(resp.c_str())) == NULL) {
            kpha_err("dup string length=%zu failed", resp.length());

            /* just throw away this buffer if we failed to dup it.
             * we cannot let a malloc failure to interrupt commands'
             * execution result
             */
            buf_ptr = 0;
            buf_size = 0;
        } else {
            buf_ptr = reinterpret_cast<uint64_t>(s_resp);

            cmd_resps.insert(std::unordered_map<uint64_t, uint32_t>
                ::value_type(buf_ptr, buf_size));
        }

        mbh->bufsize = buf_size;
        mbh->bufptr = buf_ptr;
    }

err:
    return 0;
}

static int runcmd(const char *p, char *meta_resp, size_t meta_resp_size, bool raw = false)
{
    int cmd_r, r;

    if (meta_resp_size < sizeof(meta_buf_header_t)) {
        kpha_err("Invalid meta response buffer size: %u", meta_resp_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    /* the code guarantees that meta_resp is always
     * larger than sizeof(meta_buf_header_t) */
    r = __runcmd(p, meta_resp, sizeof(meta_buf_header_t), &cmd_r, raw);

    if (r)
        return r;

    if (!raw)
        return cmd_r == 0 ? 0 : KPHA_ERR_EXEC(cmd_r);

    ((meta_buf_header_t *) meta_resp)->ret = cmd_r;
    return 0;
}

static void dropcmdresp(uint64_t bufptr)
{
    std::unordered_map<uint64_t, uint32_t>::const_iterator it;
    if ((it = cmd_resps.find(bufptr)) != cmd_resps.end()) {
        void *p = reinterpret_cast<void *>(bufptr);
        cmd_resps.erase(it);
        free(p);
    }
}

static int __readcmdresp(char *buf, uint64_t bufptr, uint32_t *size)
{
    std::unordered_map<uint64_t, uint32_t>::const_iterator it;
    uint32_t resp_size;
    void *p;

    if ((it = cmd_resps.find(bufptr)) == cmd_resps.end()) {
        kpha_err("Invalid bufptr (0x%lx, %u)", bufptr, *size);
        return KPHA_ERR_CUSTOM;
    }

    p = reinterpret_cast<void *>(bufptr);

    if ((resp_size = it->second) > *size) {
        kpha_err("Buf 0x%lx expecting size %u, get resp_size %u",
            bufptr, size, resp_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    memcpy(buf, p, resp_size);
    *size = resp_size;

    cmd_resps.erase(it);
    free(p);
    return 0;
}

/* Android should allow unaligned memory access */
#pragma pack(1)

typedef struct {
    uint64_t bufptr;
    uint32_t size;
} readcmdresp_param_t;

#pragma pack()

static int readcmdresp(char *buf, int *size, char **resp)
{
    int r;
    char *p;

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (size == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (*size < static_cast<int>(sizeof(readcmdresp_param_t))) {
        kpha_err("Expecting buffer to be larger than/equal to %zuB, got %dB",
            sizeof(readcmdresp_param_t), *size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    readcmdresp_param_t *readcmdresp_param = reinterpret_cast<readcmdresp_param_t *>(buf);

    if ((p = (char *) malloc(readcmdresp_param->size)) == NULL) {
        kpha_err("Failed malloc %u B", readcmdresp_param->size);
        return KPHA_ERR_OUT_OF_MEMORY;
    }

    if ((r = __readcmdresp(p, readcmdresp_param->bufptr, &readcmdresp_param->size))) {
        free(p);
        return r;
    }

    *resp = p;
    *size = (int) readcmdresp_param->size;

    return 0;
}

static int write_file(char *filename, char *buf_in, int buf_size)
{
    int fd, size, ret = KPHA_ERR_SUCCESS;

    if ((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
        kpha_err("open %s failed with %s(%d)", filename, strerror(errno), errno);
        return KPHA_ERR_POSIX(errno);
    }

    if ((size = write(fd, buf_in, buf_size)) != buf_size) {
        if (size == -1) {
            kpha_err("write %s failed with %s(%d)", filename, strerror(errno), errno);
            ret = KPHA_ERR_POSIX(errno);
            goto exit;
        } else {
            kpha_err("Expecting to write %dB, wrote %ZdB", buf_size, size);
            ret = KPHA_ERR_BAD_WRITE;
            goto exit;
        }
    }

exit:
    close(fd);
    return ret;
}

static int read_file(char *filename, char *buf_out, int *buf_size)
{
    int fd = 0, size = 0, ret = KPHA_ERR_SUCCESS;

    if (filename == NULL || buf_out == NULL || buf_size == 0) {
        kpha_err("Bad filename %p buf_out %p pbuf_size %p",
                filename, buf_out, buf_size);
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (*buf_size == 0) {
        kpha_err("buf_size == 0!");
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((fd = open(filename, O_RDONLY)) < 0) {
        kpha_err("open %s failed with %s(%d)", filename, strerror(errno), errno);
        return KPHA_ERR_POSIX(errno);
    }

    size = read(fd, buf_out, *buf_size);
    if (size < 0 && (strstr(filename, "device.finish") == NULL)) {
        kpha_err("read %s failed with %s(%d)", filename, strerror(errno), errno);
        ret = KPHA_ERR_POSIX(errno);
        goto exit;
    }

    *buf_size = size;
exit:
    close(fd);
    return ret;
}

static uint8_t decimal2hex(uint8_t b)
{
    if (b <= 9) {
        return b + '0';
    } else if (b >= 10 && b <= 15) {
        return b - 10 + 'a';
    } else {
        return 0xff;
    }
}

static int convert_bytes_to_hex(Buffer *buffer, char **hex_result)
{
    uint32_t length = buffer->length * 2 + 1;
    int i = 0, j = 0;
    uint8_t b = 0;

    *hex_result = (char *) malloc(length);

    if (*hex_result == NULL) {
        return KPHA_ERR_OUT_OF_MEMORY;
    }

    for (i = 0, j = 0; i < buffer->length; ++i, j += 2) {
        b = buffer->buf[i];
        (*hex_result)[j] = decimal2hex(b >> 4);
        (*hex_result)[j + 1] = decimal2hex(b & 0xf);
    }

    (*hex_result)[length - 1] = '\0';

    return 0;
}

static int get_rid(char *buf, int capacity, int *used)
{
    char *hex_str = NULL;
    int ret;

    int fd = open("/proc/rid", O_RDONLY);
    if (fd < 0) {
        kpha_err("Open /proc/rid failed with %s(%d)", strerror(errno), errno);
        return KPHA_ERR_POSIX(errno);
    }

    *used = read(fd, buf, capacity);
    Buffer to_conv = {
        (uint8_t*) buf,
        *used,
    };

    close(fd);

    if ((ret = convert_bytes_to_hex(&to_conv, &hex_str))) {
        kpha_err("convert bytes to hex fail with %d", ret);
        return ret;
    }

    *used = strlen(hex_str);
    if (*used + 1 > capacity) {
        kpha_err("Expecting buffer to be larger than %dB, got %dB", *used, capacity);
        free(hex_str);
        return KPHA_ERR_SHORT_BUFFER;
    } else {
        strcpy(buf, hex_str);
        free(hex_str);
        return 0;
    }
}

/* used does not include trailing '\0' */
static int get_serialno(char *buf, int capacity, int *used)
{
    int r;
    char serialno[PROP_VALUE_MAX];

    if ((r =  __system_property_get("ro.serialno", serialno)) < 0) {
        kpha_err("getprop(ro.serialno) failed with %d", r);
        return KPHA_ERR_POSIX(r);
    }

    if (r == 0) {
        kpha_err("Cannot find ro.serialno. Use fake one.");
        /* TKCOREOS00000000 */
        strcpy(serialno, "7CC02E0500000000");
        r = strlen(serialno);
    }

    *used = r;

    if (capacity <= *used) {
        kpha_err("Expecting %dB, got %dB", *used, capacity);
        return KPHA_ERR_SHORT_BUFFER;
    }

    strcpy(buf, serialno);
    return 0;
}

static int parse_buf_and_write_file(char *buf, int buf_size)
{
    int filename_len = 0;
    int value_len = 0;
    int ret = 0;
    char *filename = NULL;
    char *value = NULL;
    char *p = buf;
    char path_file_buf[64] = { 0 };

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (buf_size < (int) sizeof(int)) {
        kpha_err("Expecting buffer larger than %zuB, got %dB", sizeof(int), buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_DATA(&filename_len, p, sizeof(int));

    if (filename_len + 1 <= 0) {
        kpha_err("Bad filename_len %zuB", filename_len + 1);
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if ((filename = (char *) malloc(filename_len + 1)) == NULL) {
        kpha_err("Bad malloc filename %dB", filename_len);
        ret = KPHA_ERR_OUT_OF_MEMORY;
        goto err;
    }
    memset(filename, 0, filename_len + 1);

    if (buf_size < (int) (sizeof(int) + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB", sizeof(int) + filename_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(filename, p, filename_len);

    if (buf_size < (int) (sizeof(int) * 2 + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB",
                sizeof(int) * 2 + filename_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }

    COPY_DATA(&value_len, p, sizeof(int));

    if ((value = (char *) malloc(value_len + 1)) == NULL) {
        kpha_err("Bad malloc for value %dB", value_len + 1);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }

    memset(value, 0, value_len + 1);

    if (buf_size < (int) (sizeof(int) * 2 + filename_len + value_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB",
                sizeof(int) * 2 + filename_len + value_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }

    COPY_DATA(value, p, value_len);

    get_file_name(path_file_buf, 64, filename);

    if ((ret = write_file(path_file_buf, value, value_len))) {
        kpha_err("write_file() failed");
        goto err;
    }

    ret = 0;
err:
    free(filename);
    free(value);
    return ret;

}

static int parse_buf_and_import_config(char *buf, int buf_size)
{
    int filename_len = 0;
    char *filename = NULL;
    const char *comm_prefix = "kph -c import_config -i ";
    char *p = buf;
    char comm[256] = { 0 };

    int ret = 0;

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (buf_size < (int) sizeof(int)) {
        kpha_err("Expecting buffer larger than %zuB, got %dB", sizeof(int), buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    COPY_DATA(&filename_len, p, sizeof(int));

    if ((filename = (char *) malloc(filename_len + 1)) == NULL) {
        kpha_err("Bad malloc filename %dB", filename_len);
        ret = KPHA_ERR_OUT_OF_MEMORY;
        goto err;
    }
    memset(filename, 0, filename_len + 1);

    if (buf_size < (int) (sizeof(int) + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB", sizeof(int) + filename_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(filename, p, filename_len);

    memcpy(comm, comm_prefix, strlen(comm_prefix));
    strcat(comm, filename);

    ret = runcmd(comm, buf, buf_size);

 err:
    free(filename);
    return ret;
}

static int parse_buf_and_import_ta_data(char *buf, int buf_size)
{
    char *filename = NULL;
    int filename_len = 0;
    int number = -1;
    int number_len = 0;
    int ret = 0;
    char *p = buf;
    const char *comm_prefix = "kph -c import_data -i ";
    char comm[256] = { 0 };

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (buf_size < (int) sizeof(int)) {
        kpha_err("Expecting buffer larger than %zuB, got %dB", sizeof(int), buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    COPY_DATA(&filename_len, p, sizeof(int));

    if ((filename = (char *) malloc(filename_len + 1)) == NULL) {
        kpha_err("Bad malloc filename %dB", filename_len);
        ret = KPHA_ERR_OUT_OF_MEMORY;
        goto err;
    }
    memset(filename, 0, filename_len + 1);

    if (buf_size < (int) (sizeof(int) + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB", sizeof(int) + filename_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(filename, p, filename_len);

    if (buf_size < (int) (sizeof(int) * 2 + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB",
                sizeof(int) * 2 + filename_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(&number_len, p, sizeof(int));

    if (buf_size < (int) (sizeof(int) * 2 + filename_len + number_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB",
                sizeof(int) * 2 + filename_len + number_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(&number, p, number_len);

    /* FIXME check return value of sprintf */
    sprintf(comm, "%s%s -n %d", comm_prefix, filename, number);

    ret = runcmd(comm, buf, buf_size);

err:
    free(filename);
    return ret;
}


static int parse_buf_and_generate_key(char *buf, int buf_size)
{
    int number = -1;
    int number_len = 0;
    int ret = 0;
    const char *comm_prefix = "kph -c generate -n";
    char comm[128] = { 0 };

    char *p = buf;

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (buf_size < (int) sizeof(int)) {
        kpha_err("Expecting buffer larger than %zuB, got %dB", sizeof(int), buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_DATA(&number_len, p, sizeof(int));

    if (buf_size < (int) (sizeof(int) + number_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB", sizeof(int) + number_len, buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_DATA(&number, p, number_len);

    /*FIXME check rv of sprintf */
    sprintf(comm, "%s %d", comm_prefix, number);
    return runcmd(comm, buf, buf_size);
}

static int parse_buf_and_import_ta_key(char *buf, int buf_size)
{
    char *filename = NULL;
    int filename_len = 0;
    int number = -1;
    int number_len = 0;
    int ret = 0;
    char *p = buf;
    char comm[256] = { 0 };

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (buf_size < (int) sizeof(int)) {
        kpha_err("Expecting buffer larger than %zuB, got %dB", sizeof(int), buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_DATA(&filename_len, p, sizeof(int));

    if ((filename = (char *) malloc(filename_len + 1)) == NULL) {
        kpha_err("Bad malloc filename %dB", filename_len);
        ret = KPHA_ERR_OUT_OF_MEMORY;
        goto err;
    }
    memset(filename, 0, filename_len + 1);

    if (buf_size < (int) (sizeof(int) + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB", sizeof(int) + filename_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(filename, p, filename_len);

    if (buf_size < (int)(sizeof(int) * 2 + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB",
                sizeof(int) * 2 + filename_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(&number_len, p, sizeof(int));

    if (buf_size < (int)(sizeof(int) * 2 + filename_len + number_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB",
                sizeof(int) * 2 + filename_len + number_len, buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(&number, p, number_len);

    /*FIXME check return value of sprintf */
    sprintf(comm, "kph -c import_key -n %d -i %s", number, filename);
    ret = runcmd(comm, buf, buf_size);

err:
    free(filename);
    return ret;
}

static int parse_buf_and_export_key(char *buf, int buf_size)
{
    int number = -1;
    int number_len = 0;
    int ret = 0;
    char comm[128] = { 0 };
    char *p = buf;

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (buf_size < (int) sizeof(int)) {
        kpha_err("Expecting buffer larger than %zuB, got %dB", sizeof(int), buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_DATA(&number_len, p, sizeof(int));

    if (buf_size < (int)(sizeof(int) + number_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB", sizeof(int) + number_len, buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_DATA(&number, p, number_len);

    sprintf(comm, "kph -c export_key -n %d -o " OUTPUT_PATH "/", number);

    return runcmd(comm, buf, buf_size);
}

static int parse_buf_and_write_file_to_pc(char *buf, int *buf_size)
{
    int filename_len = 0;
    char *filename = NULL;
    char file_path_name[256] = { 0 };
    int ret = 0;
    char *p = buf;

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (*buf_size < (int) sizeof(int)) {
        kpha_err("Expecting buffer larger than %zuB, got %dB", sizeof(int), *buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_DATA(&filename_len, p, sizeof(int));

    if ((filename = (char *) malloc(filename_len + 1)) == NULL) {
        kpha_err("Bad malloc filename %dB", filename_len);
        ret = KPHA_ERR_OUT_OF_MEMORY;
        goto err;
    }
    memset(filename, 0, filename_len + 1);

    if (*buf_size < (int) (sizeof(int) + filename_len)) {
        kpha_err("Expecting buffer over buf_size %zuB, got %dB", sizeof(int) + filename_len, *buf_size);
        ret = KPHA_ERR_SHORT_BUFFER;
        goto err;
    }
    COPY_DATA(filename, p, filename_len);
    memset(buf, 0, *buf_size);

    get_file_name(file_path_name, 256, filename);

    if ((ret = read_file(file_path_name, buf, buf_size))) {
        kpha_err("read_file failed");
    }
err:
    free(filename);
    return ret;
}

static int read_file_from_pc(char *in_buf, int in_buf_size)
{
    int ret = 0;

    if (in_buf == NULL) {
        kpha_err("NULL in_buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (in_buf_size <= 0) {
        kpha_err("Bad in_buf_size: %d", in_buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((ret = parse_buf_and_write_file(in_buf, in_buf_size))) {
        kpha_err("parse_buf_and_write_file failed");
    }

    return ret;
}

static int kph_import_config(char *buf_in, int in_buf_size)
{

    int ret = 0;

    kpha_log("enter");
    if ((ret = parse_buf_and_import_config(buf_in, in_buf_size))) {
        kpha_err("kph import_config failed");
    }
    return ret;
}

static int kph_import_ta_data(char *buf_in, int in_buf_size)
{
    int ret = 0;
    kpha_log("enter");

    if ((ret = parse_buf_and_import_ta_data(buf_in, in_buf_size))) {
        kpha_err("parse_buf_and_import_ta_data failed");
    }

    return ret;
}

static int kph_gen_key(char *in_buf, int in_buf_size)
{
    int ret = 0;
    kpha_log("enter");

    if ((ret = parse_buf_and_generate_key(in_buf, in_buf_size))) {
        kpha_err("parse_buf_and_generate_key failed");
    }

    return ret;
}

static int kph_import_ta_key(char *in_buf, int in_buf_size)
{
    int ret = 0;
    kpha_log("enter");

    if ((ret = parse_buf_and_import_ta_key(in_buf, in_buf_size))) {
        kpha_err("parse_buf_and_import_ta_key failed");
    }

    return ret;
}

static int kph_export_key(char *in_buf, int in_buf_size)
{
    int ret = 0;
    kpha_log("enter");

    if ((ret = parse_buf_and_export_key(in_buf, in_buf_size))) {
        kpha_err("parse_buf_and_export_key failed");
    }

    return ret;
}

static int write_file_to_pc(char *buf_in_out, int *buf_len)
{
    int ret;
    ret = parse_buf_and_write_file_to_pc(buf_in_out, buf_len);
    if (ret < 0) {
        kpha_err("parse_buf_and_write_file_to_pc failed", ret);
    }
    return ret;
}

static int kph_export_device_inf(char *buf, uint32_t buf_size)
{

    int ret = 0;
    kpha_log("enter");

    if (buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (buf_size <= 0) {
        kpha_err("Bad buf_size %d", buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((ret = runcmd("kph -c export_device -o " OUTPUT_PATH "/", buf, buf_size))) {
        kpha_err("kph export DEVICE_INF failed");
    }

    return ret;
}

/*
 * out_buf format:
 * +-------------------------------------------------------+
 * |  serialno length | serialno | chipid length | chipid  |
 * +-------------------------------------------------------+
 *
 **/
static int get_sn_chipid(char *out_buf, int *out_buf_len)
{
    int ret;
    int size = *out_buf_len;
    int attr_length = 0;
    int used = 0;
    char *tmp_buf = NULL;
    char *p = NULL;
    p = out_buf;

    if (out_buf == NULL) {
        kpha_err("NULL buf");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (out_buf_len == NULL) {
        kpha_err("NULL out_buf_len");
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if ((tmp_buf = (char *) malloc(*out_buf_len)) == NULL) {
        kpha_err("Bad malloc filename %dB", *out_buf_len);
        return KPHA_ERR_OUT_OF_MEMORY;
    }
    memset(tmp_buf, 0, *out_buf_len);

    if ((ret = get_serialno(tmp_buf, size, &attr_length))) {
        kpha_err("get_serialno failed");
        free(tmp_buf);
        return ret;
    }

#define COPY_BUF(dst, sour, len)  \
    do { \
        memcpy(dst, &len, sizeof(int)); \
        dst += sizeof(int); \
        memcpy(dst, sour, len); \
        dst += len; \
    }while(0);

    used += attr_length + sizeof(int);
    if (*out_buf_len < used) {
        kpha_err("Expecting buffer to be larger than %dB, got %dB", used, *out_buf_len);
        free(tmp_buf);
        return KPHA_ERR_SHORT_BUFFER;
    }
    COPY_BUF(p, tmp_buf, attr_length);
    memset(tmp_buf, 0, *out_buf_len);

    if ((ret = get_rid(tmp_buf, size, &attr_length))) {
        kpha_err("get_rid failed");
        free(tmp_buf);
        return ret;
    }

    used += attr_length + sizeof(int);
    if (*out_buf_len < used) {
        kpha_err("Expecting buffer to be larger than %dB, got %dB", used, *out_buf_len);
        free(tmp_buf);
        return KPHA_ERR_SHORT_BUFFER;
    }

    COPY_BUF(p, tmp_buf, attr_length);
    free(tmp_buf);
    *out_buf_len = used;
    return 0;
}

static int check_device_init_status(char *in_buf, int *in_buf_size)
{
    pid_t status = 0;
    int ret = 0;

    if (in_buf == NULL || in_buf_size == NULL) {
        kpha_err("NULL in_buf %p or NULL in_buf_size %p", in_buf, in_buf_size);
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (*in_buf_size < (int) sizeof(int)) {
        kpha_err("Bad in_buf size %d", *in_buf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    ret = runcmd("kph -c check_init_status", in_buf, *in_buf_size);

    if (ret && KPHA_ERR_TYPE(ret) == KPHA_ERR_EXEC_BASE) {
        int cmd_r = KPHA_ERR_SUBERR(ret);
        if (cmd_r < 0) {
            kpha_err("kph check_keystate failed");
            return ret;
        } else {
            /* It's valid for "kph -c check_keystate" to return 1/2 */
            meta_buf_header_t *mbh = reinterpret_cast<meta_buf_header_t *>(in_buf_size);
            dropcmdresp(mbh->bufptr);
        }
    }

    memcpy(in_buf, &ret, sizeof(int));
    *in_buf_size = sizeof(int);

    kpha_log("succeeds");
    return 0;
}

static int get_version(char *inbuf, int *len)
{
    if (*len < 3) {
        return KPHA_ERR_SHORT_BUFFER;
    }

    inbuf[0] = (char) MAJOR_VERSION;
    inbuf[1] = (char) MIDDL_VERSION;
    inbuf[2] = (char) MINOR_VERSION;

    *len = 3;

    return 0;
}

static int wait_for_device(char *buf, int len)
{
    (void) buf;
    (void) len;
    /* currently not implemented */
    return 0;
}


static int get_device_pl_status(char *buf, int *len)
{
    int r, digest_ok;
    uint32_t version, verify, authorize, rpmbvalid, randtoken;
    struct certificate_desc certdesc;

    uint8_t digest[32];
    uint32_t digest_size = sizeof(digest);

    if (*len < 6 * (int) sizeof(uint32_t) + 32) {
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((r = pl_device_get_cert_info(&certdesc, NULL))) {
        kpha_err("pl_device_get_cert_info failed with %d", r);
        return KPHA_ERR_POSIX(r);
    }

    if ((r = pl_device_get_status(&version, &verify, &authorize,
        &rpmbvalid, &randtoken, NULL))) {
        kpha_err("pl_device_get_status failed with %d", r);
        return KPHA_ERR_POSIX(r);
    }

    if ((digest_ok = pl_device_get_cert_digest(digest, &digest_size))) {
        kpha_err("Get cert_digest failed with 0x%x", r);
    }

    {
        char *p = buf;

        memcpy(p, &version, sizeof(uint32_t));
        p += sizeof(uint32_t);

        memcpy(p, &verify, sizeof(uint32_t));
        p += sizeof(uint32_t);

        memcpy(p, &rpmbvalid, sizeof(uint32_t));
        p += sizeof(uint32_t);

        memcpy(p, &randtoken, sizeof(uint32_t));
        p += sizeof(uint32_t);

        memcpy(p, &certdesc.version, sizeof(uint32_t));
        p += sizeof(uint32_t);

        memcpy(p, &certdesc.cert_type, sizeof(uint32_t));
        p += sizeof(uint32_t);

        if (digest_ok == 0) {
            /* Additionally copy cert digest if
               it is available */
            memcpy(p, digest, digest_size);
            p += digest_size;
        }

        *len = (int) (p - buf);
    }

    return 0;
}

static int get_device_truststore(char *buf, int *len)
{
    int r;
    uint32_t type, status, version = 1U;

    if (*len < 3 * (int) sizeof(uint32_t)) {
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((r = pl_device_get_truststore(&type, &status, &version))) {
        kpha_err("pl_device_get_truststore failed with %d", r);
        return KPHA_ERR_POSIX(r);
    }

    {
        char *p = buf;

        memcpy(p, &version, sizeof(uint32_t));
        p += sizeof(uint32_t);

        memcpy(p, &type, sizeof(uint32_t));
        p += sizeof(uint32_t);

        memcpy(p, &status, sizeof(uint32_t));
        p += sizeof(uint32_t);

        *len = (int) (p - buf);
    }

    return 0;
}


/* [ certbuf size ] [ hash size] [ certbuf ] [ hash ] */
static int program_cert(char *inbuf, int *len)
{
    int r;
    uint32_t cert_type;
    uint32_t certbuf_size, hashbuf_size;
    char *certbuf, *hashbuf;

    if (*len < 8) {
        kpha_err("got inbuf len=%d", *len);
        return KPHA_ERR_SHORT_BUFFER;
    }

    memcpy(&certbuf_size, inbuf, sizeof(uint32_t));

    memcpy(&hashbuf_size, inbuf + sizeof(uint32_t),
        sizeof(uint32_t));

    if ((uint32_t) (*len - 8) < certbuf_size) {
        kpha_err("got inbfu len=%d, expecting certbuf_size=%u", *len, certbuf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((uint32_t) (*len - 8) - certbuf_size < hashbuf_size) {
        kpha_err("got inbfu len=%d, expecting certbuf_size=%u hashbuf_size=%u", *len, certbuf_size, hashbuf_size);
        return KPHA_ERR_SHORT_BUFFER;
    }

    certbuf = inbuf + 8;
    hashbuf = inbuf + 8 + certbuf_size;

    if ((r = pl_device_program_cert(certbuf, certbuf_size, hashbuf, hashbuf_size, &cert_type))) {
        kpha_err("program cert failed with %d. certbuf_size=%u hashbuf_size=%u", r, certbuf_size, hashbuf_size);
        return KPHA_ERR_POSIX(r);
    }

    memcpy(inbuf, &cert_type, sizeof(uint32_t));
    *len = sizeof(uint32_t);

    kpha_log("succeeds");

    return 0;
}

static int revoke_cert(char *inbuf, int len)
{
    int r;
    if ((r = pl_device_revoke_cert(inbuf, len))) {
        kpha_err("revoke cert failed with %d.", r);
        return KPHA_ERR_POSIX(r);
    }

    kpha_log("succeeds");
    return 0;
}

static int valid_cmd_buf(char *cmdbuf)
{
    uint32_t i;
    const char *candidate_cmd[] = { "kph ", "pld ", "getprop " };

    for (i = 0; i < sizeof(candidate_cmd) / sizeof(candidate_cmd[0]); i++) {
        if (strncmp(cmdbuf, candidate_cmd[i], strlen(candidate_cmd[i])) == 0)
            return true;
    }

    return false;
}

static int get_prop(const char *propname, char *respbuf, size_t len)
{
    int r;
    char linebuf[PROP_VALUE_MAX];

    uint64_t buf_ptr = 0ULL;
    uint32_t buf_size = 0;
    char *s_resp;

    meta_buf_header_t *mbh;

    if (len < sizeof(meta_buf_header_t)) {
        kpha_err("Invalid meta response buffer size: %u", len);
        return KPHA_ERR_SHORT_BUFFER;
    }

    if ((r =  __system_property_get(propname, linebuf)) < 0) {
        kpha_err("Failed to getprop(%s) with %d", propname, r);
        return KPHA_ERR_POSIX(r);
    }

    if ((s_resp = strdup(linebuf)) == NULL) {
        kpha_err("dup string length=%zu failed", r);
    } else {
        buf_ptr = reinterpret_cast<uint64_t>(s_resp);
        buf_size = static_cast<uint32_t>(r + 1);
        cmd_resps.insert(std::unordered_map<uint64_t, uint32_t>
            ::value_type(buf_ptr, buf_size));
    }

    mbh = reinterpret_cast<meta_buf_header_t *>(respbuf);

    mbh->bufsize = buf_size;
    mbh->bufptr = buf_ptr;
    mbh->ret = 0;

    return 0;
}

static int generic_execute(char *inbuf, int *len)
{
    int r;
    char *p;

    if (*len <= 1) {
        kpha_err("Invalid buffer content. Unexpected cmdlen: %d", *len);
        return KPHA_ERR_SHORT_BUFFER;
    }

    /* avoid out of bound */
    inbuf[*len - 1] = '\0';

    if (!valid_cmd_buf(inbuf)) {
        kpha_err("Invalid command buffer %s", inbuf);
        return KPHA_ERR_BAD_PARAMETERS;
    }

    if (strncmp(inbuf, "getprop", strlen("getprop")) == 0) {
        p = inbuf + strlen("getprop");

        while (*p && isspace(*p))
            ++p;

        if (*p == '\0') {
            kpha_err("Empty propname");
            return KPHA_ERR_BAD_PARAMETERS;
        }

        r = get_prop(p, inbuf, *len);
    } else {
        r = runcmd(inbuf, inbuf, *len, true);
    }

    if (r)
        return r;

    *len = sizeof(meta_buf_header_t);
    return 0;
}

/*
 * flag is 0: poweroff
 * flag is 1: reboot
 */
static int shutdown_device(char *in_buf, int in_len)
{
    int flag = -1;

    if (in_len < (int) sizeof(int)) {
        return KPHA_ERR_SHORT_BUFFER;
    }

    memcpy(&flag, in_buf, sizeof(int));

    if (flag) {
        system("reboot");
    } else {
        system("reboot -p");
    }
    return 0;
}

#include "TrustkernelExternalCommand.h"
#include "MetaPub.h"

/*FIXME several customer op use strlen(buf) instead of size as the size of buffer */
/*FIXME check several handlers which need to update the size of return buffer */
/*FIXME check several handlers which need to check the whether return value is greater than zero,
 *      instead of != zero */

int handle_trustkernel_meta_command(unsigned char cmd, char **pbuf,int *psize, unsigned char *r)
{
    int ret;
    int size = *psize;
    char *buf = *pbuf;

    kpha_log("%s: cmd=0x%02x", meta_version, cmd);

    mk_wd();

    switch(cmd) {
        case KPHA_GET_VERSION:
            CALL_META_FUNC_ARGS(get_version, buf, &size);
            break;

        case KPHA_RECV_FILE:
            CALL_META_FUNC_ARGS(read_file_from_pc, buf, size);
            break;

        case KPHA_IMPORT_CFG_FILE:
            CALL_META_FUNC_ARGS(kph_import_config, buf, size);
            break;

        case KPHA_IMPORT_TA_DATA:
            CALL_META_FUNC_ARGS(kph_import_ta_data, buf, size);
            break;

        case KPHA_IMPORT_TA_KEY:
            CALL_META_FUNC_ARGS(kph_import_ta_key, buf, size);
            break;

        case KPHA_GEN_KEY:
            CALL_META_FUNC_ARGS(kph_gen_key, buf, size);
            break;

        case KPHA_EXPORT_KEY:
            CALL_META_FUNC_ARGS(kph_export_key, buf, size);
            break;

        case KPHA_EXPORT_DEV_INF:
            CALL_META_FUNC_ARGS(kph_export_device_inf, buf, size);
            break;

        case KPHA_GET_SN_CHIPID:
            CALL_META_FUNC_ARGS(get_sn_chipid, buf, &size);
            break;

        case KPHA_SEND_FILE:
            CALL_META_FUNC_ARGS(write_file_to_pc, buf, &size);
            break;

        case KPHA_PROGRAM_CERT:
            CALL_META_FUNC_ARGS(program_cert, buf, &size);
            break;

        case KPHA_REVOKE_CERT:
            CALL_META_FUNC_ARGS(revoke_cert, buf, size);
            break;

        case KPHA_GENERIC_EXECUTE_CMD:
            CALL_META_FUNC_ARGS(generic_execute, buf, &size);
            break;

        case KPHA_GET_PL_STATUS:
            CALL_META_FUNC_ARGS(get_device_pl_status, buf, &size);
            break;

        case KPHA_GET_TRUSTSTORE:
            CALL_META_FUNC_ARGS(get_device_truststore, buf, &size);
            break;

        case KPHA_WAIT_FOR_DEVICE:
            CALL_META_FUNC_ARGS(wait_for_device, buf, size);
            break;

        case KPHA_SHUTDOWN_DEV:
            CALL_META_FUNC_ARGS(shutdown_device, buf, size);
            break;

        case KPHA_READ_CMD_RESPONSE:
            {
                int capacity;
                char *respbuf;

                capacity = size < (int) sizeof(meta_buf_header_t) ?
                    (int) sizeof(meta_buf_header_t) : size;

                if (size < capacity) {
                    if ((buf = (char *) malloc(capacity)) == NULL) {
                        /* there is no way we can set error value */
                        return -1;
                    }
                    memcpy(buf, *pbuf, size);
                    free(*pbuf);
                    *pbuf = buf;
                }

                ret = readcmdresp(buf, &size, &respbuf);

                if (ret) {
                    *psize = sizeof(meta_buf_header_t);
                    *r = KPHA_META_RESULT_FAILED;
                    FILL_META_BUF_RETURN_VALUE(buf, ret);
                } else {
                    *pbuf = respbuf;
                    *psize = size;
                    *r = KPHA_META_RESULT_SUCCESS;
                    free(buf);
                }
            }

            break;
        default:
/*            *r = KPHA_META_RESULT_BAD_COMMAND; */
            kpha_log("Unkown COMMAND 0x%x", cmd);
            return 1;
    }

    kpha_log("return=%u", *r);

    return 0;
}
