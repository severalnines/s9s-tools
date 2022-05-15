###############################################################################################
##                                                                                           ##
##  Copyright (C) 2011-present severalnines.com                                              ##
##                                                                                           ##
###############################################################################################
# !/bin/python
""" class to assist with command executions """
# ----------------------------------------------------------------------------------------------
#
# file: sys_comm.py
#
# purpose: assists to generate system commands
#
# ----------------------------------------------------------------------------------------------
import os
import subprocess
import shlex

######################################
# CONSTANTS
######################################
ARCH_AMD     = "amd64"
ARCH_INTEL   = "i386"
DEFAULT_ARCH = ARCH_AMD


class SysComm:  # pylint: disable=too-few-public-methods
    """ implementation class for assisting on system commands execution """

    ######################################
    # PUBLIC METHODS
    ######################################

    ###############################################################################
    # Description: Command constructor
    ###############################################################################
    def __init__(self):
        return

    ###############################################################################
    # exec_bash_function
    # \param function_name: function to be called
    # \param args: arguments of the function (as in command line)
    # \param bash_file: name of the file in which is located the function
    # \return true if operation went well
    ###############################################################################
    @staticmethod
    def exec_bash_function(function_name, args, bash_file):
        command = 'bash -c . {}; {} {}'.format(bash_file, function_name, args)
        return SysComm.exec_command(command)

    ###############################################################################
    # exec_command
    # executes a system command
    # \param command: command line to execute as in shell
    # \return dict with "stdout"  and "stderr"
    ###############################################################################
    @staticmethod
    def exec_command(command):
        cmd_split = shlex.split(command, comments=True)
        result = subprocess.Popen(cmd_split,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
        out, err = result.communicate()
        ret = dict()
        ret["stdout"] = out.decode('utf-8')
        ret["stderr"] = err.decode('utf-8')
        return ret

    ###############################################################################
    # get_env_var
    # returns the value of a variable (need to be exported first)
    # \param name: name of the parameter
    # \param default: value to be returned if variable is not defined
    # \return the value of the variable
    ###############################################################################
    @staticmethod
    def get_env_var(name, default):
        if name in os.environ:
            return os.environ[name]
        else:
            return default

    ###############################################################################
    # set_env_var
    # sets the value of an environment variable
    # \param name: name of the parameter
    # \param value: value to set
    # \return true if no error
    ###############################################################################
    @staticmethod
    def set_env_var(name, value):
        if name in os.environ:
            os.environ[name] = value
            os.system("echo export {}=${} >> .pys9s_results_env".format(name, name))
            return True
        return False

    ###############################################################################
    # incr_env_var
    # increments the value of an environment variable if exists and its numeric
    # \param name: name of the parameter
    # \param value: value to increment
    # \return true if no error
    ###############################################################################
    @staticmethod
    def incr_env_var(name, value):
        current = SysComm.get_env_var(name, "undef")
        if current == "undef" or not current.isnumeric():
            return False
        n_val  = int(value)
        n_curr = int(current)
        return SysComm.set_env_var(name, str(n_curr+n_val))

    ###############################################################################
    # local_arch
    # detect local architecture
    # returns the architecture as the result of the execution of command:
    #   "dpkg --print-architecture"
    # \param name: function to be called
    # \return the value of the variable
    ###############################################################################
    @staticmethod
    def local_arch():
        # get system architecture
        arch = DEFAULT_ARCH
        try:
            with open(os.path.devnull, "w") as devnull:
                dpkg = subprocess.Popen(['dpkg', '--print-architecture'],
                                        stderr=devnull, stdout=subprocess.PIPE,
                                        universal_newlines=True)

                if dpkg.wait() == 0:
                    arch = dpkg.stdout.read().strip()
        except subprocess.CalledProcessError:
            return DEFAULT_ARCH
        except subprocess.TimeoutExpired:
            return DEFAULT_ARCH
        return arch
