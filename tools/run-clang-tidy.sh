#!/bin/sh
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

set -xe

BUILDDIR=$1; shift 1

function sanitize_compile_commands
{
    local cc_file=${BUILDDIR}/compile_commands.json
    local filter_file=".clang-tidy-ignore"

    if [ ! -f "${cc_file}" ]; then
        >&2 echo "Couldn't find compile_commands.json"
        exit 1
    fi

    if [ ! -f "${filter_file}" ]; then
        return 0
    fi

    filter_files=$(cat ${filter_file} | tr '\n' '|' | head -c -1)

    local cc_bak_file=${cc_file}.bak
    mv ${cc_file} ${cc_bak_file}

    cat ${cc_bak_file} | jq -r "map(select(.file|test(\"${filter_files}\")|not))" > ${cc_file}
}

sanitize_compile_commands

cd ${BUILDDIR}
run-clang-tidy -j$(nproc) -q $@
