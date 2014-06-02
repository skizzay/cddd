#!/bin/sh

top_dir=$PWD
config_dir=$(cd $top_dir/config && pwd)
for d in $(find -P . -mindepth 1 -type d) ; do
    if [ ! -e "$d/Makefile" -a -e "$d/software.mk" ] ; then
        cd $d
        ln -s $config_dir/Makefile .
	cd $top_dir
    fi
done
