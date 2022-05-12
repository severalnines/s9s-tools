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

class FakeUt(unittest.TestCase):  # pylint: disable=too-few-public-methods
    """ implementation class for executing lxc commands and pylxd client to handle lxd containers"""

    ######################################
    # PUBLIC METHODS
    ######################################
    ###############################################################################
    # Description: FakeUt constructor
    ###############################################################################
    def __init__(self, *args, **kwargs):
        # call base class constructor
        super(FakeUt, self).__init__(*args, **kwargs)


    #####################################################################
    # NAME: deployment_check
    #####################################################################
    def deployment_check(self):
        # deploy single node elasticsearch cluster
        admin = "admin"
        admin_passwd = "myPassword"
        self.assertNotEqual(admin, admin_passwd)


##################################################################################
# suite
# defines the suite of test to be executed and its order
# @returns void
##################################################################################
def suite():
    tests_suite = unittest.TestSuite()
    tests_suite.addTest(FakeUt('deployment_check'))
    return tests_suite


if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())
