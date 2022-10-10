###############################################################################################
##                                                                                           ##
##  Copyright (C) 2011-present severalnines.com                                              ##
##                                                                                           ##
###############################################################################################
# !/bin/python
""" s9s test to verify single node mssql cluster """
# ----------------------------------------------------------------------------------------------
#
# file: ft_mssql.py
#
# purpose: check single node mssql cluster behaviour
#
# ----------------------------------------------------------------------------------------------
import tracemalloc
import unittest
import time
import os

if "USE_FT_FULL" in os.environ and os.environ["USE_FT_FULL"] is not None:
    import sys

    sys.path.insert(1, '/home/pipas/s9s-tools/tests')
from pys9s.common.s9s_cli import S9sCli
from pys9s.common.lxd_manager import LxdManager
from pys9s.common.sys_comm import SysComm
from pys9s.common.configurer import get_logger, get_lxd_client_cert, get_lxd_server_cert, get_lxd_server_conn

global checks_count
checks_count = 0

DEBUG = True
# DEBUG = False
job_finished = "FINISHED"
secs_between_steps = 5
num_attempts = 5


class FtMssqlSingle(unittest.TestCase):  # pylint: disable=too-few-public-methods
    """ implementation class for executing lxc commands and pylxd client to handle lxd containers"""

    ######################################
    # PUBLIC METHODS
    ######################################
    ###############################################################################
    # Description: FtMssqlSingle constructor
    ###############################################################################
    def __init__(self, *args, **kwargs):
        # call base class constructor
        super(FtMssqlSingle, self).__init__(*args, **kwargs)
        if DEBUG:
            tracemalloc.start()
        self.logger = get_logger()
        # init s9s client
        self.s9s = S9sCli()
        # init lxd client
        self.client = LxdManager(_endpoint=get_lxd_server_conn(),
                                 _cert=get_lxd_client_cert(),
                                 _verify=get_lxd_server_cert())
        self.test_counter = 0
        self.container_name = None
        self.cluster_name = None
        self.cluster_id = None
        self.backup_id = None
        self.backup_title = "mssql_single_backup"

    ##################################################################################
    # setUp
    # setup required nodes for tests (a counter distinguish the base container to use)
    # \return void
    ##################################################################################
    def setUp(self):
        if DEBUG:
            tracemalloc.start()

        if self.test_counter == 0:
            base_container_name = 'ubuntu-focal-cloud'
            base_container = self.client.lxd_client.instances.get(base_container_name)
            self.container_name = 'ft-mssql-py-ubuntu'
            self.cluster_name = 'cluster-mssql-py-ubuntu'
        else:
            base_container_name = 'centos-9-cloud'
            base_container = self.client.lxd_client.instances.get(base_container_name)
            self.container_name = 'ft-mssql-py-centos'
            self.cluster_name = 'cluster-mssql-py-centos'

        self.mssql_version = "2019"
        self.container_ip = None

        # clean previous container
        if base_container is None:
            self.logger.error("Base container {} does not exist".format(base_container_name))
        if self.client.container_exists(self.container_name):
            self.client.delete_container(self.container_name)
        self.logger.info("Creating container: {}".format(self.container_name))
        if not self.client.copy_container(base_container_name, self.container_name):
            self.logger.error("Could not create node {}".format(self.container_name))
            return
        if not self.client.start_container(self.container_name):
            self.logger.error("Could not start node {}".format(self.container_name))
            return

        self.container_ip = self.client.get_container_ip(self.container_name)
        # clean previous cluster
        self.cluster_id, status = self.s9s.get_cluster_id(self.cluster_name)
        if self.cluster_id is not None:
            self.s9s.drop_cluster(self.cluster_id)

        self.test_counter = self.test_counter + 1

    # tear down nodes and generate report data
    ##################################################################################
    # setUp
    # setup required nodes for tests (a counter distinguish the base container to use)
    # \return void
    ##################################################################################
    def tearDown(self):
        # save cluster id to be deleted
        if self.cluster_id is None:
            self.cluster_id, status = self.s9s.get_cluster_id(self.cluster_name)
        # delete node
        self.client.stop_container(self.container_name)
        self.client.delete_container(self.container_name)
        # delete cluster
        if self.s9s.drop_cluster(self.cluster_id) is not True:
            self.logger.error("Could not delete cluster: {}".format(self.cluster_id))
            return
        if DEBUG:
            tracemalloc.stop()
        return
        # fillReportVariables()

    #####################################################################
    # NAME: deployment_check
    #####################################################################
    def deployment_check(self):
        global checks_count
        checks_count = checks_count + 1
        # deploy single node elasticsearch cluster
        admin = "SQLServerAdmin"
        admin_passwd = "myPassword"
        cluster_type = "mssql_single "
        self.assertIsNotNone(self.container_ip)
        self.logger.info("Container IP is: {}".format(self.container_ip))
        node = "{}:1433;".format(self.container_ip)
        command = self.s9s.base_command(os_creds=True)
        command += ' cluster'
        command += ' --create'
        command += ' --job-tags=\"createCluster\"'
        command += ' --cluster-type={}'.format(cluster_type)
        command += ' --cluster-name=\"{}\"'.format(self.cluster_name)
        command += ' --nodes=\"{}\"'.format(node)
        command += ' --db-admin=\"{}\"'.format(admin)
        command += ' --db-admin-passwd=\"{}\"'.format(admin_passwd)
        command += ' --vendor=\"mssql\"'
        command += ' --print-request'
        command += ' --with-tags=\"myMssqlCluster\"'
        command += ' --print-request'
        command += ' --log'
        self.logger.info("Executing: {}".format(command))
        result = self.s9s.exec_s9s_command(command)
        if len(result["stderr"]) > 3:
            self.logger.error(result["stderr"])
        else:
            self.logger.debug("Output: {}".format(result["stdout"]))
        # let the cluster get operational status
        time.sleep(secs_between_steps)
        # get cluster id and status
        status = "FAILED"
        for i in range(1, num_attempts):
            self.cluster_id, status = self.s9s.get_cluster_id(self.cluster_name)
            if status == "STARTED":
                break
            time.sleep(secs_between_steps)
        # check cluster status
        self.assertEqual("STARTED", status)
        # TODO: add check_cluster (from include.sh) checks here

    #####################################################################
    # NAME: create_database_check
    #####################################################################
    def create_database_check(self):
        global checks_count
        time.sleep(secs_between_steps)
        checks_count = checks_count + 1
        db_name = "testCreateDatabase"  # "s9s_testing_db"
        os.environ.keys()
        db_user = os.environ["PROJECT_OWNER"] if "PROJECT_OWNER" in os.environ.keys() else self.s9s.cmon_user
        db_pass = "password"
        self.assertTrue(self.s9s.create_database(self.cluster_id, db_name))
        #self.assertTrue(self.s9s.create_account(self.cluster_id, db_name, db_user, db_pass))

    #####################################################################
    # NAME: create_backup_check
    #####################################################################
    def create_backup_check(self):
        global checks_count
        # let's leave some time for the default snapshot repository to be created
        backup_node = self.container_ip
        backup_method = "mssqlfull"
        time.sleep(secs_between_steps)
        checks_count = checks_count + 1
        command = self.s9s.base_command()
        command += " backup"
        command += " --create"
        command += " --cluster-id={}".format(self.cluster_id)
        command += " --nodes=\"{}\"".format(backup_node)
        command += " --backup-directory=/tmp"
        command += " --backup-method={}".format(backup_method)
        command += " --title={}".format(self.backup_title)
        command += " --wait"  # better wait to check correct status
        self.logger.info("Executing: {}".format(command))
        result = self.s9s.exec_s9s_command(command)
        if len(result["stderr"]) > 3:
            self.logger.error(result["stderr"])
        else:
            self.logger.debug("Output: {}".format(result["stdout"]))
        # check backup exists
        time.sleep(secs_between_steps)
        self.backup_id = self.s9s.get_backup_id(self.backup_title)
        self.assertIsNotNone(self.backup_id)

    #####################################################################
    # NAME: test1_ubuntu_cluster_all  (test_counter==0)
    # DESCRIPTION: deploy elasticsearch single node cluster on ubuntu
    # EXPECTED RESULT:
    #     - Deployment is fine and cluster status is operative
    #     - Create database
    #     - Create backup
    #####################################################################
    def test1_ubuntu_cluster_all(self):
        self.deployment_check()
        self.create_database_check()
        self.create_backup_check()

    #####################################################################
    # NAME: test2_centos_cluster_all  (test_counter==1)
    # DESCRIPTION: deploy elasticsearch single node cluster on centOS
    # EXPECTED RESULT:
    #     - Deployment is fine and cluster status is operative
    #     - Create database
    #     - Create backup
    #####################################################################
    def test2_centos_cluster_all(self):
        self.deployment_check()
        self.create_database_check()
        self.create_backup_check()


##################################################################################
# suite
# defines the suite of test to be executed and its order
# \return void
##################################################################################
def suite():
    tests_suite = unittest.TestSuite()
    tests_suite.addTest(FtMssqlSingle('test1_ubuntu_cluster_all'))
    tests_suite.addTest(FtMssqlSingle('test2_centos_cluster_all'))
    return tests_suite


if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())

    # add checks to global results
    results = unittest.TestResult()
    num_errors = len(results.errors)
    num_fails = len(results.failures)
    num_skipped = len(results.skipped)
    num_success = checks_count - (num_skipped + num_errors + num_fails)
    SysComm.incr_env_var("NUMBER_OF_PERFORMED_CHECKS", checks_count)
    SysComm.incr_env_var("NUMBER_OF_SUCCESS_CHECKS", num_success)
    SysComm.incr_env_var("NUMBER_OF_WARNING_CHECKS", num_skipped)
    SysComm.incr_env_var("NUMBER_OF_FAILED_CHECKS", num_fails + num_errors)
    if results.wasSuccessful():
        SysComm.set_env_var("FAILED", "no")
        exit(0)
    else:
        SysComm.set_env_var("FAILED", "yes")
        exit(1)
