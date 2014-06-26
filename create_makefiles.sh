#!/bin/sh

abspath () {
    path=$1
    directory=$(dirname $path)
    filename=$(basename $path)

    if [ -d $path ] ; then
        path=$(cd $directory && pwd)
    else
        path="$(cd $directory && pwd)/$filename"
    fi
    echo "$path"
}


top_dir=$(dirname $(abspath $0))
config_dir=$(cd $top_dir/config && pwd)
for d in $(find -P . -type d) ; do
    if [ ! -e "$d/Makefile" -a -e "$d/software.mk" ] ; then
        cd $d
        ln -s $config_dir/Makefile .
	cd $top_dir
    fi
done
