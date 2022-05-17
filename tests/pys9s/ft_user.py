###############################################################################################
##                                                                                           ##
##  Copyright (C) 2011-present severalnines.com                                              ##
##                                                                                           ##
###############################################################################################
# !/bin/python
""" s9s test to verify single node elasticsearch cluster """
# ----------------------------------------------------------------------------------------------
#
# file: ft_elasticsearch.py
#
# purpose: check single node elasticsearch cluster behaviour
#
# ----------------------------------------------------------------------------------------------
import unittest
import os
if os.environ["USE_FT_FULL"] is not None:
   import sys
   sys.path.insert(1, '/home/pipas/s9s-tools/tests')
from pys9s.common.s9s_cli  import S9sCli
from pys9s.common.sys_comm import SysComm

global checks_count
checks_count = 0

class FtUser(unittest.TestCase):  # pylint: disable=too-few-public-methods
    """ implementation class for executing lxc commands and pylxd client to handle lxd containers"""

    ######################################
    # PUBLIC METHODS
    ######################################
    ###############################################################################
    # Description: FtUser constructor
    ###############################################################################
    def __init__(self, *args, **kwargs):
        # call base class constructor
        super(FtUser, self).__init__(*args, **kwargs)
        self.s9s = S9sCli()
        self.checks = 0

    #####################################################################
    # NAME: deployment_check
    #####################################################################
    def check_pipas_defined(self):
        global checks_count
        checks_count = checks_count + 1
        # deploy single node elasticsearch cluster
        user_id = self.s9s.get_user("pipas")
        self.assertIsNotNone(user_id)


##################################################################################
# suite
# defines the suite of test to be executed and its order
# @returns void
##################################################################################
def suite():
    tests_suite = unittest.TestSuite()
    tests_suite.addTest(FtUser('check_pipas_defined'))
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
    ok = results.wasSuccessful()
    SysComm.set_env_var("FAILED", "no")
    exit(ok)
