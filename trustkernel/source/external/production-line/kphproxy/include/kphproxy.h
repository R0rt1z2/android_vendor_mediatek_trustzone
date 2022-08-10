#ifndef _KPHPROXY_H_
#define _KPHPROXY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TA_ID_TEE       (0U)
#define TA_ID_IFAA      (1U)
#define TA_ID_SOTER     (6U)

/* verify imported/generated key/data/config */
int verify_ta_key(uint32_t taid);

int remove_ta_key(uint32_t taid);

int verify_ta_data(uint32_t taid);
int verify_ta_data2(uint32_t taid, const char *tag);

int remove_ta_data(uint32_t taid);
int remove_ta_data2(uint32_t taid, const char *tag);

int verify_device_config(void);

int remove_device_config(void);

#define TA_DATA_HEADER_MAGIC    0x7ADA7A
#define TA_DATA_HEADER_VERSION  0x1

#define TA_DATA_PERM_PUBLIC     0x0
#define TA_DATA_PERM_TEE        0x1
#define TA_DATA_PERM_TA         0x2

struct ta_data_descriptor {
    uint32_t perm;
    uint32_t __resv[31];
} __attribute__ ((packed));

struct ta_data_header {
    uint32_t magic;
    uint32_t version;
    uint32_t head_size;
    struct ta_data_descriptor desc;
} __attribute__ ((packed));

int import_ta_data(uint32_t taid, const char *tag,
    struct ta_data_descriptor *desc, size_t desc_size, void *buf, uint32_t length);

int verify_tee_all(void);

int read_provision_state(uint32_t *state);

int write_provision_state(uint32_t state);

int overwrite_provision_state(uint32_t state);

int export_device_secret(void *prefix, uint32_t prefix_len,
    void *buf_out, uint32_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
