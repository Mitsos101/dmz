dmz - v0.1
===========

**Note: This software was written for personal educational purposes, meaning 
it might contain bugs and not work as intended. Use with caution. There are 
also other tools out there which serve the same purpose/use; you might be 
willing to choose those for production environments or daily use.**

A simple tool to easily manage and daemonize userspace applications on 
linux operating systems.


# Usage
```
Usage: dmz [option(s)] [argument] ...
-e [cmd]       - Daemonizes a shell command and redirects its output to a file.
                    e.g. dmz -e "/usr/bin/dmesg --follow"
-t             - Optional which goes along with -e. Appends a timestamp on each line on daemon's output.
-l             - Lists all running daemons initiated by dmz.
-d [dmz_id]    - Destroys a daemon by sending a SIGKILL signal to its process and deleting its log file.
                 Use the -l option to check dmz ids and -s to save its log file.
-s [dir_path]  - Optional. Used along with -d to keep a log file, moves the file (with filename its dmz id) to that directory.
-h             - Displays this usage message.
```

# Installation
Just clone this git repository and:

```gcc dmz.c -o dmz```

Or use some other compiler of your choice.


# License
GNU GPL v3, see LICENSE for more info.
