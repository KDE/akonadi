#!/bin/bash
# Copyright (c) 2020 Daniel Vrátil <dvratil@kde.org>
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Library General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
# License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
# Inspired by https://pspdfkit.com/blog/2018/using-clang-tidy-and-integrating-it-in-jenkins/

if [ $# -lt 2 ]; then
    >&2 echo "Usage: $0 SRC_DIR BUILD_DIR [TARGET_BRANCH]"
    exit 1
fi

set -xe

if [ ! -d .git ]; then
    >&2 echo "run-clang-tidy.sh must be ran in a git clone of the Akonadi repository!"
    exit 1
fi

dir=$(dirname $(readlink -f "$0"))
src_dir=$1; shift
build_dir=$1; shift
if [ $# -ge 1 ]; then
    merge_branch=$1; shift
else
    merge_branch="master"
fi
base_commit=$(git merge-base refs/remotes/origin/${merge_branch} HEAD)
changed_files=$(git diff-tree --name-only --diff-filter=d -r ${base_commit} HEAD \
                    | grep -E "\.cpp|\.h" \
                    | grep -Ev "^autotests/|^tests/")

parallel run-clang-tidy -q -p ${build_dir} {} ::: ${changed_files} | tee "${build_dir}/clang-tidy.log"

cat "${build_dir}/clang-tidy.log" | ${dir}/clang-tidy-to-junit.py ${src_dir} > "${build_dir}/clang-tidy-report.xml"
