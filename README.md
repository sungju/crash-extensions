# crash extensions #

### pstree ###

print tree list of processes

How to use)

```
$ make

crash> extend ./pstree.so

crash> man pstree

NAME
  pstree - print process list in tree

SYNOPSIS
  pstree [-p][-g][-s] [pid] ...

DESCRIPTION
  This command prints process list in tree

  The list can be modified by the following options

    -p  print process ID
    -g  print thread group instead of each threads
    -s  print process status

EXAMPLE
  Print out process list

    crash> pstree
    init --+-- swapd
           +-- httpd
           +-...


crash> pstree 3636
# of processes : 535
gnome-terminal- -+- gnome-pty-helpe 
                 |- bash -+- su -+- bash -+- crash 
                 |- bash 
                 |- bash -+- su -+- bash 
                 `- bash -+- vim 

```
