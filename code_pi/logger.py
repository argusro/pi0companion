#========================================================================================
#
#   Logger used by other scripts
#
#   This script update specific and a master log
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   version 3 as published by the Free Software Foundation.
#
#   Use:  import logger
#         log, master_log = logger.set_logs(os.path.basename(__file__))
#         logger.main(<level>,<message>,log, master_log)
#
#   Version: 1.0 - 09/2023
#   Author: ArgusR <argusr@me.com>
#
#========================================================================================
import sys
import json
import logging
import inspect
# import push_log
from logging.handlers import RotatingFileHandler

base_path="/home/pi/LOG/"
script_name=''
DB_MGMT="/home/pi/db/mgmt.json"

def setup_logger(name, log_file, script):
    MGMT = json.loads(open(DB_MGMT).read())
    log_level = MGMT["log_level"]
    if log_level == "DEBUG":
        level_=logging.DEBUG
    if log_level == "INFO":
        level_=logging.INFO

    console_format = "%(asctime)s - %(levelname)s - %(process)d - "+ script + "%(message)s"

    log_formatter = logging.Formatter(console_format)
    handler = RotatingFileHandler(log_file, mode='a', maxBytes=5*1024*1024,
                                backupCount=2, encoding=None, delay=0)
    handler.setFormatter(log_formatter)
    logger = logging.getLogger(name)
    logger.setLevel(level_)
    logger.addHandler(handler)
    return logger

# Initiate loggers
def set_logs(script):
    global script_name
    script_name = script
    master_logger = setup_logger('master', base_path +'master.log',script)
    script_logger = setup_logger(script, base_path+ script + ".log",script)
    return script_logger, master_logger

# Save log message
def main(level,message,script_logger,master_logger,just_script=False):
    global script_name
    func = inspect.currentframe().f_back
    line = str(func.f_lineno)
    # module = func.f_code.co_name
    # message = module + ':' + line + ' - ' +  message
    message = ':' + line + ' - ' +  message
    if level == "info":
        script_logger.info(message)
        if just_script is False: master_logger.info(message)
    elif level == "warning":
        script_logger.warning(message)
        if just_script is False: master_logger.warning(message)
    elif level == "error":
        script_logger.error(message)
        if just_script is False: master_logger.error(message)
    elif level == "critical":
        script_logger.critical(message)
        if just_script is False: master_logger.critical(message)
    elif level == "debug":
        script_logger.debug(message)
        if just_script is False: master_logger.debug(message)
    
    # # If log different from info send it to Thinksboard
    # if level != "info" and level != "debug":
    #     push_log.main(level,script_name+message)

if __name__ == "__main__":
    main(sys.argv[2], sys.argv[3])
