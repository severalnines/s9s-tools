###############################################################################################
##                                                                                           ##
##  Copyright (C) 2011-present severalnines.com                                              ##
##                                                                                           ##
###############################################################################################
# !/bin/python
""" class to assist parsing and setting configuration """
# ----------------------------------------------------------------------------------------------
#
# file: configurer.py
#
# purpose: assist parsing and setting configuration
#
# ----------------------------------------------------------------------------------------------
import os
import sys
import logging
from pys9s.common.sys_comm import SysComm
from logging.handlers import RotatingFileHandler

log_file_env_var = "LOGFILE"
default_logfile  = "/tmp/pys9s.log"
log_level = 10  # DEBUG = 10, INFO = 20, ERROR = 40

# logger singleton
global logger_instance
logger_instance = None

###############################################################################
# Name: get_logger
# Description: returns an initialized logger
# Arguments: log_filename - filename to write on project's log/ folder
#  
# Returns: logger object
###############################################################################
def get_logger(logfile_name=None):
    global logger_instance
    if logger_instance is not None:
        return logger_instance
    # create logger instance
    if logfile_name is None:
        logfile_name = SysComm.get_env_var(log_file_env_var, default_logfile)
    log_file = logfile_name.replace('.py', '.log')
    print("log file is: " + log_file)
    log_formatter = logging.Formatter('%(asctime)s %(levelname)s %(funcName)s(%(lineno)d) %(message)s')
    user = os.environ.get("USER")
    logger = logging.getLogger(user)
    file_handler = RotatingFileHandler(filename=log_file,
                                       mode='a',
                                       maxBytes=1 * 1024 * 1024,
                                       backupCount=2,
                                       encoding=None,
                                       delay=False)
    file_handler.setFormatter(log_formatter)
    logger.addHandler(file_handler)
    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_handler.setFormatter(log_formatter)
    stdout_handler.setLevel(log_level)
    logger.addHandler(stdout_handler)
    logger.setLevel(log_level)
    logger.propagate = False
    logger_instance = logger
    return logger_instance
