#!/bin/bash

get_distro ()
{
   if [ -f /etc/os-release ]; then
      . /etc/os-release
      DISTRO="${ID}-${VERSION_ID}"
   elif command -v lsb_release >/dev/null 2>&1; then
      name=$(lsb_release -s -i)
      if [ "$name" = "RedHatEnterpriseServer" ]; then # RHEL 6.2 at least
         name="rhel"
      fi
      version=$(lsb_release -s -r)
      DISTRO="${name}-${version}"
   elif [ -f /etc/redhat-release ]; then
      release=$(cat /etc/redhat-release)

      if [[ "$release" =~ "Red Hat" ]]; then
         name="rhel"
      elif [[ "$release" =~ "Fedora" ]]; then
         name="fedora"
      fi
      version=$(echo $release | sed 's/.*\([[:digit:]]\).*/\1/g')
      DISTRO="${name}-${version}"
   elif [ -f /etc/lsb-release ]; then
      . /etc/lsb-release
      DISTRO="${DISTRIB_ID}-${DISTRIB_RELEASE}"
   elif grep -R "Amazon Linux" "/etc/system-release" >/dev/null 2>&1; then
      DISTRO="amzn64"
   fi

   OS_NAME=$(uname -s)
   MARCH=$(uname -m)
   DISTRO=$(echo "$OS_NAME-$DISTRO-$MARCH" | tr '[:upper:]' '[:lower:]')

   echo $DISTRO
}

# get_mongodb_download_url_for "linux-distro-version-architecture" "latest|34|32"
# Sets EXTRACT to aproprate extract command
# Sets MONGODB_DOWNLOAD_URL to the aproprate download url
get_mongodb_download_url_for ()
{
   _DISTRO=$1
   _VERSION=$2

   VERSION_34="3.4.0"
   VERSION_32="3.2.11"

   EXTRACT="tar zxf"
   # getdata matrix on:
   # https://evergreen.mongodb.com/version/5797f0493ff12235e5001f05
   case "$_DISTRO" in
      darwin*)
         MONGODB_LATEST="http://downloads.10gen.com/osx/mongodb-osx-x86_64-enterprise-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/osx/mongodb-osx-x86_64-enterprise-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/osx/mongodb-osx-x86_64-enterprise-${VERSION_32}.tgz"
      ;;
      sunos*i86pc)
         MONGODB_LATEST="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-latest.tgz"
             MONGODB_34="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-${VERSION_34}.tgz"
             MONGODB_32="https://fastdl.mongodb.org/sunos5/mongodb-sunos5-x86_64-${VERSION_32}.tgz"
      ;;
      linux-rhel-7.2-s390x)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-rhel72-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-rhel72-${VERSION_34}.tgz"
             MONGODB_32=""
      ;;
      linux-rhel-7.1-ppc64le)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-rhel71-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-rhel71-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-rhel71-${VERSION_32}.tgz"
      ;;
      linux-rhel-8.0*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel80-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel80-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel80-${VERSION_32}.tgz"
      ;;
      linux-rhel-7.0*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel70-${VERSION_32}.tgz"
      ;;
      linux-rhel-6.2*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-rhel62-${VERSION_32}.tgz"
      ;;
      linux-rhel-5.5*)
         MONGODB_LATEST="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-rhel55-latest.tgz"
             MONGODB_34="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-rhel55-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.mongodb.org/linux/mongodb-linux-x86_64-rhel55-${VERSION_32}.tgz"
      ;;
      linux-sles-11*-x86_64)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-suse11-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-suse11-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-suse11-${VERSION_32}.tgz"
      ;;
      linux-sles-12.1-s390x)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-suse12-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-suse12-${VERSION_34}.tgz"
             MONGODB_32=""
      ;;
      linux-sles-12*-x86_64)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-suse12-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-suse12-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-suse12-${VERSION_32}.tgz"
      ;;
      linux-amzn64*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-amzn64-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-amzn64-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-amzn64-${VERSION_32}.tgz"
      ;;
      linux-debian-7*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-debian71-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-debian71-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-debian71-${VERSION_32}.tgz"
      ;;
      linux-debian-8*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-debian81-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-debian81-${VERSION_34}.tgz"
             MONGODB_32=""
      ;;
      linux-ubuntu-16.04-s390x)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-s390x-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32=""
      ;;
      linux-ubuntu-16.04-ppc64le)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-ppc64le-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32=""
      ;;
      linux-ubuntu-16.04-aarch64)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-arm64-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-arm64-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32=""
      ;;
      linux-ubuntu-16.04*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1604-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1604-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1604-${VERSION_32}.tgz"
      ;;
      linux-ubuntu-14.04*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1404-${VERSION_32}.tgz"
      ;;
      linux-ubuntu-12.04*)
         MONGODB_LATEST="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-latest.tgz"
             MONGODB_34="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-${VERSION_34}.tgz"
             MONGODB_32="http://downloads.10gen.com/linux/mongodb-linux-x86_64-enterprise-ubuntu1204-${VERSION_32}.tgz"
      ;;
      windows32*)
         EXTRACT="/cygdrive/c/Progra~2/7-Zip/7z.exe x"
         MONGODB_LATEST="https://fastdl.mongodb.org/win32/mongodb-win32-i386-latest.zip"
             MONGODB_34="https://fastdl.mongodb.org/win32/mongodb-win32-i386-${VERSION_34}.zip"
             MONGODB_32="https://fastdl.mongodb.org/win32/mongodb-win32-i386-${VERSION_32}.zip"
      ;;
      windows64*)
         EXTRACT="/cygdrive/c/Progra~2/7-Zip/7z.exe x"
         MONGODB_LATEST="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-latest.zip"
             MONGODB_34="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-${VERSION_34}.zip"
             MONGODB_32="https://fastdl.mongodb.org/win32/mongodb-win32-x86_64-2008plus-${VERSION_32}.zip"
      ;;
      cygwin*)
         EXTRACT="/cygdrive/c/Progra~2/7-Zip/7z.exe x"
         MONGODB_LATEST="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-latest.zip"
             MONGODB_34="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-${VERSION_34}.zip"
             MONGODB_32="http://downloads.10gen.com/win32/mongodb-win32-x86_64-enterprise-windows-64-${VERSION_32}.zip"
      ;;
   esac

   # Fallback to generic Linux x86_64 builds (without SSL) when no platform specific link is available.
   case "$_DISTRO" in
      *linux*x86_64)
         MONGODB_LATEST=${MONGODB_LATEST:-"http://downloads.mongodb.org/linux/mongodb-linux-x86_64-latest.tgz"}
                 MONGODB_34=${MONGODB_34:-"http://downloads.mongodb.org/linux/mongodb-linux-x86_64-${VERSION_34}.tgz"}
                 MONGODB_32=${MONGODB_32:-"http://downloads.mongodb.org/linux/mongodb-linux-x86_64-${VERSION_32}.tgz"}
      ;;
   esac

   MONGODB_DOWNLOAD_URL=""
   case "$_VERSION" in
      latest) MONGODB_DOWNLOAD_URL=$MONGODB_LATEST ;;
      3.4) MONGODB_DOWNLOAD_URL=$MONGODB_34 ;;
      3.2) MONGODB_DOWNLOAD_URL=$MONGODB_32 ;;
   esac

   [ -z "$MONGODB_DOWNLOAD_URL" ] && MONGODB_DOWNLOAD_URL="Unknown version: $_VERSION for $_DISTRO"

   echo $MONGODB_DOWNLOAD_URL
}

download_and_extract ()
{
   MONGODB_DOWNLOAD_URL=$1
   EXTRACT=$2

   cd $ARTIFACTS_DIR
   curl $MONGODB_DOWNLOAD_URL \
        --output mongodb-binaries.tgz \
        --silent \
        --fail \
        --max-time 60 \
        --retry 5 \
        --retry-delay 0

   $EXTRACT mongodb-binaries.tgz

   rm mongodb-binaries.tgz
   mv mongodb* mongodb
   chmod -R +x mongodb
   find . -name vcredist_x64.exe -exec {} /install /quiet \;
   ./mongodb/bin/mongod --version
   cd -
}
