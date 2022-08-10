import ConfigParser
from lib_logging import logD, logI, logW, logE, logC

config = ConfigParser.RawConfigParser()

# read the specified file in to the parser
def read_file(path_to_file):
   # reset the contents of the parser
   #logD("Resetting parser content...")
   sections = config.sections()

   for section in sections:
      config.remove_section(section)

   #logD("Attempting to parse configuration file: " + path_to_file)
   try:
      fp = open(path_to_file, 'r')
   except IOError:
      logE("Specifed file was not found.")
      return False

   try:      
     config.readfp(fp)
     fp.close()
     return config
   except ConfigParser.Error as e:
      logE("Specifed file is badly formatted. Parsing failed.")
      logE(e)
      return False
   
# get a list of all defined sections
def get_section_list():
   return config.sections()

# check if specified section is defined
def is_section_defined(section):
   return config.has_section(section)

# check if specified option exists in specified section
def is_option_defined(section, option):
   return config.has_option(section, option)

# get all options
def get_all_options(section):
   return config.items(section)

# read an option value
def get_value_as_string(section, option):
   value = ""
   try:
      value = config.get(section, option)
   except ConfigParser.NoOptionError:
      logE("Specified option [" + option + "] was not found.")

   return value

# read an option value as a boolean (e.g. "1", "True", "true" return True; "0", "False", "false" return False)
def get_value_as_boolean(section, option):
   try:
      value = config.getboolean(section, option)
      return value
   except ConfigParser.NoOptionError:
      logE("Specified option [" + option + "] was not found.")
   except ConfigParser.ValueError:
      value = config.get(section, option)
      logE("Specified option [" + option + "] could not be interpreted as an boolean. Value: " + value)

# read an option value as an integer
def get_value_as_integer(section, option):
   value = None
   try:
      value = config.getint(section, option)
      return value
   except ConfigParser.NoOptionError:
      logE("Specified option [" + option + "] was not found.")
   except ConfigParser.ValueError:
      value = config.get(section, option)
      logE("Specified option [" + option + "] could not be interpreted as an integrer. Value: " + value)

# set the value of an option, creating the option if it does not exist. NOTE: not written to file.
def set_value(section, option, value):
   try:
      config.set(section, option, value)
      return True
   except ConfigParser.NoSectionError:
      logE("Specified section [" + section + "] does not exist.")
   except ConfigParser.TypeError:
      logE("value type is not valid.  Value must be of type: str")
 
   return False

#write the current configuration data to the file specified (over-writes if the file already exists)
def write_to_file(path_to_file):
   logI("Writing configuration to file...")
   try:
      fp = open(path_to_file, 'w')
   except IOError:
      logE("Specifed file was not found: " + path_to_file)
      return False

   config.write(fp)
   fp.close()
   return True

