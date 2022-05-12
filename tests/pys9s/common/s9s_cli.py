###############################################################################################
##                                                                                           ##
##  Copyright (C) 2011-present severalnines.com                                              ##
##                                                                                           ##
###############################################################################################
# !/bin/python
""" class to assist s9s cli executions """
# ----------------------------------------------------------------------------------------------
#
# file: s9s_cli.py
#
# purpose: assists to generate s9s commands by reading configuration (user) and provide formatted
#          commands
#
# ----------------------------------------------------------------------------------------------
import os
from pys9s.common.sys_comm import SysComm
from pys9s.common.configurer import get_logger

######################################
# CONSTANTS
######################################
controller_port_ft_full = "9556"  # pointing to ft_full (9556)
controller_port_cmon    = "9501"  # against real cmon   (9501)

class S9sCli:  # pylint: disable=too-few-public-methods
    """ implementation class for assisting on s9s cli commands generation """

    ######################################
    # PUBLIC METHODS
    ######################################

    ###############################################################################
    # Description: S9sCli constructor
    ###############################################################################
    def __init__(self):
        self.logger = get_logger()
        self.controller      = None
        self.controller_port = None
        self.cmon_user       = None
        self.cmon_pass       = None
        self.ssh_user        = None
        self.ssh_user_cert   = None
        s9s_tools_dir = os.getcwd().split('s9s-tools')[0]
        self.s9s_bin = "{}s9s-tools/s9s/s9s".format(s9s_tools_dir)  # using repository binary
        self.load_configuration()

    ###############################################################################
    # load_configuration
    # Initialize configuration variables required for cli connection
    #   - cmon user credentials
    #   - ssh credentials for nodes
    # \return true if operation went well
    ###############################################################################
    def load_configuration(self):
        use_ft_full = False
        self.ssh_user = "root"
        self.ssh_user_cert = "/home/alvaro/.ssh/id_rsa"  # ssh private key
        self.cmon_user = "pipas" if use_ft_full else "ccalvaro"
        self.cmon_pass = "secret" if use_ft_full else "vinu"
        self.controller = "localhost"
        self.controller_port = controller_port_ft_full if use_ft_full else controller_port_cmon

    ###############################################################################
    # base_command
    # \return s9s cli command with some connections parameters
    ###############################################################################
    def base_command(self, os_creds=False, color=True):
        command = '{}'.format(self.s9s_bin)
        if color:
            command += ' --color=always'
        command += ' --controller=https://{}:{}'.format(self.controller, self.controller_port)
        command += ' --cmon-user={}'.format(self.cmon_user)
        command += ' --password={}'.format(self.cmon_pass)
        if os_creds:
            command += ' --os-user={}'.format(self.ssh_user)
            command += ' --os-key-file={}'.format(self.ssh_user_cert)
        return command

    ###############################################################################
    # get_cluster_id
    # \param cluster_name
    # \return the cluster id for a given cluster_name
    ###############################################################################
    def get_cluster_id(self, cluster_name):
        command = self.base_command()
        command += ' cluster'
        command += ' --list'
        command += ' --long'
        command += ' --batch'
        command += ' --cluster-name={}'.format(cluster_name)
        self.logger.info("Executing: {}".format(command))
        result = SysComm.exec_command(command)
        if len(result["stderr"]) > 3:
            self.logger.error(result["stderr"])
            return None, None
        else:
            self.logger.info("Output: {}".format(result["stdout"]))
            cid = result["stdout"].split(' ')[0]
            status = result["stdout"].split(' ')[1]
            self.logger.debug("cluster id found: {}".format(cid))
            if not cid.isnumeric():
                return None, None
            return cid, status

    ###############################################################################
    # get_backup_id
    # \param backup_title
    # \param cluster_id
    # \return the corresponding backup id if any
    ###############################################################################
    def get_backup_id(self, backup_title, cluster_id=None):
        command = self.base_command()
        command += ' backup'
        command += ' --list'
        command += ' --long'
        if cluster_id is not None:
            command += ' --cluster-id={}'.format(cluster_id)
        command += ' --batch'
        self.logger.info("Executing: {}".format(command))
        result = SysComm.exec_command(command)
        if len(result["stderr"]) > 3:
            self.logger.error(result["stderr"])
            return None
        else:
            bid = "wrong_id"
            self.logger.debug("Output: {}".format(result["stdout"]))
            for line in result["stdout"].split('\n')[:-1]:
                line_values = line.split(' ')
                # on current day (10:38:25)
                title1 = line_values[16] if len(line_values) >= 17 else None
                # on other day (2022-05-04 10:38:25)
                title2 = line_values[17] if len(line_values) >= 18 else None
                if title1 == backup_title or title2 == backup_title:
                    bid = line_values[0]
                    break
            self.logger.info("Backup id found: {}".format(bid))
            if not bid.isnumeric():
                return None
            return bid

    ###############################################################################
    # drop_cluster
    # \param cluster_id
    # \return the cluster id for a given cluster_name
    ###############################################################################
    def drop_cluster(self, cluster_id):
        command = self.base_command(color=False)
        command += ' cluster'
        command += ' --drop'
        command += ' --cluster-id={}'.format(cluster_id)
        command += ' --wait'
        command += ' --print-request'
        command += ' --print-json'
        command += ' --log'
        self.logger.debug("Executing: {}".format(command))
        result = SysComm.exec_command(command)
        if len(result["stderr"]) > 3:
            self.logger.error(result["stderr"])
            return False
        else:
            self.logger.debug("Output: {}".format(result["stdout"]))
            return True

    ###############################################################################
    # exec_s9s_command
    # executes a s9s job command and logs it
    # \param command: command line to execute as in shell
    # \param log_job: flag to indicate if job log must be logged on test execution
    # \return dict with "stdout"  and "stderr"
    ###############################################################################
    def exec_s9s_command(self, command, log_job=True):
        ret = SysComm.exec_command(command)
        if log_job:
            job_id, jstatus = self.get_last_job_id()
            if job_id is None:
                self.logger.error("Could not obtain last job ID")
                return ret
            job_command = self.base_command()
            job_command += " job"
            job_command += " --job-id={}".format(job_id)
            job_command += " --log"
            job_ret = SysComm.exec_command(job_command)
            self.logger.info("job {} output:\n{}".format(job_id,
                                                         job_ret["stdout"]))
        return ret

    ###############################################################################
    # get_last_job_id
    # returns the job id of the last executed job
    # \return last job ID
    ###############################################################################
    def get_last_job_id(self):
        command = self.base_command()
        command += " job"
        command += " --list"
        command += " --batch"
        command += ""
        result = SysComm.exec_command(command)
        if len(result["stderr"]) > 3:
            self.logger.error(result["stderr"])
            return None, None
        else:
            lines = result["stdout"].split('\n')
            last_line = lines[len(lines)-2]  # last line is empty
            self.logger.debug("Output last line: {}".format(last_line))
            jid = last_line.split(' ')[0]
            self.logger.debug("job id found: {}".format(jid))
            jstatus = last_line.split(' ')[2]
            self.logger.debug("job status: {}".format(jstatus))
            if not jid.isnumeric():
                return None, None
            return jid, jstatus
