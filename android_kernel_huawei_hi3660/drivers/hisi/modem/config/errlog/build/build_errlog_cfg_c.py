#!/usr/bin/python
#-*- coding: UTF-8 -*-
import os
import sys
import shutil
import time
import hashlib
import xml.dom.minidom
from xml.dom import Node
from xml.dom import minidom


os.system("title errlog xml decode~")

reload(sys)
sys.setdefaultencoding('gb18030')

#*****************************************************************************
#  宏定义
#*****************************************************************************
PYTHON_SEARCH_HASH_FLAG_START="/* TODO: g_aulModemErrRept hash_value:"
PYTHON_SEARCH_FLAG_START="/* TODO: python search flag satrt */"
PYTHON_SEARCH_FLAG_END  ="/* TODO: python search flag end */"
PATH_ERR_LOG_CFG_H        ='..\errlog_faultiddef.h'
PATH_ERR_LOG_CFG_C        ='..\errlog_cfg.c'

#*****************************************************************************
#  全局变量声明
#*****************************************************************************
PATH_ERR_LOG_XML_ARRY = ['..\errlog_cfg_xml\GUTL_ErrLog_Cfg.xml','..\errlog_cfg_xml\PAM_ErrLog_Cfg.xml','..\errlog_cfg_xml\NAS_ErrLog_Cfg.xml','..\errlog_cfg_xml\CTTF_ErrLog_Cfg.xml','..\errlog_cfg_xml\CAS_ErrLog_Cfg.xml','..\errlog_cfg_xml\CSDR_ErrLog_Cfg.xml','..\errlog_cfg_xml\CPROC_ErrLog_Cfg.xml']

def decode_xml_by_faultId(dict):
    '''
     函 数 名  : decode_xml_by_faultId
     功能描述  : 根据faultid解析对应xml
     输入参数  : dict

     输出参数  :
     返 回 值  : errlog_cfg.c中fault关联列表

     修改历史      :
      1.日    期   : 2016年2月18日
        作    者   : d00212987
        修改内容   : ERR LOG FAULT ID关联项目新增
    '''
    #print dict

    str_cdma_all ='\r\n#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)'
    str_gu_all = ''
    str1_cdma_all = '\r\n#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)'
    str1_gu_all = ''
    str2_cdma_all = '\r\n#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)'
    str2_gu_all = ''
    #反序
    dd =sorted(dict.items(),key=lambda e:e[1],reverse=True)
    for fault_id_name in dd:
        #print fault_id_name,dict[fault_id_name]
        str_cdma =''
        str1_cdma =''
        str2_cdma =''
        str_gu  =''
        str1_gu  =''
        str2_gu  =''
        arrycount = []
        arrycount = [0]*6
        for path_xml in  PATH_ERR_LOG_XML_ARRY:
            #打开xml文档
            dom = xml.dom.minidom.parse(path_xml);

            #得到文档元素对象
            root = dom.documentElement
            modem = root.getAttribute("MODE")

            bb = root.getElementsByTagName('errlog')
            if ('CDMA' == modem):

                for b in bb:
                    #print b.nodeName
                    type = b.getAttribute("TYPE")
                    #print type

                    fault_id=b.getAttribute("FAULT_ID")
                    #print fault_id
                    if(fault_id_name[0] != fault_id):
                       continue
                    if(0 == int(b.getAttribute("MODME_NUME"))):
                        #print modem_number
                        str_cdma+='    {'+fault_id+', '

                        pid=b.getAttribute("PID")
                        #print pid
                        str_cdma+=pid+', '

                        alarm_id=b.getAttribute("ALARM_ID")
                        #print alarm_id
                        str_cdma+=alarm_id+'},\n'
                        arrycount[0]+=1;
                    if(1 == int(b.getAttribute("MODME_NUME"))):
                        #print modem_number
                        str1_cdma+='    {'+fault_id+', '

                        pid=b.getAttribute("PID")
                        #print pid
                        str1_cdma+=pid+', '

                        alarm_id=b.getAttribute("ALARM_ID")
                        #print alarm_id
                        str1_cdma+=alarm_id+'},\n'
                        arrycount[1]+=1;
                    if(2 == int(b.getAttribute("MODME_NUME"))):
                        #print modem_number
                        str2_cdma+='    {'+fault_id+', '

                        pid=b.getAttribute("PID")
                        #print pid
                        str2_cdma+=pid+', '

                        alarm_id=b.getAttribute("ALARM_ID")
                        #print alarm_id
                        str2_cdma+=alarm_id+'},\n'
                        arrycount[2]+=1;
            else:
                for b in bb:
                    #print b.nodeName
                    type = b.getAttribute("TYPE")
                    #print type

                    fault_id=b.getAttribute("FAULT_ID")
                    #print fault_id
                    if(fault_id_name[0] != fault_id):
                       continue
                    if(0 == int(b.getAttribute("MODME_NUME"))):
                        #print modem_number
                        str_gu+='    {'+fault_id+', '

                        pid=b.getAttribute("PID")
                        #print pid
                        str_gu+=pid+', '

                        alarm_id=b.getAttribute("ALARM_ID")
                        #print alarm_id
                        str_gu+=alarm_id+'},\n'
                        arrycount[3]+=1;
                    if(1 == int(b.getAttribute("MODME_NUME"))):
                        #print modem_number
                        str1_gu+='    {'+fault_id+', '

                        pid=b.getAttribute("PID")
                        #print pid
                        str1_gu+=pid+', '

                        alarm_id=b.getAttribute("ALARM_ID")
                        #print alarm_id
                        str1_gu+=alarm_id+'},\n'
                        arrycount[4]+=1;
                    if(2 == int(b.getAttribute("MODME_NUME"))):
                        #print modem_number
                        str2_gu+='    {'+fault_id+', '

                        pid=b.getAttribute("PID")
                        #print pid
                        str2_gu+=pid+', '

                        alarm_id=b.getAttribute("ALARM_ID")
                        #print alarm_id
                        str2_gu+=alarm_id+'},\n'
                        arrycount[5]+=1;

        if ('' != str_cdma):
                str_zhushi_s=' /*Item:%d'%arrycount[0]+' */'
                str_cdma_all += '\r\n    '+fault_id_name[1]+str_zhushi_s+'\r\n' + str_cdma
        if ('' != str1_cdma):
                str_zhushi_s=' /*Item:%d'%arrycount[1]+' */'
                str1_cdma_all += '\r\n    '+fault_id_name[1]+str_zhushi_s+'\r\n' + str1_cdma
        if ('' != str2_cdma):
                str_zhushi_s=' /*Item:%d'%arrycount[2]+' */'
                str2_cdma_all += '\r\n    '+fault_id_name[1]+str_zhushi_s+'\r\n' + str2_cdma

        if ('' != str_gu):
                str_zhushi_s=' /*Item:%d'%arrycount[3]+' */'
                str_gu_all += '\r\n    '+fault_id_name[1]+ str_zhushi_s +'\r\n' + str_gu
        if ('' != str1_gu):
                str_zhushi_s=' /*Item:%d'%arrycount[4]+' */'
                str1_gu_all += '\r\n    '+fault_id_name[1]+ str_zhushi_s+'\r\n' + str1_gu
        if ('' != str2_gu):
                str_zhushi_s=' /*Item:%d'%arrycount[5]+' */'
                str2_gu_all += '\r\n    '+fault_id_name[1]+ str_zhushi_s+'\r\n' + str2_gu

    print('*****************************************************************************')

    array=[]
    array.append(str_gu_all.replace('*/ /*','') +'\r\n' + str_cdma_all.replace('*/ /*','') +'\r\n#endif\r\n')
    array.append(str1_gu_all.replace('*/ /*','') +'\r\n' + str1_cdma_all.replace('*/ /*','') +'\r\n#endif\r\n')
    array.append(str2_gu_all.replace('*/ /*','') +'\r\n' + str2_cdma_all.replace('*/ /*','') +'\r\n#endif\r\n')
    return array

def get_faultid_from_includefile():
    '''
     函 数 名  : get_faultid_from_includefile
     功能描述  : 从errlog_cfg.h中获取faultid的枚举
     输入参数  : NA

     输出参数  : 获取到的faultid已经对应的注释
     返 回 值  : dict {‘FAULT_ID’：‘注释’}

     修改历史      :
      1.日    期   : 2016年2月18日
        作    者   : d00212987
        修改内容   : ERR LOG FAULT ID关联项目新增
    '''
    includefp=open(PATH_ERR_LOG_CFG_H, 'rb')

    try:
         faultidArry = includefp.read()

         #获取FAULT ID枚举定义起始位置
         faultid_start_index = faultidArry.find(PYTHON_SEARCH_FLAG_START)+len(PYTHON_SEARCH_FLAG_START)
         #print faultid_start_index
         faultid_end_index   = faultidArry.find(PYTHON_SEARCH_FLAG_END)
         #print faultid_end_index

         #获取FAULT ID code
         str2= faultidArry[faultid_start_index:faultid_end_index]

         #拆分FAULT ID及注释
         s = str2.splitlines();
         #print s
         fault_id_arr_dict = {}
         for x in s:
             if (0 == len(x.strip())):
                 continue
             else:
                 #获取FAULT ID枚举定义
                 faultid_end_idx=x.strip().find(" ")
                 fault_id=x.strip()[0:faultid_end_idx]
                 #print fault_id.strip()

                 #获取FAULT ID注释
                 faultid_start_idx=x.find("/*")
                 faultid_end_idx=x.find("*/")+len("*/")
                 zhushi=x[faultid_start_idx:faultid_end_idx]
                 #print zhushi.strip()

                 fault_id_arr_dict[fault_id]=zhushi.strip()

    finally:
         includefp.close( )

    return fault_id_arr_dict

def get_old_xml_hash_value():
    '''
     函 数 名  : write_file_cfg
     功能描述  : 从errlog_cfg.h中获取faultid的枚举
     输入参数  : NA

     输出参数  : 获取到的faultid已经对应的注释
     返 回 值  : dict {‘FAULT_ID’：‘注释’}

     修改历史      :
      1.日    期   : 2016年2月18日
        作    者   : d00212987
        修改内容   : ERR LOG FAULT ID关联项目新增
    '''
    includefp=open(PATH_ERR_LOG_CFG_C, 'rb')
    str_hash_valude=''
    try:
         faultidArry = includefp.read()

         #获取hash value起始位置
         faultid_start_index = faultidArry.find(PYTHON_SEARCH_HASH_FLAG_START)+len(PYTHON_SEARCH_HASH_FLAG_START)

         #获取hash value
         str_hash_valude=faultidArry[faultid_start_index:(faultid_start_index+32)]
    #读取errlog_cfg.c,获取替换标记
    finally:
         includefp.close()

    print("old xml hash value is:"+str_hash_valude)
    return str_hash_valude

def write_file_cfg_hash(str_old,str_new):
    '''
     函 数 名  : write_file_cfg_hash
     功能描述  : 从errlog_cfg.h中获取faultid的枚举
     输入参数  : NA

     输出参数  : 获取到的faultid已经对应的注释
     返 回 值  : dict {‘FAULT_ID’：‘注释’}

     修改历史      :
      1.日    期   : 2016年2月18日
        作    者   : d00212987
        修改内容   : ERR LOG FAULT ID关联项目新增
    '''
    includefp=open(PATH_ERR_LOG_CFG_C, 'rb')
    str_hash_valude=''
    faultidArry=''
    try:
        faultidArry = includefp.read()
        faultidArry = faultidArry.replace(str_old, str_new, 1);
    finally:
         includefp.close()

    includefp=open(PATH_ERR_LOG_CFG_C, 'wb')
    try:
        includefp.write(faultidArry)
    finally:
         includefp.close()

    print str_hash_valude
    return str_hash_valude


def write_file_cfg(write_count):
    '''
     函 数 名  : write_file_cfg
     功能描述  : 从errlog_cfg.h中获取faultid的枚举
     输入参数  : NA

     输出参数  : 获取到的faultid已经对应的注释
     返 回 值  : dict {‘FAULT_ID’：‘注释’}

     修改历史      :
      1.日    期   : 2016年2月18日
        作    者   : d00212987
        修改内容   : ERR LOG FAULT ID关联项目新增
    '''
    includefp=open(PATH_ERR_LOG_CFG_C, 'rb')
    str_errlog_cfg_c_pre=''
    str_errlog_cfg_c_end=''
    try:
         faultidArry = includefp.read()

         #获取FAULT ID枚举定义起始位置
         faultid_start_index = faultidArry.find(PYTHON_SEARCH_FLAG_START)+len(PYTHON_SEARCH_FLAG_START)
         #print faultid_start_index
         faultid_end_index   = faultidArry.find(PYTHON_SEARCH_FLAG_END)
         #print faultid_end_index

         #获取FAULT ID code
         str_errlog_cfg_c_pre=faultidArry[0:faultid_start_index]
         str_errlog_cfg_c_end=faultidArry[faultid_end_index:len(faultidArry)]
    #读取errlog_cfg.c,获取替换标记
    finally:
         includefp.close()

    #复制.c
    shutil.copyfile(PATH_ERR_LOG_CFG_C,'..\errlog_cfg_bak_'+time.strftime("%Y%m%d%H%M%S", time.localtime())+'.c')
    try:
    #重写.c
        includefp=open(PATH_ERR_LOG_CFG_C, 'wb')
        strcount = '\r\nint g_aulModem0ErrRept[][3]=\r\n\
{'+write_count[0]+'\r\n};\r\n\r\n'
        strcount += '#if (FEATURE_ON == FEATURE_MULTI_MODEM)\r\nint g_aulModem1ErrRept[][3]=\r\n\
{'+write_count[1]+'\r\n};\r\n\r\n'
        strcount += '#if(3 == MULTI_MODEM_NUMBER)\r\nint g_aulModem2ErrRept[][3]=\r\n\
{'+write_count[2]+'\r\n};\r\n#endif\r\n#endif\r\n'
        includefp.write(str_errlog_cfg_c_pre+strcount+str_errlog_cfg_c_end)
    finally:
        includefp.close()
    #return fault_id_arr_dict

def calc_xml_file_hash():
    '''
     函 数 名  : calc_xml_file_hash
     功能描述  : 更具xml内容生成对应的hash

     返 回 值  : hash字符串

     修改历史      :
      1.日    期   : 2016年2月18日
        作    者   : d00212987
        修改内容   : ERR LOG FAULT ID关联项目新增
    '''
    str_xml_value=''
    for path_xml in  PATH_ERR_LOG_XML_ARRY:
        xml_fp=open(path_xml, 'rb')
        try:
            str_xml_value += xml_fp.read()
        finally:
            xml_fp.close()
    #生成hash
    md5obj = hashlib.md5()
    md5obj.update(str_xml_value)
    hash = md5obj.hexdigest()
    print("new xml hash value is:"+hash)

    return hash

if __name__ == "__main__":
    '''
     函 数 名  : __main__
     功能描述  : 根据xml配置文件生成errlog_cfg.c

     修改历史      :
      1.日    期   : 2016年2月18日
        作    者   : d00212987
        修改内容   : ERR LOG FAULT ID关联项目新增
    '''
    #检查文件是否有改动
    #读取老的hash字符串
    str_old_hash = get_old_xml_hash_value()
    #生成新的hash
    str_new_hash = calc_xml_file_hash()

    print(time.strftime("%Y-%m-%d %H:%M:%S ", time.localtime()))
    errcode = 512
    if(str_old_hash!= str_new_hash):
        errcode = 512
        print("xml is changed,pls commit errlog_cfg.c and xml !")
    else:
        errcode = 0
        print("xml is not changed ")

    #通过include file 获取 Fault id
    dict_faultid=get_faultid_from_includefile();

    #生成errlog_cfg.c中fault关联列表全局数组
    glob_str = decode_xml_by_faultId(dict_faultid);

    #回写errlog_cfg.c文件
    write_file_cfg(glob_str);

    #更新xml标记
    write_file_cfg_hash(str_old_hash,str_new_hash);

    if errcode == 0:
         print("Everything is done!")
         os.system("pause")
    else :
        print ">> Error: " + str(errcode)
        sys.paus()
        # sys.exit(errcode) return the low 8 bits of errcode,so errcode is set to 1
        errcode = 1
    sys.exit(errcode)


