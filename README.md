# mongosql-auth-c
A MySQL authentication plugin that implements the client-side of MongoDB-supported authentication mechanisms for the BI Connector.

Version 1.0 of this plugin supports the following mechanisms:

- SCRAM-SHA-1
- PLAIN
- GSSAPI

## Supported Platforms

The plugin is built and tested on the following platforms (all x86\_64):

- Windows 2008 R2
- OSX 10.12
- Ubuntu 14.04
- RHEL 7.0

The plugin is built against MySQL 5.7.18 Community Edition (64-bit),
and tested with MySQL 5.7.18 Community Edition and the MongoDB Connector for BI 2.2.

## Installation

The plugin tarball/installer can be downloaded from the [releases page](https://github.com/mongodb/mongosql-auth-c/releases).

### Windows

First, download the MySQL 5.7.18 [installer](https://dev.mysql.com/downloads/file/?id=470091) and install the products you need.
Then, install the plugin using the msi installer provided on the releases page.

### OSX and Linux

The plugin library (`mongosql_auth.so`) should be installed in `<mysql home>/lib/plugin/`.
The default location of `<mysql home>` varies by platform, but the location of the plugin directory can be verified by running `mysql_config --plugindir`.

Alternatively, the plugin can be installed in a directory of your choice if you provide the `plugin-dir=<your_install_dir>` option to your MySQL client.

On Linux, ensure that the `libgssapi_krb5` shared library is installed and in the default search path if the GSSAPI mechanism is desired.

## Usage

This plugin can be used with the 64-bit version of the the MySQL shell and the MySQL ODBC driver.

### Parameters

Authentication parameters can be specified on the user name field.

**mechanism** (optional)

*Default: `SCRAM-SHA-1`*

The authentication mechanism to use. Supported mechanisms are `PLAIN`, `SCRAM-SHA-1`, and `GSSAPI`.

Example PLAIN authentication:
```
mysql --default-auth=mongosql_auth -u "username?mechanism=PLAIN"
```

For GSSAPI authentication, a hostname (not an IP address) is required to form the service principal name (SPN) of the BI Connector service, `serviceName/hostname@REALM`. For example:

```
mysql --default-auth=mongosql_auth -u "username?mechanism=GSSAPI" -h mongosql.example.com
```

This authenticates the user principal `username@DEFAULT.REALM.COM` to the service principal `mongosql/mongosql.example.com@DEFAULT.REALM.COM`.

**serviceName** (optional)

*Default: `mongosql`*

The service name of the MongoDB BI Connector. Used for the `GSSAPI` mechanism only.

For example:

```
mysql --default-auth=mongosql_auth -u "username?mechanism=GSSAPI&serviceName=mongosqlservice" -h mongosql.example.com
```

**source** (optional)

*Default: `$external`*

The authentication source to use.  For the GSSAPI and PLAIN mechanisms, the required source is `$external`.

For example:

```
mysql --default-auth=mongosql_auth -u "username?mechanism=SCRAM-SHA-1&source=somedb"
```

### default-auth

To authenticate with `mongosqld` using the `mongosql_auth` plugin, you will need to provide the `default-auth=mongosql_auth` option to your MySQL client.
There are a number of ways to accomplish this, depending on which client program you are using.

#### Command-Line Flag

With the MySQL shell, the default-auth option can be specified as follows:

```
mysql -uusername -ppassword --default-auth=mongosql_auth
```

#### Kerberos GSSAPI Authentication on Unix

To use Kerberos GSSAPI authentication on OSX/Linux, the following considerations must be made:

- A valid `krb5.conf` configuration is required (exact location is platform dependent). The default realm will be used for GSSAPI authentication.
  - See [krb5.conf](test/resources/gssapi/krb5.conf) as an example configuration.
  - On OSX, an entry in /etc/hosts may be required to resolve the hostname of the KDC. For example:
    - `12.34.56.78 realm.example.com`
  - To specify a custom Kerberos configuration, the `KRB5_CONFIG` environment variable will be used.
- The credential acquisition process is as follows:
  - If a password is provided to `mysql`, the plugin will use it to obtain a credential from the KDC.
    - *This requires the `forwardable = true` setting in the `[libdefaults]` section of the krb5.conf file*.
  - If no password is provided to `mysql`, the plugin will use a credential in the default credential cache, created by a preceding call to `kinit`.
  - If no credentials are found in the cache, the plugin will use the default client keytab (KRB5_CLIENT_KTNAME) if provided to obtain a credential from the KDC.
    - This only works on MIT Linux Kerberos.
    - *This requires the `forwardable = true` setting in the `[libdefaults]` section of the krb5.conf file*
- On Linux and MacOS, `kinit` must be run with the `-f` flag to request a forwardable (delegatable) TGT.
  - For example: `kinit -f myuser@EXAMPLE.REALM.COM`

For debugging, Kerberos information can be logged by setting the environment variable `KRB5_TRACE` to a file path.

#### ODBC Connection Parameter

If you are using the MySQL ODBC driver, the interface you use to configure your DSN may provide a field where you can specify the default auth plugin to use.
Specifying `mongosql_auth` here will cause the ODBC driver to use the `mongosql_auth` plugin by default.

#### Configuration File

MySQL configuration files can be found in many locations, as enumerated [here](https://dev.mysql.com/doc/refman/5.7/en/option-files.html).
In one of these files, add a line with `default-auth=mongosql_auth` to the `[client]` section (or create it if it doesn't yet exist).
To use this same configuration file with an ODBC DSN, provide the `USE_MYCNF=1` connection parameter to your ODBC DSN.


## License
Copyright (c) 2018 MongoDB Inc.
Dual licensed under the Apache and GPL licenses.
