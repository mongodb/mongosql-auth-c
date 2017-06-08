"""
This script automates the process of downloding the most recent sqlproxy releases
"""

import ast
import datetime
import getopt
import json
import os
import re
import subprocess
import sys
import tempfile
import urllib2

import requests

EVG_BASE = "https://evergreen.mongodb.com/rest/v1/"
UNITS = ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']
USAGE = """
SQLProxy Download tool 0.1
Usage:
  download-sqlproxy.py (-h | --help)
Options:
  -h --help     Show this screen.
"""


def ensure_env():
    """Ensures the required environment variables are set.
    """
    if os.environ.get('EVG_USER', None) is None:
        print("Can't find 'EVG_USER' in environment variables")
        sys.exit(1)
    if os.environ.get('EVG_KEY', None) is None:
        print("Can't find 'EVG_KEY' in environment variables")
        sys.exit(1)
    if os.environ.get('AWS_ACCESS_KEY_ID', None) is None:
        print("Can't find 'AWS_ACCESS_KEY_ID' in environment variables")
        sys.exit(1)
    if os.environ.get('AWS_SECRET_ACCESS_KEY', None) is None:
        print("Can't find 'AWS_SECRET_ACCESS_KEY' in environment variables")
        sys.exit(1)
    if os.environ.get('SQLPROXY_DOWNLOAD_DIR', None) is None:
        print("Can't find 'SQLPROXY_DOWNLOAD_DIR' in environment variables")
        sys.exit(1)
    if os.environ.get('SQLPROXY_BUILD_VARIANT', None) is None:
        print("Can't find 'SQLPROXY_BUILD_VARIANT' in environment variables")
        sys.exit(1)


class SQLProxyDownloader(object):
    """Represents a release instance.
    """
    def __init__(self, build_variant):
        """Initialize a new SQLProxyDownloader object.
        """
        self.__urls = {}
        self.__build_variant = build_variant

    def download_binaries(self):
        """Downloads the binaries from Evergreen.
        """
        for entry, url in self.__urls.items():
            print(entry)

            if entry != self.__build_variant:
                continue

            file_name = "sqlproxy"
            data = urllib2.urlopen(url)
            file_path = open(os.path.join(os.environ.get('SQLPROXY_DOWNLOAD_DIR'), file_name), 'wb')
            meta = data.info()
            file_size = int(meta.getheaders("Content-Length")[0])
            block_sz = 8192
            while True:
                buf = data.read(block_sz)
                if not buf:
                    break
                file_path.write(buf)
            file_path.close()

    def fetch_latest_version(self):
        url = "%s/projects/sqlproxy/versions" % (EVG_BASE)
        user_name = os.environ.get('EVG_USER', None)
        evg_key = os.environ.get('EVG_KEY', None)
        headers = {'Auth-Username': user_name, 'Api-Key': evg_key}
        print("Contacting Evergreen at %s" % (url))
        response = requests.get(url, headers=headers)
        if response.status_code != 200:
            print("Can't contact Evergreen at %s: %s" % (url, response.text))
            sys.exit(1)
        data = json.loads(response.text)
        self.__version = data["versions"][0]["version_id"]
        print("Using latest version '%s'") % (self.__version)

    def fetch_urls(self):
        """Fetches the release's binaries URL from the Evergreen API.
        """
        url = "%s/versions/%s" % (EVG_BASE, self.__version)
        user_name = os.environ.get('EVG_USER', None)
        evg_key = os.environ.get('EVG_KEY', None)
        headers = {'Auth-Username': user_name, 'Api-Key': evg_key}
        print("Contacting Evergreen at %s" % (url))
        response = requests.get(url, headers=headers)
        if response.status_code != 200:
            print("Can't contact Evergreen at %s: %s" % (url, response.text))
            sys.exit(1)
        versions = json.loads(response.text)
        builds = versions["builds"]

        if len(builds) == 0:
            print "No builds found for version '%s'" % (self.__version)
            sys.exit(0)

        for build_name in builds:
            # don't upload binaries for race buildvariant
            if "race" in build_name:
                print("Skipping build %s..." % (build_name))
                continue
            url = "%s/builds/%s" % (EVG_BASE, build_name)
            print("Fetching build %s..." % (url))
            response = requests.get(url, headers=headers)
            if response.status_code != 200:
                print("Can't contact Evergreen")
                response.raise_for_status()
            build = json.loads(response.text)
            task = build["tasks"]
            dist = task.get("dist", None)

            if dist is None:
                print("No dist task found for '%s'" % (build["name"]))
                continue

            status = dist.get("status", None)

            if status is None:
                print("No status found for '%s'" % (build["name"]))
                sys.exit(1)

            if status != "success":
                print("%s dist has status '%s', exiting..." % (build["name"], \
                    status))
                continue

            url = "%s/tasks/%s" % (EVG_BASE, dist["task_id"])
            print("Fetching task %s..." % (url))
            response = requests.get(url, headers=headers)

            if response.status_code != 200:
                print("Can't contact Evergreen")
                sys.exit(1)

            entry = json.loads(response.text)
            variant = entry["build_variant"]
            extension = ".msi" if "windows" in variant else ".tgz"
            for file in entry["files"]:
                url = file["url"]
                _, ext = os.path.splitext(url)
                if ext == extension:
                    self.__urls[variant] = url
                    break

    def run(self):
        """Runs an instance of the SQLProxy Downloader.
        """
        self.fetch_latest_version()
        self.fetch_urls()
        self.download_binaries()

if __name__ == '__main__':
    ensure_env()
    build_variant = os.getenv('SQLPROXY_BUILD_VARIANT')
    print("downloading build variant '%s'" % (build_variant))
    releaser = SQLProxyDownloader(build_variant)
    releaser.run()

