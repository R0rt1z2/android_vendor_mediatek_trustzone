# Verification of TrustKernel TKCore TEE integration
import sys
import os
import time
import lib_adb
import lib_config_file
from lib_logging import logD, logI, logW, logE, logC

teeIntegrationCheck = {'result':'UNKNOWN','tee_version':'UNKNOWN'}

Global = {}
NormalWorld = {}
SystemInfo = {}
SystemTAs = {}
SPTAs = {}
SELinux = {}
Connectivity = {}

def verify_integration(workingDirectory, dumpFilePath, secureComponents):

   logI("+--------------------------------------------------------------------------")
   logI("|                            Verify tkcore bsp integration...")
   logI("+--------------------------------------------------------------------------")

   if lib_adb.is_file_exist(dumpFilePath):
      lib_adb.cmd("rm " + dumpFilePath, check_result=False)
   
   if not lib_adb.start_activity("com.trustkernel.teeotaverifier/.MainActivity"):
      logE("Could not start tee verifier app")
      logI("+--------------------------------------------------------------------------")
      return False, teeIntegrationCheck

   time.sleep(5) # give time for checks to happen

   # broadcast intent
   lib_adb.broadcast_intent("com.trustkernel.teeotaverifier.DUMP --es filename " + dumpFilePath) 

   lib_adb.stop_activity("com.trustkernel.teeotaverifier")

   # retrieve and parse dump file
   lib_adb.pull(dumpFilePath,workingDirectory)

   #read config file
   dump_file = os.path.join(workingDirectory,os.path.basename(dumpFilePath))

   if not lib_config_file.read_file(dump_file):
      logE("Could not read tee verifier dump")
      logI("+--------------------------------------------------------------------------")
      return False, teeIntegrationCheck

   # test get_section_list
   sections = lib_config_file.get_section_list()  

   dictionary = {}

   for section in sections:
      if section == "android":
         dictionary = SystemInfo
      elif section == "global":
         dictionary = Global 
      elif section == "TEE":
         dictionary = teeIntegrationCheck
      elif section == "system ta":
         dictionary = SystemTAs
      elif section == "SP ta":
         dictionary = SPTAs
      elif section == "normal_world" or section == "files_permissions":
         dictionary = NormalWorld
      elif section.startswith("selinux"):
         dictionary = SELinux
      elif section == "connectivity":
         dictionary = Connectivity

      for option in lib_config_file.get_all_options(section):
         dictionary[option[0]]=option[1]

   logI("Global Info:")
   for k,v in Global.iteritems():
      logI(k + ':' + v)

   logI("tee Info: <version could not be detected yet>")
   
   teeIntegrationCheck['result'] = "SUCCESS"

   if not verify_normal_world():
      teeIntegrationCheck['result'] = "ERROR"

   if not verify_secure_components(secureComponents):
	   teeIntegrationCheck['result'] = "ERROR"

   logI("---------------------------------------------------------------------------")
   logI("                              Global verification result: " + teeIntegrationCheck['result'])
   logI("---------------------------------------------------------------------------")
   return True, teeIntegrationCheck

def verify_normal_world():
 
   list = []
   for k, v in NormalWorld.iteritems():
     if 'false' in v:
        list.append(k)

   logW("BUG: teed may not be detected even though it is valid, you may ignore this error")

   if not list:
      return True
   else:
      logE("Normal World misses libraries/bins or bad permissions: " + str(list))

      if SELinux['available'] == 'true' and SELinux['enforcing'] == 'true':
         logI("SELinux is activated,  and enforcing")
      return False

def verify_secure_components(componentsToCheck):

   missing = []
   ret = True

   for uuid in componentsToCheck.viewkeys():
      if  str(uuid) not in str(SystemTAs.keys()) and str(uuid) not in str(SPTAs.keys()):
         missing.append(str(componentsToCheck[uuid])+':'+str(uuid))

   if missing:
      ret = False
      for item  in missing:
         logE("Unable to find presence of Secure Components: " + item)

   return ret
