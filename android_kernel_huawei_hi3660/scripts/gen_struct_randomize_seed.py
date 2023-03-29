#!/usr/bin/env python3
#
# Tool for generate a randomize struct  seed, from include/linux head file ,and sigend by x509 public key , then
# hash with sha256
#
# Copyright (c) 2018, huawei Co.Ltd
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# Authors:
#        yinyouzhan <yinyouzhan@huawei.com>
#

import argparse
import os
import sys
import time
import string
import re
import platform
from datetime import datetime

import hashlib
import subprocess
from optparse import OptionParser
from optparse import Option, OptionValueError
first_md5 = hashlib.md5()
last_md5 = hashlib.md5()
first_sha256 = hashlib.sha256()
md5_64bit = ''
count = 0
def read_all_file_context_gen_md5(source_dir):
    global first_md5
    global last_md5
    global count
    real_files = []
    source_dir = source_dir.replace("//","/")
    for root, dirs, files in os.walk(source_dir, followlinks = False):
        for file in files:
            if (file.endswith(".h") == False):
                continue
            if os.path.islink(os.path.join(root, file)) == True:
#                print "catched, file= "+ os.path.join(root, file)
                continue

            real_files.append(os.path.join(root, file))
    real_files.sort()

    for real_file in real_files:
        rFile = open(real_file, 'r')
        allLine = rFile.readlines()
        rFile.close()
        for oneline in allLine:
            count = count + 1
            if (count % 2 == 0):
                first_md5.update(oneline)
            else:
                last_md5.update(oneline)
    return first_md5.hexdigest() + last_md5.hexdigest()

def output_64bit_md5(out_file):
    global md5_64bit
    file_object = open(out_file, 'w')
    file_object.write(md5_64bit)
    file_object.close()
#generate sha256 from a signed md5 file
def read_signed_md5_gen_sha256(input_file):
    global first_sha256
    global count
    rFile = open(input_file, 'r')
    allLine = rFile.readlines()
    rFile.close()
    for oneline in allLine:
        first_sha256.update(oneline)

    return first_sha256.hexdigest()


class MultipleOption(Option):
    ACTIONS = Option.ACTIONS + ("extend",)
    STORE_ACTIONS = Option.STORE_ACTIONS + ("extend",)
    TYPED_ACTIONS = Option.TYPED_ACTIONS + ("extend",)
    ALWAYS_TYPED_ACTIONS = Option.ALWAYS_TYPED_ACTIONS + ("extend",)

    def take_action(self, action, dest, opt, value, values, option_parser):
        if action == "extend":
            values.ensure_value(dest, []).append(value)
        else:
            Option.take_action(self, action, dest, opt, value, values, option_parser)
if __name__ == '__main__':
    usage = "$-c source_code,input kernel source code"

    option_parser = OptionParser(option_class=MultipleOption, usage=usage)
    option_parser.add_option("-c", "--source_code", dest="source_code", metavar="FILE")
    option_parser.add_option("-o", "--outfile", dest="outfile", metavar="FILE")
    option_parser.add_option("-s", "--sha256", dest="sha256", metavar="FILE", action="extend", type="string")
    (options, args) = option_parser.parse_args()


    if not options.source_code and not options.sha256:
        sys.exit("Error: Must specify source_code file(s) or input sha256\n" + option_parser.usage)
    if not options.sha256 and not os.path.exists(options.source_code):
        sys.exit("Error: source_code " + options.source_code + " does not exist\n"	+ option_parser.usage)
    if not options.sha256:
        md5_64bit = read_all_file_context_gen_md5(options.source_code)
        if options.outfile:
            output_64bit_md5(options.outfile)

    if options.sha256:
        md5_64bit = read_signed_md5_gen_sha256(options.outfile)
        output_64bit_md5(options.outfile)
        print md5_64bit
    sys.exit(0)


