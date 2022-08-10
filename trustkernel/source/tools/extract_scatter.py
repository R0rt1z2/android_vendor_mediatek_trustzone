#!/usr/bin/python


PART_INFO = {
    "preloader" : {
        "filename" : "preloader.bin",
        "is_download": True,
        "type" : "SV5_BL_BIN",
        "region": "EMMC_BOOT_1",
        "boundary_check" : False,
        "is_reserved" : False,
        "operation_type" : "BOOTLOADERS"
    },
    "pgpt" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "proinfo" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "PROTECTED"
    },
    "nvram" : {
        "filename" : "NONE",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": False,
        "is_reserved" : False,
        "operation_type" : "BINREGION"
    },
    "protect1" : {
        "filename" : "NONE",
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": False,
        "is_reserved" : False,
        "operation_type" : "PROTECTED"
    },
    "protect2" : {
        "filename" : "NONE",
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": False,
        "is_reserved" : False,
        "operation_type" : "PROTECTED"
    },
    "lk" : {
        "filename" : "lk.bin",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "para" : {
        "filename" : "NONE",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": False,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "boot" : {
        "filename" : "boot.img",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "recovery" : {
        "filename" : "recovery.img",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "logo" : {
        "filename" : "logo.bin",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "expdb" : {
        "filename" : "NONE",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": False,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "seccfg" : {
        "filename" : "NONE",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": False,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "oemkeystore" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "secro" : {
        "filename" : "secro.img",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "keystore" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "tee1" : {
        "filename" : "trustzone.bin",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "tee2" : {
        "filename" : "trustzone.bin",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "frp" : {
        "filename" : "NONE",
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_download": False,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "nvdata" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "metadata" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "system" : {
        "filename" : "system.img",
        "is_download": True,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "cache" : {
        "filename" : "cache.img",
        "is_download": True,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "userdata" : {
        "filename" : "userdata.img",
        "is_download": True,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "UPDATE"
    },
    "flashinfo" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : False,
        "is_reserved" : True,
        "operation_type" : "RESERVED"
    },
    "sgpt" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : False,
        "is_reserved" : True,
        "operation_type" : "RESERVED"
    },
    "ztecfg" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "NORMAL_ROM",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "recovery2" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "radio" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "vendor" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "odm" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
    "oem" : {
        "filename" : "NONE",
        "is_download": False,
        "type" : "EXT4_IMG",
        "region": "EMMC_USER",
        "boundary_check" : True,
        "is_reserved" : False,
        "operation_type" : "INVISIBLE"
    },
};

f = open("part", "r")

print "############################################################################################################"
print "#"
print "#  General Setting "
print "#    "
print "############################################################################################################"
print "- general: MTK_PLATFORM_CFG"
print "  info: "
print "    - config_version: V1.1.2"
print "      platform: MT6737T"
print "      project: zte6737t_66_n"
print "      storage: EMMC"
print "      boot_channel: MSDC_0"
print "      block_size: 0x20000"
print "############################################################################################################"
print "#"
print "#  Layout Setting"
print "#"
print "############################################################################################################"

#outf = open("scatter", "w")
partidx = 2
while True:
    line = f.readline()
    if line == "":
        break
    part = line.split(" ")
    rang = part[1][1:-1].split("-")
    start = int(rang[0], 16)
    end = int(rang[1], 16)
    partname = part[2][1:-1]

#    print start, end, partname
    print "- partition_index: SYS%d" % (partidx)
    print "  partition_name: %s" % (partname)
    print "  file_name: %s" % (PART_INFO[partname]["filename"])
    print "  is_download: %s" % ("true" if PART_INFO[partname]["is_download"] else "false")
    print "  type: %s" % (PART_INFO[partname]["type"])
    if partname == "flashinfo":
        print "  linear_start_addr: 0x%X" % (0xffff0084)
    else:
        print "  linear_start_addr: 0x%x" % (start)
    if partname == "flashinfo":
        print "  physical_start_addr: 0x%X" % (0xffff0084)
    else:
        print "  physical_start_addr: 0x%x" % (start)
    if partname == "userdata":
        print "  partition_size: 0x%x" % (0x60000000)
    else:
        print "  partition_size: 0x%x" % (end - start + 1)
    print "  region: %s" % (PART_INFO[partname]["region"])
    print "  storage: HW_STORAGE_EMMC"
    print "  boundary_check: %s" % ("true" if PART_INFO[partname]["boundary_check"] else "false")
    print "  is_reserved: %s" % ("true" if PART_INFO[partname]["is_reserved"] else "false")
    print "  operation_type: %s" % (PART_INFO[partname]["operation_type"])
    print "  reserve: 0x00"
    print

    partidx += 1

f.close()
