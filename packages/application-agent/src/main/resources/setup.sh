#!/bin/sh

# Application Agent install script
# Outline:
# 1.  load existing environment configuration
# 2.  source the "functions.sh" file:  mtwilson-linux-util-*.sh
# 3.  look for ~/tbootxm.env and source it if it's there
# 4.  force root user installation
# 5.  define application directory layout 
# 6.  backup current configuration and data, if they exist
# 7.  create application directories and set folder permissions
# 8.  store directory layout in env file
# 9.  install prerequisites
# 10. unzip tbootxm archive tbootxm-zip-*.zip into TBOOTXM_HOME, overwrite if any files already exist
# 11. copy utilities script file to application folder
# 12. set additional permissions
# 13. validate correct kernel version
# 14. run additional setup tasks

#####

# default settings
# note the layout setting is used only by this script
# and it is not saved or used by the app script
export TBOOTXM_HOME=${TBOOTXM_HOME:-/opt/tbootxm}
TBOOTXM_LAYOUT=${TBOOTXM_LAYOUT:-home}

# the env directory is not configurable; it is defined as TBOOTXM_HOME/env and
# the administrator may use a symlink if necessary to place it anywhere else
export TBOOTXM_ENV=$TBOOTXM_HOME/env

# load application environment variables if already defined
if [ -d $TBOOTXM_ENV ]; then
  TBOOTXM_ENV_FILES=$(ls -1 $TBOOTXM_ENV/*)
  for env_file in $TBOOTXM_ENV_FILES; do
    . $env_file
    env_file_exports=$(cat $env_file | grep -E '^[A-Z0-9_]+\s*=' | cut -d = -f 1)
    if [ -n "$env_file_exports" ]; then eval export $env_file_exports; fi
  done
fi

# functions script (mtwilson-linux-util-4.0-SNAPSHOT.sh) is required
# we use the following functions:
# java_detect java_ready_report 
# echo_failure echo_warning
# register_startup_script
UTIL_SCRIPT_FILE=$(ls -1 mtwilson-linux-util-*.sh | head -n 1)
if [ -n "$UTIL_SCRIPT_FILE" ] && [ -f "$UTIL_SCRIPT_FILE" ]; then
  . $UTIL_SCRIPT_FILE
fi

define_grub_file

# load installer environment file, if present
if [ -f ~/tbootxm.env ]; then
  echo "Loading environment variables from $(cd ~ && pwd)/tbootxm.env"
  . ~/tbootxm.env
  env_file_exports=$(cat ~/tbootxm.env | grep -E '^[A-Z0-9_]+\s*=' | cut -d = -f 1)
  if [ -n "$env_file_exports" ]; then eval export $env_file_exports; fi
else
  echo "No environment file"
fi

# enforce root user installation
if [ "$(whoami)" != "root" ]; then
  echo_failure "Running as $(whoami); must install as root"
  exit -1
fi

# define application directory layout
if [ "$TBOOTXM_LAYOUT" == "linux" ]; then
  export TBOOTXM_CONFIGURATION=${TBOOTXM_CONFIGURATION:-/etc/tbootxm}
  export TBOOTXM_REPOSITORY=${TBOOTXM_REPOSITORY:-/var/opt/tbootxm}
  export TBOOTXM_LOGS=${TBOOTXM_LOGS:-/var/log/tbootxm}
elif [ "$TBOOTXM_LAYOUT" == "home" ]; then
  export TBOOTXM_CONFIGURATION=${TBOOTXM_CONFIGURATION:-$TBOOTXM_HOME/configuration}
  export TBOOTXM_REPOSITORY=${TBOOTXM_REPOSITORY:-$TBOOTXM_HOME/repository}
  export TBOOTXM_LOGS=${TBOOTXM_LOGS:-$TBOOTXM_HOME/logs}
fi
export TBOOTXM_BIN=$TBOOTXM_HOME/bin
export TBOOTXM_JAVA=$TBOOTXM_HOME/java
export TBOOTXM_LIB=$TBOOTXM_HOME/lib

# note that the env dir is not configurable; it is defined as "env" under home
export TBOOTXM_ENV=$TBOOTXM_HOME/env

tbootxm_backup_configuration() {
  if [ -n "$TBOOTXM_CONFIGURATION" ] && [ -d "$TBOOTXM_CONFIGURATION" ] &&
    (find "$TBOOTXM_CONFIGURATION" -mindepth 1 -print -quit | grep -q .); then
    datestr=`date +%Y%m%d.%H%M`
    backupdir=/var/backup/tbootxm.configuration.$datestr
    mkdir -p "$backupdir"
    cp -r $TBOOTXM_CONFIGURATION $backupdir
  fi
}

tbootxm_backup_repository() {
  if [ -n "$TBOOTXM_REPOSITORY" ] && [ -d "$TBOOTXM_REPOSITORY" ] &&
    (find "$TBOOTXM_REPOSITORY" -mindepth 1 -print -quit | grep -q .); then
    datestr=`date +%Y%m%d.%H%M`
    backupdir=/var/backup/tbootxm.repository.$datestr
    mkdir -p "$backupdir"
    cp -r $TBOOTXM_REPOSITORY $backupdir
  fi
}

# backup current configuration and data, if they exist
tbootxm_backup_configuration
tbootxm_backup_repository

# create application directories (chown will be repeated near end of this script, after setup)
for directory in $TBOOTXM_HOME $TBOOTXM_CONFIGURATION $TBOOTXM_REPOSITORY $TBOOTXM_JAVA $TBOOTXM_BIN $TBOOTXM_LOGS $TBOOTXM_ENV $TBOOTXM_LIB; do
  mkdir -p $directory
  chmod 700 $directory
done

# store directory layout in env file
echo "# $(date)" > $TBOOTXM_ENV/tbootxm-layout
echo "export TBOOTXM_HOME=$TBOOTXM_HOME" >> $TBOOTXM_ENV/tbootxm-layout
echo "export TBOOTXM_CONFIGURATION=$TBOOTXM_CONFIGURATION" >> $TBOOTXM_ENV/tbootxm-layout
echo "export TBOOTXM_REPOSITORY=$TBOOTXM_REPOSITORY" >> $TBOOTXM_ENV/tbootxm-layout
echo "export TBOOTXM_JAVA=$TBOOTXM_JAVA" >> $TBOOTXM_ENV/tbootxm-layout
echo "export TBOOTXM_BIN=$TBOOTXM_BIN" >> $TBOOTXM_ENV/tbootxm-layout
echo "export TBOOTXM_LOGS=$TBOOTXM_LOGS" >> $TBOOTXM_ENV/tbootxm-layout
echo "export TBOOTXM_LIB=$TBOOTXM_LIB" >> $TBOOTXM_ENV/tbootxm-layout

# make sure unzip and authbind are installed
TBOOTXM_YUM_PACKAGES="zip unzip dos2unix perl"
TBOOTXM_APT_PACKAGES="zip unzip dos2unix perl"
TBOOTXM_YAST_PACKAGES="zip unzip"
TBOOTXM_ZYPPER_PACKAGES="zip unzip dos2unix perl"
auto_install "Installer requirements" "TBOOTXM"
if [ $? -ne 0 ]; then echo_failure "Failed to install prerequisites through package installer"; exit -1; fi

# delete existing java files, to prevent a situation where the installer copies
# a newer file but the older file is also there
if [ -d $TBOOTXM_HOME/java ]; then
  rm $TBOOTXM_HOME/java/*.jar 2>/dev/null
fi

# extract tbootxm  (tbootxm-zip-5.0-SNAPSHOT.zip)
echo "Extracting application..."
TBOOTXM_ZIPFILE=`ls -1 tbootxm-*.zip 2>/dev/null | head -n 1`
unzip -oq $TBOOTXM_ZIPFILE -d $TBOOTXM_HOME

## EXTRACT WML BINARIES
echo "Extracting wml binaries..."
MTWILSON_WML_PACKAGE=`ls -1 lib-workload-measurement-*.zip 2>/dev/null | head -n 1`
if [ -z "$MTWILSON_WML_PACKAGE" ]; then
  echo_failure "Failed to find wml zip package"
  exit -1
fi
unzip -oq $MTWILSON_WML_PACKAGE -d $TBOOTXM_HOME
if [ $? -ne 0 ]; then echo_failure "Failed to extract wml zip package"; exit -1; fi

# copy utilities script file to application folder
cp $UTIL_SCRIPT_FILE $TBOOTXM_HOME/bin/functions.sh

# set permissions
chmod 700 $TBOOTXM_HOME/bin/*

### INSTALL RPMMIO MODULES 
#echo "Installing rpmmio modules..."
#MTWILSON_RPMMIO_PACKAGE=`ls -1 tbootxm-rpmmio-*.bin 2>/dev/null | tail -n 1`
#if [ -z "$MTWILSON_RPMMIO_PACKAGE" ]; then
#  echo_failure "Failed to find rpmmio module package"
#  exit -1
#fi
#./$MTWILSON_RPMMIO_PACKAGE
#if [ $? -ne 0 ]; then echo_failure "Failed to install rpmmio module package"; exit -1; fi

# fix_libcrypto for UBUNTU18.04
# UBUNTU18.04 ISSUE:
# While generating initrd via the installer, the libcrypto.so.1.0.0 library cannot be
# found in /lib/x86_64-linux-gnu. Solution is to create a missing symlink in /lib/x86_64-linux-gnu.
# So in general, what we want to do is:
# 1. identify the location of libcrypto.so.1.0.0 library
# 2. identify which lib directory it's in (/usr/lib/x86_64-linux-gnu, etc)
# 3. create a symlink from /usr/lib/x86_64-linux-gnu/libcrypto.so.1.0.0 to /lib/x86_64-linux-gnu/libcrypto.so.1.0.0
fix_libcrypto() {
  local has_libcrypto_lib=`find /lib/x86_64-linux-gnu/ -name libcrypto.so.1.0.0 2>/dev/null | head -1`
  if [ -z "$has_libcrypto_lib" ]; then
    local has_libcrypto=`find / -name libcrypto.so.1.0.0 2>/dev/null | head -1`
    if [ -n "$has_libcrypto" ]; then
      echo "Creating missing symlink for $has_libcrypto"
      ln -s $has_libcrypto /lib/x86_64-linux-gnu/libcrypto.so.1.0.0
    fi
  fi
}

if aptget_detect; then
  if [ "$(whoami)" == "root" ]; then
    fix_libcrypto
  fi
fi

# Generate initrd
$TBOOTXM_BIN/generate_initrd.sh
if [ $? -ne 0 ]; then echo_failure "Failed to generate initrd"; exit -1; fi

# Configure host
$TBOOTXM_BIN/configure_host.sh
if [ $? -ne 0 ]; then echo_failure "Failed to configure host with tbootxm"; exit -1; fi

echo "Updating ldconfig for WML library"
echo "$TBOOTXM_HOME/lib" > /etc/ld.so.conf.d/wml.conf
ldconfig
if [ $? -ne 0 ]; then echo_warning "Failed to load ldconfig. Please run command "ldconfig" after installation completes."; fi

echo_success "Application Agent Installation complete"
