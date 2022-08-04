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
default_logfile = "/tmp/pys9s.log"
log_level = 10  # DEBUG = 10, INFO = 20, ERROR = 40

# Singletons
global logger_instance
logger_instance = None
global pylxd_client_instance
pylxd_client_instance = None


###############################################################################
# Name: get_logger
# Description: returns an initialized logger
# \param log_filename: filename to write on project's log/ folder
#  
# \return logger object
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
    log_formatter = logging.Formatter('%(asctime)s : %(levelname)s : %(filename)s:%(lineno)d : %(message)s')
    user = os.environ.get("USER", "pipas")
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


###############################################################################
# Name: get_lxd_client_cert
# Description: returns the location of the .crt and .key files for the user
#
# \return client lxd certs tuple (client.crt, client.key)
###############################################################################
def get_lxd_client_cert():
    logger = get_logger()
    user = SysComm.get_env_var("USER", "pipas")
    crt = '/home/{}/snap/lxd/common/config/client.crt'.format(user)
    key = '/home/{}/snap/lxd/common/config/client.key'.format(user)
    if not os.path.exists(crt):
        logger.error("User {} has not lxd certificate configured on:\n{}".
                     format(user, crt))
        return None, None
    if not os.path.exists(key):
        logger.error("User {} has not lxd certificate configured on:\n{}".
                     format(user, key))
        return None, None
    return crt, key


###############################################################################
# Name: get_lxd_server_cert
# Description: returns the location of the .crt file for  the lxd server
#
# \return server lxd cert (server.crt)
###############################################################################
def get_lxd_server_cert():
    logger = get_logger()
    server_crt = '/var/snap/lxd/common/lxd/server.crt'
    if not os.path.exists(server_crt):
        logger.error("lxd server certificate not found on:\n{}".format(server_crt))
        return None
    return server_crt


###############################################################################
# Name: get_lxd_server_conn
# Description: returns the pylxd client https connection string
#
# \return https URI
###############################################################################
def get_lxd_server_conn():
    logger = get_logger()
    port = '8443'
    server = SysComm.get_env_var("SERVER", "127.0.0.1")
    conn = 'https://{}:{}'.format(server, port)
    logger.info("using lxd server: {}".format(conn))
    return conn
