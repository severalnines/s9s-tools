###############################################################################################
##                                                                                           ##
##  Copyright (C) 2011-present severalnines.com                                              ##
##                                                                                           ##
###############################################################################################
# !/bin/python
""" s9s test to verify high availability elasticsearch cluster """
# ----------------------------------------------------------------------------------------------
#
# file: ft_elasticsearch_ha.py
#
# purpose: check HA elasticsearch cluster behaviour
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


class FtElasticsearchHA(unittest.TestCase):  # pylint: disable=too-few-public-methods
    """ implementation class for executing lxc commands and pylxd client to handle lxd containers"""

    ######################################
    # PUBLIC METHODS
    ######################################
    ###############################################################################
    # Description: FtElasticsearchSingle constructor
    ###############################################################################
    def __init__(self, *args, **kwargs):
        # call base class constructor
        super(FtElasticsearchHA, self).__init__(*args, **kwargs)
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
        self.container_list = None
        self.cluster_name = None
        self.cluster_id = None
        self.backup_id = None
        self.backup_title = "elastic_single_backup"

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
            self.container_list = ['ft-elasticsearch-ha-ubuntu-1',
                                   'ft-elasticsearch-ha-ubuntu-2',
                                   'ft-elasticsearch-ha-ubuntu-3']
            self.cluster_name = 'elasticsearch-py-ubuntu-ha'
            self.elastic_version = "8.3.1"  # latest
        else:
            base_container_name = 'centos-9-cloud'
            base_container = self.client.lxd_client.instances.get(base_container_name)
            self.container_list = ['ft-elasticsearch-ha-centos-1',
                                   'ft-elasticsearch-ha-centos-2',
                                   'ft-elasticsearch-ha-centos-3']
            self.cluster_name = 'elasticsearch-py-ha-centos'
            self.elastic_version = "8.1.7"

        self.container_ip = list()

        # clean previous container
        if base_container is None:
            self.logger.error("Base container {} does not exist".format(base_container_name))
        for container in self.container_list:
            if self.client.container_exists(container):
                self.client.delete_container(container)
            self.logger.info("Creating container: {}".format(container))
            if not self.client.copy_container(base_container_name, container):
                self.logger.error("Could not create node {}".format(container))
                return
            if not self.client.start_container(container):
                self.logger.error("Could not start node {}".format(container))
                return
            self.container_ip.append(self.client.get_container_ip(container))

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
        # delete nodes
        for container in self.container_list:
            self.client.stop_container(container)
            if self.client.delete_container(container) is not True:
                self.logger.error("Could not delete node {}".format(container))

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
        admin = "admin"
        admin_passwd = "myPassword"
        self.assertIsNotNone(self.container_ip)
        self.logger.info("Containers IP are: {}".format(self.container_ip))
        node1 = 'elastic://{}?roles=master-data'.format(self.container_ip[0])
        node2 = 'elastic://{}?roles=master-data'.format(self.container_ip[1])
        node3 = 'elastic://{}?roles=master-data'.format(self.container_ip[2])
        command = self.s9s.base_command(os_creds=True)
        command += ' cluster'
        command += ' --create'
        command += ' --cluster-name={}'.format(self.cluster_name)
        command += ' --cluster-type=elastic'
        command += ' --nodes=\"{};{};{}\"'.format(node1, node2, node3)
        # storage_host can not be defined, there are still problems to use nfs on containers
        command += ' --db-admin=\"{}\"'.format(admin)
        command += ' --db-admin-passwd=\"{}\"'.format(admin_passwd)
        command += ' --vendor=\"elasticsearch\"'
        command += ' --provider-version=\"{}\"'.format(self.elastic_version)
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

    #####################################################################
    # NAME: plugins_check
    #####################################################################
    def plugins_check(self):
        global checks_count
        checks_count = checks_count + 1
        if self.elastic_version != "7.x":  # since version 8.x plugins comes on pre-installed on packages
            return
        # check that repository-hdfs was installed
        expected_plugin = "repository-s3"
        c_name = self.container_list
        command = ['/usr/share/elasticsearch/bin/elasticsearch-plugin', 'list']
        exit_code, c_stdout, c_std_err = self.client.execute_on_container(c_name,
                                                                          command)
        plugins = str.strip(c_stdout)
        self.assertEqual(0, exit_code)
        self.assertEqual(expected_plugin, plugins)

    #####################################################################
    # NAME: freeze_node1_check
    #####################################################################
    def freeze_node1_check(self):
        global checks_count
        checks_count = checks_count + 1
        # freeze node, wait and check FAILURE status
        status = "STARTED"
        self.client.stop_container(self.container_list[0])
        for i in range(1, num_attempts):
            time.sleep(secs_between_steps)
            self.cluster_id, status = self.s9s.get_cluster_id(self.cluster_name)
            if status == 'DEGRADED':
                break
        self.assertEqual('DEGRADED', status)

    #####################################################################
    # NAME: freeze_node2_check
    #####################################################################
    def freeze_node23_check(self):
        global checks_count
        checks_count = checks_count + 1
        # freeze node, wait and check FAILURE status
        status = "STARTED"
        self.client.stop_container(self.container_list[1])
        self.client.stop_container(self.container_list[2])
        for i in range(1, num_attempts):
            time.sleep(secs_between_steps)
            self.cluster_id, status = self.s9s.get_cluster_id(self.cluster_name)
            if status == 'FAILURE':
                break
        self.assertEqual('FAILURE', status)

    #####################################################################
    # NAME: recover_nodes_check
    #####################################################################
    def recover_nodes_check(self):
        global checks_count
        checks_count = checks_count + 1
        # recover node, wait and check STARTED states
        status = "FAILURE"
        self.client.start_container(self.container_list[0])
        self.client.start_container(self.container_list[1])
        self.client.start_container(self.container_list[2])
        for i in range(1, 5 * num_attempts):
            time.sleep(secs_between_steps)
            self.cluster_id, status = self.s9s.get_cluster_id(self.cluster_name)
            if status == 'STARTED':
                break
        self.assertEqual('STARTED', status)

    #####################################################################
    # NAME: test1_ubuntu_cluster_all  (test_counter==0)
    # DESCRIPTION: deploy elasticsearch single node cluster on ubuntu
    # EXPECTED RESULT:
    #     - Deployment is fine and cluster status is operative
    #     - Create backup
    #     - Restore backup
    #     - Delete backup
    #     - Stop node, wait and check FAILURE status
    #     - Recover node, wait and check STARTED states
    #####################################################################
    def test1_ubuntu_cluster_all(self):
        self.deployment_check()
        # self.plugins_check()
        self.freeze_node1_check()
        self.freeze_node2_check()
        self.recover_nodes_check()

    #####################################################################
    # NAME: test2_centos_cluster_all  (test_counter==1)
    # DESCRIPTION: deploy elasticsearch ha cluster on centOS
    # EXPECTED RESULT:
    #     - Deployment is fine and cluster status is operative
    #     - Stop node, wait and check DEGRADED cluster status
    #     - Stop one more node and wait and check FAILED cluster status
    #     - Recover nodes, wait and check STARTED states
    #####################################################################
    def test2_centos_cluster_all(self):
        self.deployment_check()
        self.plugins_check()
        self.freeze_node1_check()
        self.freeze_node2_check()
        self.recover_nodes_check()


##################################################################################
# suite
# defines the suite of test to be executed and its order
# \return void
##################################################################################
def suite():
    tests_suite = unittest.TestSuite()
    tests_suite.addTest(FtElasticsearchHA('test1_ubuntu_cluster_all'))
    tests_suite.addTest(FtElasticsearchHA('test2_centos_cluster_all'))
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
