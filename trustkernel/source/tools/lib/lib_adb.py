import lib_process
import time
import os
import hashlib
import fnmatch
import tarfile
from lib_logging import logD, logI, logW, logE, logC

adb_env_var = ""
adb_working_dir = ""
adb_target_device = None
adb_connection_type = None
# This dictionnary will contains all binaries push on the target, we 'll save their md5, their local + target location
# We can use them after to check if binairies are corrupted after a test execution
binariesDict={}

busyboxPath = '/data/app/busybox'

def set_busybox_path(path):
   global busyboxPath
   if path != busyboxPath:
      logD("busyboxPath changed from: " + busyboxPath + " to: " + path)
      busyboxPath=path


def set_connection_infos(connection_identifier,connection_type=""):
   global adb_target_device
   global adb_connection_type

   # used for routing information if we have multiple attached devices
   if connection_identifier:
      adb_target_device = connection_identifier
   if adb_connection_type!="":
      adb_connection_type = connection_type
      
   logD("connection identifier set to: " + connection_identifier)
      
#------------------------------------------------------------------------------#      
def get_adb_header():
   cmd = ['adb']
   if (adb_target_device):
      cmd.append('-s')
      cmd.append(adb_target_device)
   return cmd

def format_shell_cmd(cmd):
   global adb_env_var
   global adb_working_dir
   
   if adb_working_dir!="":
      cmd = "cd " + adb_working_dir + ";"+ cmd
      
   return  ['shell', adb_env_var + cmd]
#------------------------------------------------------------------------------#    

def non_blocking_cmd(cmd, out=None):
   logD("% "+ " ".join(cmd))
   return lib_process.start(cmd, out=out)

def blocking_cmd(cmd, check_result, timeout, nb_retry=2):
   nb_try=0
   retry=True
   
   while(retry):
      retry=False
      istimeout=False
      nb_try+=1
      
      tmp_file=None
      if  os.name=='nt':
         tmp_file=open(os.path.join("adb_tmp"),"w") 

      adb_handle = non_blocking_cmd(cmd, out=tmp_file)
   
      #check timeout if specified
      if(timeout>0):
         start_time=time.time()
         while ((not lib_process.is_ended(adb_handle)) and (time.time()-start_time)<timeout):
            time.sleep(0.1)
         
         duration = time.time()-start_time
         if(duration>=timeout):
            lib_process.kill(adb_handle)
            #timeout...
            istimeout=True
      
      if tmp_file!=None:
         tmp_file.close()
         res=""
         with open("adb_tmp", "r") as f:
            for line in f:
               res+=line
      else:
         res = lib_process.get_output(adb_handle)

      if "error: device not found" in res:
         connect()
         retry=True
         
      if "error: protocol fault" in res:
         logW("protocol fault")
         disconnet()
         time.sleep(1)
         connect()
         retry=True
         
      if((res!="" and check_result) or istimeout):
         retry=True
      
      if(nb_try>nb_retry): # '>' and not '>=' because try!=retry...
         retry=False
   
   if istimeout:
      res += "# ERROR: command timeout"
      
   if (res!="" and check_result):
      logE("##########################################################################")
      logE("# ERROR: adb command '" + " ".join(cmd) + "' returned an error:")
      logE(res)
      logE("##########################################################################")
      #exit(1)
      
   return res

#adb shell non blocking command
def run(prgm_name,prgm_param="",out=None):
   cmd = get_adb_header()
   cmd += format_shell_cmd(prgm_name+" "+prgm_param)
   return non_blocking_cmd(cmd, out=out)

#adb blocking command
def adb(param, check_result=False, timeout=300):
   cmd = get_adb_header()
   cmd += param
   return blocking_cmd(cmd, check_result, timeout)

#adb shell blocking command
def cmd(cmd, check_result=True, timeout=300):
   full_cmd=format_shell_cmd(cmd)
   return adb(full_cmd, check_result, timeout)
#------------------------------------------------------------------------------#

def install_driver(driver_name,remove_if_existing=False):
   #remove if existing
   if(remove_if_existing):
      res = cmd("lsmod", check_result=False)
      driver_name_without_ext = driver_name.split(".")[-2]
      driver_name_without_ext = driver_name_without_ext.split("/")[-1]
      
      if(res.find(driver_name_without_ext)!=-1):
         cmd("rmmod "+driver_name)
         
   cmd("insmod "+ driver_name)

def run_daemon(daemon_name,param="",remove_if_existing=False):
   #remove if existing
   if(remove_if_existing):
      pid=get_pid(daemon_name)
      if pid!=-1:
         kill_pid(pid)
          
         pid=get_pid(daemon_name)
         if pid!=-1:
            return False
            
   cmd(daemon_name+" "+param+";sleep 1") #FSY: why 'sleep 1'?????

   pid=get_pid(daemon_name)
   
   if pid==-1:
       logE("##########################################################################")
       logE("# ERROR: run_daemon failed, Daemon processes: " + daemon_name + " is not running")
       logE("##########################################################################")
       return False

   return True

def clean_env_var():
   global adb_env_var
   adb_env_var=""
   
def add_to_env_var(var_name, var_val, before=False):
   global adb_env_var
   #manage a list...
   if before:
      adb_env_var+=var_name+"="+var_val+":$"+var_name+";"
   else:
      adb_env_var+=var_name+"=$"+var_name+":"+var_val+";"

def set_working_dir(dir_name):
   global adb_working_dir
   
   adb_working_dir=dir_name
   
def get_pid(prgm_name):
   res = cmd("ps",check_result=False)
   processes = res.splitlines()
   
   pid=-1
   for line in reversed(processes):
      if(line.find(prgm_name)!=-1):
         val = line.split()
         pid = val[1]
         break

   return pid

def kill_pid(pid):
   cmd("kill -9 "+pid)
   return

def is_device_connected():
   res = adb(['get-state'])
   if not "device" in res:
      return False
   return True

def is_boot_completed():
   res = adb(['getprop sys.boot_completed'])
   if not "1" in res:
      return False
   return True

def is_file_exist(target_path):
   res = cmd('if [ -f '+target_path+' ]; then echo -n "OK"; fi;',check_result=False)
   return res=="OK"
   #res = cmd("ls "+target_path, check_result=False,timeout=300).rstrip()
   ##remove escape sequence from busybox 'ls' command...
   #ansi_escape = re.compile(r'\x1b[^m]*m')
   #res = ansi_escape.sub('', res)
   #return (res == target_path)


def is_dir_exist(dir_name):
   res = cmd('if [ -d '+dir_name+' ]; then echo -n "OK"; fi;',check_result=False)
   return res=="OK"
   
def pull(target_filename, local_path):
   if not is_file_exist(target_filename):
      logE(target_filename +" not found on the target")
      return False

   if os.path.isdir(local_path):
      local_file=os.path.join(local_path, target_filename.split("/")[-1])
   else:
      local_file=local_path
   
   if os.path.isfile(local_file):
      if md5_comparison(local_file, target_filename, output_traces=False)[0]:
         return True
   
   nbAttemptsLeft=3
   while (nbAttemptsLeft>0) :
      adb(['pull',target_filename,local_path])
      if md5_comparison(local_file, target_filename, output_traces=(nbAttemptsLeft==1))[0]:
         return True
      nbAttemptsLeft-=1
    
   logE("Pulling "+target_filename +": Failed " )
   return False
def add_binary_in_dict(filename,target_file_md5,local_dir,target_dir):
      binariesDict[filename]=[target_file_md5,local_dir,target_dir]
   
def push(local_filename, target_path, check_md5=True, timeout=300):
   
   if(not os.path.isfile(local_filename)):
      logE("%s file does not exists" % local_filename)
      return False
      
   if target_path.endswith("/"):
      target_dir=target_path[:-1]
      target_filename=os.path.split(local_filename)[1]
   elif is_dir_exist(target_path):
      target_dir=target_path
      target_filename=os.path.split(local_filename)[1]
   else:
      target_dir=os.path.split(target_path)[0]
      target_filename=os.path.split(target_path)[1]
   target_file=target_dir + "/" + target_filename
   
   cmd("mkdir -p "+target_dir)
   
   if check_md5:
      md5_comparison_result=md5_comparison(local_filename, target_file, output_traces=False)
      if md5_comparison_result[0]: #optim or not?
         # Save md5 of this binarie in binariesDict
         target_file_md5 = md5_comparison_result[1]
         local_dir=os.path.split(local_filename)[0]
         add_binary_in_dict(target_filename,target_file_md5,local_dir,target_dir)
         return True
 
   nbAttemptsLeft=3
   while (nbAttemptsLeft>0) :
      adb(['push',local_filename, target_file],timeout=timeout)
      if check_md5:
         md5_comparison_result=md5_comparison(local_filename, target_file, output_traces=(nbAttemptsLeft==1))
         if md5_comparison_result[0]:
            # Save md5 of this binarie in binariesDict
            target_file_md5 = md5_comparison_result[1]
            local_dir=os.path.split(local_filename)[0]
            add_binary_in_dict(target_filename,target_file_md5,local_dir,target_dir)
            return True
      else:
         if is_file_exist(target_file):
            return True

      nbAttemptsLeft-=1
    
   logE("Pushing "+local_filename +": Failed ")
   return False
        
def push_dir(local_dir, target_dir, timeout=300):
   global busyboxPath
   
   if(not os.path.isdir(local_dir)):
      logE("%s directory does not exists" % local_dir)
      return False
      
   if os.listdir(local_dir) ==  []:
      return True #directory is empty, no files to copy
   
   if(not target_dir.endswith("/")):
      logE("target dir '%s' should end with a /" % target_dir)
      return False
   
   #remove and create a clean target directory
   #cmd("rm -r "+target_dir)
   cmd("mkdir -p "+target_dir)
   
   #tar all the files to push
   tar = tarfile.open(os.path.join(local_dir,"out.tar.gz"), "w:gz")
   tar.add(local_dir, arcname=".")
   tar.close()
   #push them to the board
   adb(['push',os.path.join(local_dir,"out.tar.gz"), target_dir],timeout=timeout)
   cmd("cd "+target_dir+";" + busyboxPath + " tar -xzf out.tar.gz;rm out.tar.gz")
   os.remove(os.path.join(local_dir,"out.tar.gz"))
   
   #get md5 for files on target
   dict_target={}
   md5_target = cmd("cd "+target_dir+";" + busyboxPath + " md5sum *",check_result=False)
   for line in md5_target.split('\n'):
      if len(line)>0:
         line_split=line.split()
         if len(line_split)!=2:
            logE("error while geting md5 on target :")
            logE(md5_target)
            return False
         dict_target[line_split[1]]=line_split[0]
   
   #compare with md5 of local files
   for local_file in os.listdir(local_dir):
      local_file_md5_hash=hashlib.md5(file(os.path.join(local_dir,local_file),"rb").read()).hexdigest()
      if not local_file in dict_target:
         logE("file not found on target:"+local_file)
         return False
      
      if(local_file_md5_hash!=dict_target[local_file]):
         logE("hash is different for '"+local_file+"' file: local="+local_file_md5_hash+" target="+dict_target[local_file])
         return False
         
      # Save md5 of this binarie in binariesDict
      add_binary_in_dict(local_file,local_file_md5_hash,local_dir,target_dir)
      
   return True
   
def check_md5_list(pattern="*"):
   global busyboxPath
   
   #start_time=time.time()
   for key in binariesDict.viewkeys():
      if fnmatch.fnmatch(key, pattern):
         values=binariesDict[key]
         md5_hash,local_dir,target_dir=values[0],values[1],values[2]
         
         target_filename=os.path.join(target_dir,key)
         if is_file_exist(target_filename):
            current_md5_hash= cmd(busyboxPath + " md5sum "+target_filename,check_result=False,timeout=300).split()[0]
         else:
            current_md5_hash= "0" # default value if file not present
            
         if current_md5_hash!=md5_hash:
            logW("Warning : The following file was corrupted "+key)
            logW("Its current md5 hash is:"+ current_md5_hash+", but md5 hash of the original file is :"+md5_hash)
            push(os.path.join(local_dir,key), target_dir, check_md5=True)
   #duration=time.time()-start_time
   #logD("Check_md5_list function takes "+str(duration)+" s")

def display_all_files_pushed_on_target():     
   logD("---------------------------------------------Files pushed on target----------------------------------------------------------")
   logD('{0:40}  {1:35s}  {2:40s}'.format("Files","MD5_hash","Location"))
   for key in sorted(binariesDict.viewkeys()):
      if not key.endswith(".rsp"):
         logD('{0:40}  {1:35s}  {2:40s}'.format(key, binariesDict[key][0],binariesDict[key][2]))
   logD("-----------------------------------------------------------------------------------------------------------------------------")

def was_pushed_to_target(filename):
   if filename in binariesDict:
      return True
   else:
      return False

def get_push_location(filename):
   if filename in binariesDict:
      return binariesDict[filename][2]
   else:
      return ''

def pull_dir(target_dir, local_dir):
   global busyboxPath
      
   if not is_dir_exist(target_dir):
      logE("target %s directory does not exists" % target_dir)
      return False
      
   if(not os.path.isdir(local_dir)):
      os.makedirs(local_dir)
         
   #tar all the files to pull
   #cmd("cd "+target_dir+";" + busyboxPath + " tar -czf out.tar.gz *")
   cmd("cd "+target_dir+";" + busyboxPath + " find . -maxdepth 1 -type f -print0 | " + busyboxPath + " xargs -0 " + busyboxPath + " tar -czf out.tar.gz")
   adb(['pull',target_dir+"/out.tar.gz" , local_dir])
   cmd("cd "+target_dir+";rm out.tar.gz")
   
   tar = tarfile.open(os.path.join(local_dir,"out.tar.gz"), "r:gz")
   tar.extractall(local_dir)
   tar.close()
   
   os.remove(os.path.join(local_dir,"out.tar.gz"))
   
   #get md5 for files on pc
   dict_local={}
   for local_file in os.listdir(local_dir):
      if os.path.isfile(os.path.join(local_dir,local_file)):
         local_file_md5_hash=hashlib.md5(file(os.path.join(local_dir,local_file),"rb").read()).hexdigest()
         dict_local[local_file]=local_file_md5_hash
   
   md5_target = cmd("cd "+target_dir+";" + busyboxPath + " md5sum *",check_result=False)
   for line in md5_target.split('\n'):
      if len(line)>0:
         line_split=line.split()
         if len(line_split)!=2:
            logE("error while geting md5 on target :")
            logE(md5_target)
            return False
         
         if not line_split[1] in dict_local:
            logE("file not found on host:"+line_split[1])
            return False
         
         if(line_split[0]!=dict_local[line_split[1]]):
            logE("hash is different for '"+line_split[1]+"' file: target="+line_split[0]+" local="+dict_local[line_split[1]])
            return False
         
   return True
               
def reboot():
   logI("Adb reboot the device:")
   adb(['reboot'])
   logI("{:>75}".format("SUCCESS"))
   
def wait_device(): #FSY: add timeout?
   logI("Waiting for the device:")
   adb(['wait-for-device'])
   logI("{:>75}".format("SUCCESS"))

def root():
   logI("Set in root mode")
   adb(['root'])
   wait_device()

def is_adb_root():
   res = cmd('id',check_result=False)
   return "uid=0(root)" in res

def remount():
   logI("Remount")
   adb(['remount'])
   wait_device()
   
def connect():
   global adb_target_device
   global adb_connection_type
   if "ethernet" in str(adb_connection_type): 
      adb(['connect',adb_target_device])
      wait_device()
      #nothing is possible if not in ethernet mode

            
def disconnet():
   global adb_target_device
   global adb_connection_type
   if adb_connection_type!=None: 
      if "ethernet" in adb_connection_type:
         adb(['disconnect',adb_target_device])
      #nothing is possible if not in ethernet mode

def md5_comparison(local_file, target_file, output_traces=True):
   global busyboxPath
   
   if not os.path.exists(local_file):
      return [False, False]
      
   local_file_md5_hash=hashlib.md5(file(local_file,"rb").read()).hexdigest()
   
   res = cmd(busyboxPath + " md5sum "+target_file,check_result=False,timeout=300)
   md5_res = res.split()
   if len(md5_res)!=2:
      if output_traces:
         logE("md5sum error:\n"+res)
      return [False,False]
   
   if md5_res[1] != target_file:
      if output_traces:
         logE("md5sum error:\n"+res)
      return [False,False]
   
   if md5_res[0]!=local_file_md5_hash:
      if output_traces:
         logE("hash is different between '"+local_file+"' and '"+target_file+"': local="+local_file_md5_hash+" target="+md5_res[0])
      return [False,False]
   
   return [True,md5_res[0]]
   
def install_apk(apk_name, allow_downgrade=False):
   if allow_downgrade:
      result=adb(['install','-r','-d',apk_name])
   else:
      result=adb(['install','-r',apk_name])

   if "Success" in result:
      return True
   else:
      # Printing full adb install command
      logE("  adb install -r (-d)" + apk_name + " failed.")
      logE(result)
      return False
   
def remove_package(package_name):
   result=adb(['uninstall',package_name])
   if "Success" in result:
      return True
   else:
      # Printing full adb uninstall command
      logE("  adb uninstall " + package_name + " failed.")
      logE(result)
      return False
   
def stop_service(service_name):
   cmd("am force-stop "+service_name,check_result=False)

def start_service(service, remove_if_existing=False):
   package_name=service.split('/.')[0]
   pid=get_pid(package_name)

   if pid!=-1:
      #remove if existing
      if(not remove_if_existing):
         return True # service is already running, and no re-start request

      # kill and attempt to restart...
      kill_pid(pid)

   result = cmd("am startservice "+service,check_result=False)

   if "Error-" in result:
      logE("##########################################################################")
      logE("# ERROR: Starting service " + service + ": " + result)
      logE("##########################################################################")
      return False

   time.sleep(5) # give the service time to start

   if len(package_name) > 15:
      package_name=package_name[-15:] # take COMM value

   pid=get_pid(package_name)
   if pid==-1:
      logE("##########################################################################")
      logE("# ERROR: Starting service " + service + ": " + result)
      logE("##########################################################################")
      return False

   return True      
   
def start_activity(component, category="android.intent.category.LAUNCHER",action="android.intent.action.MAIN"):
   result=cmd("am start -a " + action + " -c "+ category + " -n " + component,check_result=False)
   if "Error" not in result:
      return True
   else:
      return False

def stop_activity(package_name):
   cmd("am force-stop "+package_name,check_result=False)
   
def broadcast_intent(intent_name):
   result=cmd("am broadcast -a "+intent_name,check_result=False)
   if "Broadcast completed: result=0" in result:
      return True
   else:
      return False
         
def tcp_forward(local_port, remote_port):
   result=adb(['forward','tcp:'+str(local_port),'tcp:'+str(remote_port)])
   if "error" not in result:
      return True
   else: 
      return False

# check if the Package Manager is available i.e. can we install an APK. wait-for-device quits before PM is available.
def wait_for_package_manager():
   logI("waiting for Package Manager...")
   start_time=time.time()

   while int(time.time()-start_time)<20:
      if "package:/system/" in cmd("pm path android",check_result=False):
         return True
      time.sleep(1)
      
   return False   
   
def setdate(new_time):
   global busyboxPath
   
   result = cmd(busyboxPath + ' date -u "'+time.strftime('%Y-%m-%d %H:%M:%S',new_time)+'"', check_result=False)
   
   try:
      adb_time=time.strptime(result.split('\n')[0].strip(), "%a %b %d %H:%M:%S %Z %Y")
   except:
      logE("Try to set the platform date to '"+time.strftime('%Y-%m-%d %H:%M:%S',new_time)+"' but get in result:")
      logE(result)
      return False
   
   #if not (result.startswith(time.strftime('%a %b %d %H:%M:%S',new_time)) and result.endswith(time.strftime('%Y',new_time))):
   if adb_time!=new_time:
      logE("Try to set the platform date to '"+time.strftime('%Y-%m-%d %H:%M:%S',new_time)+"' but get in result:")
      logE(result)
      return False
   return True

def is_adbd_root():
   res = cmd("id", check_result=False)

   if "uid=0(root) gid=0(root)" in res:
      return True
   else:
      return False


def switch_to_user():
   res = cmd("id", check_result=False)

   if "uid=2000(shell) gid=2000(shell)" in res:
      return True # already in user mode
   else:
       cmd("su shell", check_result=False)
       res = cmd("id", check_result=False)
       if "uid=2000(shell) gid=2000(shell)" in res:
          return True
       else:
          return False
     
def is_interface_up(if_name):

   res = cmd("netcfg", check_result=False)
   for line in res.split("\n"):
      if (if_name in line) and ("UP" in line) and not ("0.0.0.0" in line):
         return True

   return False

def enableWifi():

   if is_interface_up("wlan0"):
      return True

   cmd("svc wifi enable",check_result=False)
   cmd("svc wifi prefer",check_result=False)

   start_time=time.time()

   while int(time.time()-start_time)<60:
      if is_interface_up("wlan0"):
         return True
      time.sleep(1)

   return False

def setMacAdd(if_name, mac_add):
   cmd("netcfg "+if_name+" hwaddr "+mac_add, check_result=False) #this command always return an error even in case of success
        
   isOk=False
   res = cmd("netcfg", check_result=False)
   for line in res.split("\n"):
      if (if_name in line) and (mac_add.lower() in line):
         isOk=True
         
   return isOk
 
def getDHCPIp(if_name):
   cmd("netcfg "+if_name+" dhcp", timeout=10)
         
   return is_interface_up(if_name)
            
# USER INTERFACE UTILITY FUNCTIONS...

# here, device string is hardware specific. use input_tap for a device-agnostic usage
# send_touch_event gives us the possiblitiy of a long press however
def send_touch_event(coord_X, coord_Y, device='/dev/input/event0',hold=False):
   # 3 53 = X coord
   # 3 54 = Y coord
   # 3 48 = width
   # 3 58 = pressure
   # 0 2 = end touch data
   # 0 0 = end report
   # 0 2 = release report
   # 0 0 = end
   cmd("sendevent " + device + " 3 53 " + str(coord_X))
   cmd("sendevent " + device + " 3 54 " + str(coord_Y))
   cmd("sendevent " + device + " 3 48 5")
   cmd("sendevent " + device + " 3 58 50")
   cmd("sendevent " + device + " 0 2 0")
   cmd("sendevent " + device + " 0 0 0")
   if hold:
      time.sleep(1)
   cmd("sendevent " + device + " 0 2 0")
   cmd("sendevent " + device + " 0 0 0")
   return True

# simulate a touchscreen swipe
def send_swipe_event(coord1_X, coord1_Y, coord2_X, coord2_Y):
   cmd('input touchscreen swipe '+ str(coord1_X) + ' ' + str(coord1_Y) + ' ' + str(coord2_X) + ' ' + str(coord2_Y))

# simulate a touchscreen tap
def input_tap(coord_X, coord_Y):
   cmd('input tap ' + str(coord_X) + ' ' + str(coord_Y))

# input text on the select text box
def input_text(text=""):
   if text=="":
      return False   
  
   words = text.split(" ")

   for word in words:   
      cmd('input text ' + word)
      cmd('input keyevent 62')

   return True

# simulate "Enter" softkey press
def input_enter_softkey():
   cmd('input keyevent 66')

# simulate "Power" HW button press
def input_power_button():
   cmd('input keyevent 26')

# simulate "Menu" HW button press
def input_menu_button():
   cmd('input keyevent 82')

# simulate "Back" HW button press
def input_back_button():
   cmd('input keyevent 4')

# simulate "DPAD_CENTER" HW button press
def input_dpad_center_button():
   cmd('input keyevent 23')

# simulate "DPAD_UP" HW button press
def input_dpad_up_button():
   cmd('input keyevent 19')

# simulate "DPAD_DOWN" HW button press
def input_dpad_down_button():
   cmd('input keyevent 20')

# simulate "DPAD_LEFT" HW button press
def input_dpad_left_button():
   cmd('input keyevent 21')

# simulate "DPAD_RIGHT" HW button press
def input_dpad_right_button():
   cmd('input keyevent 22')
   
def unlockScreen():
   res = cmd('dumpsys input_method', check_result=False)
   if "mScreenOn=false" in res:
      input_power_button()
      time.sleep(0.1)
   input_menu_button()
         
