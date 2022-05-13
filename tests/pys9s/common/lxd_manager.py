###############################################################################################
##                                                                                           ##
##  Copyright (C) 2011-present severalnines.com                                              ##
##                                                                                           ##
###############################################################################################
# !/bin/python
""" lxd manager to handle containers life cycle """
# ----------------------------------------------------------------------------------------------
#
# file: lxd_manager.py
#
# purpose: handles lxd images and containers lifecycle
#
# ----------------------------------------------------------------------------------------------
import time
from pylxd import Client
from pys9s.common.sys_comm import SysComm
from pylxd.exceptions import NotFound, LXDAPIException
from pys9s.common.configurer import get_logger

secs_between_steps      = 5       # seconds to wait between tests steps
err_chars_threshold     = 3       # number of chars on stderr to detect command failure


class LxdManager:  # pylint: disable=too-few-public-methods
    """ implementation class for executing lxc commands and pylxd client to handle lxd containers"""

    ######################################
    # PUBLIC METHODS
    ######################################

    ###############################################################################
    # Description: LxdManager constructor
    # Arguments:
    #
    # Returns:
    ###############################################################################
    def __init__(self, _endpoint, _cert=None, _verify=None, _passwd=None):
        if _cert is not None:
            self.lxd_client = Client(endpoint=_endpoint,
                                     cert=_cert,
                                     verify=_verify)
        else:
            self.lxd_client = Client(endpoint=_endpoint)
            self.lxd_client.authenticate(password=_passwd)

        self.logger = get_logger()
        self.lxc_bin = "/usr/bin/lxc"  # LxdManager.exec_command('which lxc')['stdout']
        self.instances = dict()  # map with key (container_name) and value (container object)

    ###############################################################################
    # get_containers
    # \return the list of containers and their state
    ###############################################################################
    def get_containers(self):
        return self.lxd_client.containers.all()

    ###############################################################################
    # container_exists
    # @container_name: name of the container
    # \return true if a container with specified name exists
    ###############################################################################
    def container_exists(self, container_name):
        try:
            if self.lxd_client.containers.get(container_name) is not None:
                return True
        except NotFound as e:
            self.logger.debug("Container ot found: {}. Error {}".format(container_name, str(e)))
        return False

    ###############################################################################
    # delete_container
    # @container_name: name of the container
    # \return true if container was deleted
    ###############################################################################
    def delete_container(self, container_name):
        container = self.lxd_client.containers.get(container_name)
        if container is not None:
            container.stop()
            time.sleep(secs_between_steps)
            container.delete()
            if container_name in self.instances.keys():
                del self.instances[container_name]
            return True
        return False

    ###############################################################################
    # copy_container
    # @base_container: name of the container to copy
    # @new_container: name of the container to be created
    # \return true if container could be created
    ###############################################################################
    def copy_container(self, base_container, new_container):
        config = {'name': '{}'.format(new_container),
                  'source': {'type': 'copy', 'source': '{}'.format(base_container)}}
        try:
            ret = self.lxd_client.containers.create(config, wait=True)
            if ret is not None:
                self.instances[new_container] = ret
                return True
        except LXDAPIException as e:
            self.logger.error("Could not copy container: {}".format(str(e)))
            return False

    ###############################################################################
    # get_container_ip_cmd
    # \param container_name: name of the container
    # \return the ip of the container given its name
    ###############################################################################
    def get_container_ip_cmd(self, container_name):
        command = '{} list | grep {} | cut -d \"|\" -f 4 | cut -d \" \" -f 2 '.format(self.lxc_bin,
                                                                                      container_name)
        # FAILS on shlex.split with parsing quoted delimiter "|" as it takes |
        ret = SysComm.exec_command(command)
        ip = None
        if len(ret['stderr']) > 3:
            self.logger.error("Getting container ip: {}".format(ret['stderr']))
            return ip
        ip = ret['stdout']
        return ip

    ###############################################################################
    # start_container
    # \param container_name: name of the container to start
    # \return true if operation went well
    ###############################################################################
    def start_container(self, container_name):
        if container_name not in self.instances.keys():
            self.logger.error("Container {} has not an instance".format(container_name))
            return False
        else:
            try:
                self.instances[container_name].start()
                # need a few secs until a v4 ip is assigned
                time.sleep(secs_between_steps)
                return True
            except LXDAPIException as e:
                self.logger.error("Could not start container {}. Error: {}",
                                  container_name, str(e))
                return False

    ###############################################################################
    # stop_container
    # \param container_name: name of the container to stop
    # \return true if operation went well
    ###############################################################################
    def stop_container(self, container_name):
        if container_name not in self.instances.keys():
            self.logger.error("Container {} has not an instance".format(container_name))
            return False
        else:
            try:
                container = self.instances[container_name]
                if container.status != 'Stopped':
                    container.stop()
                    # need a few secs until it is really stopped
                    time.sleep(secs_between_steps)
                return True
            except LXDAPIException as e:
                self.logger.error("Could not stop container {}. Error: {}",
                                  container_name, str(e))
                return False

    ###############################################################################
    # freeze_container
    # \param container_name: name of the container to freeze
    # \return true if operation went well
    ###############################################################################
    def freeze_container(self, container_name):
        if container_name not in self.instances.keys():
            self.logger.error("Container {} has not an instance".format(container_name))
            return False
        else:
            try:
                container = self.instances[container_name]
                if container.status == 'Running':
                    container.freeze()
                    # need a few secs until it is really freeze
                    time.sleep(secs_between_steps)
                else:
                    self.logger.info("Can not freeze as container status is not RUNNING")
                    return False
                return True
            except LXDAPIException as e:
                self.logger.error("Could not freeze container {}. Error: {}",
                                  container_name, str(e))
                return False

    ###############################################################################
    # unfreeze_container
    # \param container_name: name of the container to unfreeze
    # \return true if operation went well
    ###############################################################################
    def unfreeze_container(self, container_name):
        if container_name not in self.instances.keys():
            self.logger.error("Container {} has not an instance".format(container_name))
            return False
        else:
            try:
                container = self.instances[container_name]
                # if container.status == 'Frozen': status is Running! but on "lxc list" FROZEN
                if container.status == 'Running':
                    container.freeze()
                    # need a few secs until it is really freeze
                    time.sleep(secs_between_steps)
                else:
                    self.logger.info("Can not unfreeze as container status is not FROZEN")
                    return False
                return True
            except LXDAPIException as e:
                self.logger.error("Could not unfreeze container {}. Error: {}",
                                  container_name, str(e))
                return False

    ###############################################################################
    # get_container_ip
    # returns external ip of the container
    # \param container_name: name of the container
    # \return container's IP
    ###############################################################################
    def get_container_ip(self, container_name):
        if container_name not in self.instances.keys():
            self.logger.error("Container {} has not an instance".format(container_name))
            return None
        else:
            container = self.instances[container_name]
            if container.status != 'Running':
                self.logger.error(" container is not Running. State:".format(container_name))
                return None
            state = container.state()
            ip = state.network['eth0']['addresses'][0]['address']
            return ip
