#!/usr/bin/env python3
#
# Tool for check duplicated structure definition
# when copy a structure definition form one file to another file, source code will lost homogeneity
#
# Copyright (c) 2018, huawei Co.LTD
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
import struct

from optparse import OptionParser
from optparse import Option, OptionValueError

all_struct_definition_dict = {}
whitelist = []
count = 0
#init whitelist
def init_whitelist2list(list_string):
    global whitelist
    whitelist = list_string.split(",")

def is_file_in_white(file):
    for item in whitelist:
        if(file.find(item,0,len(file)) != -1):
            return True
    return False
#some file path has ../../, in this case ,we compare file name
def get_struct_filename_line_flag(line_in_rand_temp):
    struct = line_in_rand_temp.split(",")[0]
    full_filename = line_in_rand_temp.split(",")[1]
    filename = full_filename.split("/")[-1]
    line = line_in_rand_temp.split(",")[2]
    flag = line_in_rand_temp.split(",")[3]
    return struct,filename,line,flag

def remove_duplicate_line_dict_out(file):
    rFile = open(file, 'r')
    allLine = rFile.readlines()
    rFile.close()
# rm the file, which is created in gcc_plugin rand_struct.so ,and also been deleted in clean_all_rand_temp_file
    os.remove(file)
    line_dict = {}
    if(is_file_in_white(file) == True):
        return line_dict
    for oneline in allLine:
        struct,filename,line,flag = get_struct_filename_line_flag(oneline)
        if struct in line_dict:
#remove duplicated record in *.ramd_temp
            if line_dict[struct][0] != oneline:
                dict_struct,dict_filename,dict_line,dict_flag = get_struct_filename_line_flag(line_dict[struct][0])
                if (dict_filename != filename or dict_line != line or dict_flag != flag):
                    line_dict[struct].append(oneline)
        else:
# only compare the first item in list, because we want to find duplicated structure definition
            line_dict[struct] = [oneline]
    return line_dict
def clean_all_rand_temp_file (source_dir):
    global count
    for root, dirs, files in os.walk(source_dir, followlinks = True):
        for file in files:
            if (file.endswith(".rand_temp")):
                count = count + 1
                if (count % 50 == 0):
# delay,other wise IO too busy
                    time.sleep(0.05)
                os.remove(os.path.join(root, file))

def clean_all_rand_temp_file_by_shell (source_dir):
    global count
    errcode = 0
    for root, dirs, files in os.walk(source_dir, followlinks = True):
        for dir in dirs:
            new_dir = os.path.join(root, dir)
            cmd = "rm -f *.ramd_temp"
            current_dir = os.getcwd()
            os.chdir(new_dir)
            errcode = os.system(cmd)
            os.chdir(current_dir)
            if errcode != 0:
                print new_dir + " rm_file_error\n"
            if (count % 50 == 0):
                time.sleep(0.05)

def clean_all_rand_temp_file_all (source_dir):
    source_dir = source_dir.replace("//","/")
    clean_all_rand_temp_file(source_dir)

def clean_all_rand_temp_file_all_by_shell (source_dir):
    source_dir = source_dir.replace("//","/")
    clean_all_rand_temp_file_by_shell(source_dir)

def merge_line_dict(dict1,dict2):
    return dict(dict1, **dict2)

def walk_all_dir_get_all_struct_dict(source_dir):
    global count
    global all_struct_definition_dict
    source_dir = source_dir.replace("//","/")
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            if (file.endswith(".rand_temp")):
                count = count + 1
                if (count % 100 == 0):
# delay,other wise IO too busy
                    time.sleep(0.05)
                all_struct_definition_dict = merge_line_dict(all_struct_definition_dict,remove_duplicate_line_dict_out(os.path.join(root, file)))

def is_exist_duplicate_struct_definition(struct_dict):
    if isinstance(struct_dict, dict) == False:
        print "process *.rand_temp to dictionary failed!"
    for one in struct_dict:
        if len(struct_dict[one]) > 1:
            sys.exit("ERROR: struct "+one +" has duplicated definition in: "+struct_dict[one][0]+", other definition is in: "+struct_dict[one][1])
#            print "ERROR: struct "+one +" has duplicated definition in: "+struct_dict[one][0]+", other definition is in: "+struct_dict[one][1]

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
    usage = "$-s source_code,input kernel source code,-c clean rand_temp file"

    option_parser = OptionParser(option_class=MultipleOption, usage=usage)
    option_parser.add_option("-s", "--source_code", dest="source_code", metavar="FILE")
    option_parser.add_option("-c", "--clean", dest="clean", metavar="FILE")
    option_parser.add_option("-w", "--whitelist", dest="whitelist", metavar="FILE")
    (options, args) = option_parser.parse_args()


    if not options.source_code:
        sys.exit("Error: Must specify source_code file(s)\n" + option_parser.usage)
    if not os.path.exists(options.source_code):
        sys.exit("Error: source_code " + options.source_code + " does not exist\n"	+ option_parser.usage)
    if options.clean:
        clean_all_rand_temp_file(options.source_code)
        sys.exit(0)
    if options.whitelist:
        init_whitelist2list(options.whitelist)

    print options.whitelist
    walk_all_dir_get_all_struct_dict(options.source_code)
    walk_all_dir_get_all_struct_dict(options.source_code+"/../../vendor")
    walk_all_dir_get_all_struct_dict(options.source_code+"/../../device")
#    print all_struct_definition_dict
    print "check duplicated structure definition of kernel source code\n"
    is_exist_duplicate_struct_definition(all_struct_definition_dict)
    clean_all_rand_temp_file_all_by_shell(options.source_code)
#  clean agagin
    time.sleep(1)
    clean_all_rand_temp_file_all(options.source_code)
    print "duplicated structure definition check OK\n"
    sys.exit(0)


