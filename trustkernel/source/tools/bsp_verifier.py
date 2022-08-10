#!/usr/bin/env python

import time
import sys
import argparse
import os
import string
import shutil
import traceback
import socket
import errno
import re
import fnmatch

from lib import lib_integration_verification
from lib import lib_config_file
from lib import lib_process
from lib.lib_logging import logD, logI, logW, logE, logC, init_log
from lib import lib_adb

#DEFAULT_NS_BINARIES = { "teed" : "/system/bin/", \
#		"kph" : "/system/bin/", \
#		"libteec.so" : "/system/lib64/"}

DEFAULT_SYSTEM_TAS_DRIVERS = {'8b1e0e41-2636-11e1-ad9e0002a5d5c51b.ta':'IFAA', \
		'b46325e6-5c90-8252-2eada8e32e5180d6.ta':'KPH', \
		'2ea702fa-17bc-4752-b3adb2871a772347.ta':'KEYMASTER', \
		'02662e8e-e126-11e5-b86d9a79f06e9478.ta':'GATEKEEPER'}

#---------------------------------------------------------------------------
#                 MAIN class
#---------------------------------------------------------------------------
def main(argv):
   print("+--------------------------------------------------------------------------")
   print("|                            TESTing TrustKernel TEE BSP Integration Result")
   print("+--------------------------------------------------------------------------")
   sys.stdout.flush()

   init_log("log")

   lib_adb.push("lib/busybox", lib_adb.busyboxPath, check_md5=False)
   lib_adb.cmd("chmod 777 " +  lib_adb.busyboxPath)

   lib_integration_verification.verify_integration("./", "/sdcard/dump_tee_ota_verifier.txt", DEFAULT_SYSTEM_TAS_DRIVERS)
#--------------------------------------------------------------#
#             Calling the MAIN Function                        #
#--------------------------------------------------------------#
if __name__ == "__main__":
   main(sys.argv)
