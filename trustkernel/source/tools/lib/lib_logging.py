import logging
import inspect

Test_Stack=[]

def init_log(path_to_log):
   logging.basicConfig(level=logging.DEBUG,
                    format='%(asctime)s %(level_short_name)-3s %(caller_name)-20s   %(message)s',
                    datefmt='%H:%M:%S',
                    filename=path_to_log,
                    filemode='w')
   # define a Handler which writes INFO messages or higher to the sys.stderr
   console = logging.StreamHandler()
   console.setLevel(logging.INFO)
   # set a format which is simpler for console use
   formatter = logging.Formatter('%(asctime)s %(level_long_name)s %(message)s', datefmt='%H:%M:%S')
   # tell the handler to use this format
   console.setFormatter(formatter)

   # add the handler to the root logger
   logging.getLogger('').addHandler(console)

def log(lvl, msg):
   short_log_name = {logging.DEBUG:"DEB", logging.INFO:"INF", logging.WARNING:"WAR", logging.ERROR:"ERR", logging.CRITICAL:"CRI"}
   long_log_name = {logging.DEBUG:"", logging.INFO:"", logging.WARNING:" WARNING:", logging.ERROR:" ERROR:", logging.CRITICAL:" CRITICAL:"}
    
   #logger = logging.getLogger('')
   caller=inspect.stack()[2][3]
   caller = caller[:19] + (caller[19:] and '..')
   
   logging.log(lvl, msg, extra={'caller_name':caller, 'level_short_name':short_log_name[lvl], 'level_long_name':long_log_name[lvl]})

def logD(msg):
   log(logging.DEBUG, msg)

def logI(msg):
   log(logging.INFO, msg)

def logW(msg):
   log(logging.WARNING, msg)

def logE(msg):
   log(logging.ERROR, msg)

def logC(msg):
   log(logging.CRITICAL, msg)
   
#def logTest(msg):
#   Test_Stack.append(msg)
#   log(logging.INFO, msg)
#
#def logResult(res):
#   res_txt={True:'SUCCESS', False:'FAILED'}
#   test=Test_Stack.pop()
#   
#   log(logging.INFO, res_txt[res])

