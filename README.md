# s9s-tools

Repository for tools helping to manage and monitor your Severalnines clusters. 
The repository contains the following tools:
- s9s  (cli)

# APT and YUM repositories

Well these repositories are updated on few weekly basis currently.

## Ubuntu DEB repository

LINK: https://launchpad.net/~severalnines/+archive/ubuntu/s9s-tools

```
sudo add-apt-repository ppa:severalnines/s9s-tools
sudo apt-get update
sudo apt-get install s9s-tools
```

## Debian (+Ubuntu) DEB repositories

See http://download.opensuse.org/repositories/home:/kedazo/ 

To add debian/ubuntu repos the following needs to be done
```
# See possible distros from URL: http://download.opensuse.org/repositories/home:/kedazo/ 
DISTRO=Debian_8.0
wget -qO - http://download.opensuse.org/repositories/home:/kedazo/${DISTRO}/Release.key | sudo apt-key add -
echo "deb http://download.opensuse.org/repositories/home:/kedazo/${DISTRO}/ ./" | sudo tee /etc/apt/sources.list.d/s9s-tools.list
sudo apt-get update
sudo apt-get install s9s-tools
```

## YUM Repositories for CentOS 6/7 and RHEL 6/7

LINK: https://build.opensuse.org/repositories/home:kedazo/s9s-tools

The repository files for each distribution:
- http://download.opensuse.org/repositories/home:/kedazo/CentOS_6/home:kedazo.repo
- http://download.opensuse.org/repositories/home:/kedazo/CentOS_7/home:kedazo.repo
- http://download.opensuse.org/repositories/home:/kedazo/RHEL_6/home:kedazo.repo
- http://download.opensuse.org/repositories/home:/kedazo/RHEL_7/home:kedazo.repo

# Some screenshots

![Screenshot01](screenshots/screen-01.png)
![Screenshot02](screenshots/screen-02.png)
![Screenshot03](screenshots/screen-03.png)
![Screenshot04](screenshots/screen-04.png)
![Screenshot05](screenshots/screen-05.png)
![Screenshot06](screenshots/screen-06.png)
![Screenshot07](screenshots/screen-07.png)
![Screenshot08](screenshots/screen-08.png)

