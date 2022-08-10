import subprocess
import time
import os
import time
import threading
import signal
from lib_logging import logD, logI, logW, logE, logC

def start(cmd,out=None,timeout_=0):
   handle = None
   
   timeout_=0
   
   if not out:
      out=subprocess.PIPE
   
   try:
      handle = subprocess.Popen(cmd, universal_newlines=True,stdin=out,stdout=out,stderr=subprocess.STDOUT)
      if timeout_>0:
         t = threading.Timer( timeout_, timeout, [handle] )
         t.start()
   except:
      pass

   return handle
   
def start_shell(cmd,timeout_=0):
   handle = None

   # WARNING the handle is the handle on the shell, not the child processes
   # to have the spawned process inherit from shell we need to do:
   # handle = subprocess.Popen(["exec" + cmd],shell=True,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)

   try:
      handle = subprocess.Popen([cmd],shell=True,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,preexec_fn=os.setsid)
      if timeout_>0:
         t = threading.Timer( timeout_, timeout, [handle] )
         t.start()
   except:
      pass

   return handle
   
def is_ended(handle):
   is_ended = (handle.poll()!=None)
   return is_ended

def timeout( p ):
   if p.poll() == None:
      try:
          p.kill()
          logE('process taking too much time to perform --terminating!')
      except:
          pass
def get_output(handle):
   return handle.communicate()[0]

def get_returncode(handle):
   return handle.returncode

def blocking_cmd(cmd):
   hndl = start(cmd,timeout_=300)
   return get_output(hndl)

def kill(handle):
   if is_ended(handle):
      return True
   
   #logI("terminate")
   handle.terminate()   
   time.sleep(0.1)
   
   if is_ended(handle):
      return True
   
   #logI("kill")
   handle.kill()   
   time.sleep(0.1)
   handle.wait()
   
   if is_ended(handle):
      return True
      
   return False

   
