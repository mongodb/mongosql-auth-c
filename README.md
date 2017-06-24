# mongosql-auth-c
A MySQL authentication plugin that implements the client-side of MongoDB-supported authentication mechanisms for the BI Connector.

Version 1.0 of this plugin supports the following mechanisms:

- SCRAM-SHA-1
- PLAIN

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

## Usage

This plugin can be used with the 64-bit version of the the MySQL shell and the MySQL ODBC driver.

### Parameters

Optionally, specify the authentication mechanism via a query parameter on the user name.
For example:

```
username?mechanism=PLAIN
```

Optionally, specify the authentication source via a query parameter on the user name.
For example:

```
username?source=somedb
```

### default-auth

To authenticate with `mongosqld` using the `mongosql_auth` plugin, you will need to provide the `default-auth=mongsql_auth` option to your MySQL client.
There are a number of ways to accomplish this, depending on which client program you are using.

#### Command-Line Flag

With the MySQL shell, the default-auth option can be specified as follows:

```
mysql -uusername -ppassword --default-auth=mongosql_auth
```

#### ODBC Connection Parameter

If you are using the MySQL ODBC driver, the interface you use to configure your DSN may provide a field where you can specify the default auth plugin to use.
Specifying `mongosql_auth` here will cause the ODBC driver to use the `mongosql_auth` plugin by default.

#### Configuration File

MySQL configuration files can be found in many locations, as enumerated [here](https://dev.mysql.com/doc/refman/5.7/en/option-files.html).
In one of these files, add a line with `default-auth=mongosql_auth` to the `[client]` section (or create it if it doesn't yet exist).
To use this same configuration file with an ODBC DSN, provide the `USE_MYCNF=1` connection parameter to your ODBC DSN.
