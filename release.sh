#!/bin/sh

# make a source release

svn export . mmlinuxserver
tar -czf mmlinuxserver-0.0.0.tar.gz mmlinuxserver
rm -rf mmlinuxserver
