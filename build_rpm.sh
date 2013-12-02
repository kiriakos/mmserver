#!/bin/bash
set -e

RPMBUILD_DIR=`rpm --eval %{_topdir}`

pushd `dirname $0` > /dev/null
MMSERVER_RPM_DIR=$(pwd)
popd > /dev/null

cd $RPMBUILD_DIR/SPECS
cp $MMSERVER_RPM_DIR/*.spec .
cd -

spectool -g -R $RPMBUILD_DIR/SPECS/mmserver.spec
rpmbuild --clean -ba $RPMBUILD_DIR/SPECS/mmserver.spec
