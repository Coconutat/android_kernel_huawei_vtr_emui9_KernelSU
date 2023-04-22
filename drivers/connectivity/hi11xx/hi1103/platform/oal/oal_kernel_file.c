


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_kernel_file.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID  OAM_FILE_ID_OAL_KERNEL_FILE_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

/*****************************************************************************
  3 函数实现
*****************************************************************************/
/*
 * 内核文件打开函数
 * 参数为文件路径
 * 操作file类型结构变量
 *
 */

oal_file* oal_kernel_file_open_etc(oal_uint8 *file_path,oal_int32 ul_attribute)
{
    oal_file* pst_file;
    pst_file = filp_open((oal_int8 *)file_path,ul_attribute,0777);
    if(IS_ERR(pst_file))
    {
        return 0;
    }

    return pst_file;
}


loff_t oal_kernel_file_size_etc(oal_file *pst_file)
{
    struct inode *pst_inode;
    loff_t        ul_fsize = 0;

    if(OAL_PTR_NULL != ((pst_file->f_path).dentry))
    {
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,1,0))
        pst_inode = file_inode(pst_file);
#else
        pst_inode = ((pst_file->f_path).dentry)->d_inode;
#endif
        ul_fsize = pst_inode->i_size;
    }

     return ul_fsize;
}


ssize_t oal_kernel_file_read_etc(oal_file *pst_file, oal_uint8 *pst_buff, loff_t ul_fsize)
{
    loff_t      *pos;

    pos = &(pst_file->f_pos);

    return vfs_read(pst_file,(oal_int8 *)pst_buff,ul_fsize,pos);
}



oal_int oal_kernel_file_write_etc(oal_file *pst_file,oal_uint8 *pst_buf,loff_t fsize)
{
    loff_t *pst_pos = &(pst_file->f_pos);

    vfs_write(pst_file,(oal_int8 *)pst_buf,fsize,pst_pos);

    return OAL_SUCC;
}


oal_int oal_kernel_file_print_etc(oal_file *pst_file,const oal_int8 *pc_fmt,...)
{

    oal_int8                    auc_str_buf[OAL_PRINT_FORMAT_LENGTH];   /* 保存要打印的字符串 buffer used during I/O */
    OAL_VA_LIST                 pc_args;


    if(OAL_PTR_NULL == pst_file||OAL_PTR_NULL == pc_fmt)
    {
        return OAL_FAIL;
    }

    OAL_VA_START(pc_args, pc_fmt);
    OAL_VSPRINTF(auc_str_buf, OAL_PRINT_FORMAT_LENGTH, pc_fmt, pc_args);
    OAL_VA_END(pc_args);

    return oal_kernel_file_write_etc(pst_file,(oal_uint8 *)auc_str_buf,OAL_STRLEN(auc_str_buf));

}
#endif

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT

OAL_STATIC oal_kobject* g_conn_syfs_root_object = NULL;
OAL_STATIC oal_kobject* g_conn_syfs_root_boot_object = NULL;

oal_kobject* oal_get_sysfs_root_object_etc(oal_void)
{
    if(NULL != g_conn_syfs_root_object)
        return g_conn_syfs_root_object;
    g_conn_syfs_root_object = kobject_create_and_add("hisys", OAL_PTR_NULL);
    return g_conn_syfs_root_object;
}

oal_kobject* oal_get_sysfs_root_boot_object_etc(oal_void)
{
    OAL_STATIC oal_kobject* root_boot_object = NULL;
    if(g_conn_syfs_root_boot_object)
        return g_conn_syfs_root_boot_object;
    root_boot_object = oal_get_sysfs_root_object_etc();
    if(NULL == root_boot_object)
        return NULL;
    g_conn_syfs_root_boot_object = kobject_create_and_add("boot", root_boot_object);
    return g_conn_syfs_root_boot_object;
}

oal_kobject* oal_conn_sysfs_root_obj_init_etc(oal_void)
{
    return oal_get_sysfs_root_object_etc();
}

oal_void oal_conn_sysfs_root_obj_exit_etc(oal_void)
{
    if(NULL != g_conn_syfs_root_object)
    {
        kobject_del(g_conn_syfs_root_object);
        g_conn_syfs_root_object = NULL;
    }
}

oal_void oal_conn_sysfs_root_boot_obj_exit_etc(oal_void)
{
    if(NULL != g_conn_syfs_root_boot_object)
    {
        kobject_del(g_conn_syfs_root_boot_object);
        g_conn_syfs_root_boot_object = NULL;
    }
}
oal_module_symbol(oal_get_sysfs_root_object_etc);
oal_module_symbol(oal_get_sysfs_root_boot_object_etc);
oal_module_symbol(oal_conn_sysfs_root_obj_exit_etc);
oal_module_symbol(oal_conn_sysfs_root_boot_obj_exit_etc);
#else
oal_kobject* oal_get_sysfs_root_object_etc(oal_void)
{
    return NULL;
}

oal_kobject* oal_conn_sysfs_root_obj_init_etc(oal_void)
{
    return NULL;
}

oal_void oal_conn_sysfs_root_obj_exit_etc(oal_void)
{
    return;
}

oal_void oal_conn_sysfs_root_boot_obj_exit_etc(oal_void)
{
    return;
}
#endif


/*lint -e19*/
oal_module_symbol(oal_kernel_file_open_etc);
oal_module_symbol(oal_kernel_file_size_etc);
oal_module_symbol(oal_kernel_file_read_etc);
oal_module_symbol(oal_kernel_file_write_etc);
oal_module_symbol(oal_kernel_file_print_etc);
oal_module_license("GPL");

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

