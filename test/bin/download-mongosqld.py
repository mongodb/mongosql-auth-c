"""
This script automates the process of downloading the latest BI Connector nigthly.
"""

import os
import sys
import urllib2


def download_binary(build_variant):
    """Downloads a binary from Amazon's S3.
    """
    file_name = "mongosqld"
    extension = "msi" if "win" in build_variant else "tgz"
    url = "https://s3.amazonaws.com/info-mongodb-com/mongodb-bi/v2/latest/mongodb-bi-%s-latest.%s" % (build_variant, extension)
    print url
    data = urllib2.urlopen(url)
    file_path = open(os.path.join(os.environ.get('MONGOSQLD_DOWNLOAD_DIR'), file_name), 'wb')
    meta = data.info()
    file_size = int(meta.getheaders("Content-Length")[0])
    block_sz = 8192
    while True:
        buf = data.read(block_sz)
        if not buf:
            break
        file_path.write(buf)
    file_path.close()


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
    if os.environ.get('MONGOSQLD_DOWNLOAD_DIR', None) is None:
        print("Can't find 'MONGOSQLD_DOWNLOAD_DIR' in environment variables")
        sys.exit(1)
    if os.environ.get('MONGOSQLD_BUILD_VARIANT', None) is None:
        print("Can't find 'MONGOSQLD_BUILD_VARIANT' in environment variables")
        sys.exit(1)


if __name__ == '__main__':
    ensure_env()
    build_variant = os.getenv('MONGOSQLD_BUILD_VARIANT')
    print("downloading build variant '%s'" % (build_variant))
    download_binary(build_variant)