#ifndef TRUSTKERNEL_EXTERNAL_COMMAND_H_
#define TRUSTKERNEL_EXTERNAL_COMMAND_H_

#include "DriverInterface.h"

#define KPHA_META_PREFIX        (0xC0U)
#define KPHA_META_CMD_MASK      ((1U << 6) - 1)
#define KPHA_META_CMD(cmdid)    ((unsigned char) (KPHA_META_PREFIX | ((cmdid) & KPHA_META_CMD_MASK)))

#define KPHA_RECV_FILE          KPHA_META_CMD(0)

#define KPHA_IMPORT_CFG_FILE    KPHA_META_CMD(1)
#define KPHA_IMPORT_TA_DATA     KPHA_META_CMD(2)
#define KPHA_IMPORT_TA_KEY      KPHA_META_CMD(3)

#define KPHA_GEN_KEY            KPHA_META_CMD(4)

#define KPHA_EXPORT_KEY         KPHA_META_CMD(5)
#define KPHA_EXPORT_DEV_INF     KPHA_META_CMD(6)

#define KPHA_GET_SN_CHIPID      KPHA_META_CMD(7)
#define KPHA_SEND_FILE          KPHA_META_CMD(8)

/* newly added since version 3.0.0 */

#define KPHA_WAIT_FOR_DEVICE    KPHA_META_CMD(11)
#define KPHA_GET_VERSION        KPHA_META_CMD(12)
#define KPHA_PROGRAM_CERT       KPHA_META_CMD(13)
#define KPHA_GET_PL_STATUS      KPHA_META_CMD(14)

/* cmd15 is used by clear_keystate */
#define KPHA_GENERIC_EXECUTE_CMD    KPHA_META_CMD(16)
#define KPHA_GET_TRUSTSTORE     KPHA_META_CMD(17)
#define KPHA_REVOKE_CERT        KPHA_META_CMD(18)

/* end for version 3.x */

#define KPHA_SHUTDOWN_DEV       KPHA_META_CMD(19)
#define KPHA_READ_CMD_RESPONSE  KPHA_META_CMD(20)

int handle_trustkernel_meta_command(unsigned char cmd, char **pbuf, int *psize, unsigned char *r);

#endif

