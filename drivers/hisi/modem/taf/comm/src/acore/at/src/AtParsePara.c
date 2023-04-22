/*
* Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
* foss@huawei.com
*
* If distributed as part of the Linux kernel, the following license terms
* apply:
*
* * This program is free software; you can redistribute it and/or modify
* * it under the terms of the GNU General Public License version 2 and 
* * only version 2 as published by the Free Software Foundation.
* *
* * This program is distributed in the hope that it will be useful,
* * but WITHOUT ANY WARRANTY; without even the implied warranty of
* * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* * GNU General Public License for more details.
* *
* * You should have received a copy of the GNU General Public License
* * along with this program; if not, write to the Free Software
* * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*
* Otherwise, the following license terms apply:
*
* * Redistribution and use in source and binary forms, with or without
* * modification, are permitted provided that the following conditions
* * are met:
* * 1) Redistributions of source code must retain the above copyright
* *    notice, this list of conditions and the following disclaimer.
* * 2) Redistributions in binary form must reproduce the above copyright
* *    notice, this list of conditions and the following disclaimer in the
* *    documentation and/or other materials provided with the distribution.
* * 3) Neither the name of Huawei nor the names of its contributors may 
* *    be used to endorse or promote products derived from this software 
* *    without specific prior written permission.
* 
* * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "AtCheckFunc.h"



#define    THIS_FILE_ID        PS_FILE_ID_AT_PARSEPARA_C


/* 参数匹配初始空状态表 */
AT_SUB_STATE_STRU AT_PARA_NONE_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_NONE_STATE，atCheckLeftBracket成功，则进入AT_PARA_LEFT_BRACKET_STATE */
    {    At_CheckLeftBracket    ,    AT_PARA_LEFT_BRACKET_STATE    },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配左括号状态表 */
AT_SUB_STATE_STRU AT_PARA_LEFT_BRACKET_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_LEFT_BRACKET_STATE，atCheckZero成功，则进入AT_PARA_ZERO_STATE */
    {At_CheckChar0,   AT_PARA_ZERO_STATE},

    /* 如果当前状态是AT_PARA_LEFT_BRACKET_STATE，atCheckDigit成功，则进入AT_PARA_NUM_STATE */
    {At_CheckDigit,  AT_PARA_NUM_STATE},

    /* 如果当前状态是AT_PARA_LEFT_BRACKET_STATE，at_NoQuotLetter成功，则进入AT_PARA_NO_QUOT_LETTER_STATE */
    {atNoQuotLetter, AT_PARA_NO_QUOT_LETTER_STATE},

    /* 如果当前状态是AT_PARA_LEFT_BRACKET_STATE，atCheckLetter成功，则进入AT_PARA_LETTER_STATE */
    {At_CheckLetter, AT_PARA_LETTER_STATE},

    /* 如果当前状态是AT_PARA_LEFT_BRACKET_STATE，atCheckQuot成功，则进入AT_PARA_LEFT_QUOT_STATE */
    {At_CheckQuot,   AT_PARA_LEFT_QUOT_STATE},

    /* 子状态表结束 */
    {NULL,           AT_BUTT_STATE},
};

/* 参数匹配参数状态表 */
AT_SUB_STATE_STRU AT_PARA_NUM_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_NUM_STATE，atCheckDigit成功，则进入AT_PARA_NUM_STATE */
    {   At_CheckDigit   ,   AT_PARA_NUM_STATE    },

    /* 如果当前状态是AT_PARA_NUM_STATE，atCheckColon成功，则进入AT_PARA_NUM_COLON_STATE */
    {   atCheckComma ,   AT_PARA_NUM_COLON_STATE  },

    /* 如果当前状态是AT_PARA_NUM_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {   At_CheckRightBracket  , AT_PARA_RIGHT_BRACKET_STATE   },

    /* 如果当前状态是AT_PARA_NUM_STATE，atCheckCharSub成功，则进入AT_PARA_SUB_STATE */
    {   At_CheckCharSub   ,   AT_PARA_SUB_STATE    },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配范围状态表 */
AT_SUB_STATE_STRU AT_PARA_SUB_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_SUB_STATE，atCheckZero成功，则进入AT_PARA_ZERO_SUB_STATE */
    {At_CheckChar0,  AT_PARA_ZERO_SUB_STATE},

    /* 如果当前状态是AT_PARA_SUB_STATE，atCheckDigit成功，则进入AT_PARA_NUM_SUB_STATE */
    {At_CheckDigit, AT_PARA_NUM_SUB_STATE},

    /* 子状态表结束 */
    {NULL,          AT_BUTT_STATE},
};

/* 参数匹配范围数字状态表 */
AT_SUB_STATE_STRU AT_PARA_NUM_SUB_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckDigit成功，则进入AT_PARA_NUM_SUB_STATE */
    {   At_CheckDigit   ,   AT_PARA_NUM_SUB_STATE    },

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckColon成功，则进入AT_PARA_NUM_SUB_COLON_STATE */
    {   atCheckComma ,   AT_PARA_NUM_SUB_COLON_STATE  },

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {   At_CheckRightBracket  ,   AT_PARA_RIGHT_BRACKET_STATE   },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配范围数字逗号状态表 */
AT_SUB_STATE_STRU AT_PARA_NUM_SUB_COLON_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_NUM_SUB_COLON_STATE，atCheckZero成功，则进入AT_PARA_ZERO_STATE */
    {At_CheckChar0,  AT_PARA_ZERO_STATE},

    /* 如果当前状态是AT_PARA_NUM_SUB_COLON_STATE，atCheckDigit成功，则进入AT_PARA_NUM_STATE */
    {At_CheckDigit, AT_PARA_NUM_STATE},

    /* 子状态表结束 */
    {NULL,          AT_BUTT_STATE},
};

/* 参数匹配右括号状态表 */
AT_SUB_STATE_STRU AT_PARA_RIGHT_BRACKET_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_RIGHT_BRACKET_STATE，atCheckColon成功，则进入AT_PARA_COLON_STATE */
    {   atCheckComma ,   AT_PARA_COLON_STATE  },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配逗号状态表 */
AT_SUB_STATE_STRU AT_PARA_COLON_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_COLON_STATE，atCheckLeftBracket成功，则进入AT_PARA_LEFT_BRACKET_STATE */
    {   At_CheckLeftBracket  ,   AT_PARA_LEFT_BRACKET_STATE   },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配左引号状态表 */
AT_SUB_STATE_STRU AT_PARA_LEFT_QUOT_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_LEFT_QUOT_STATE，atCheckNoQuot成功，则进入AT_PARA_LEFT_QUOT_STATE */
    {   At_CheckNoQuot   ,   AT_PARA_LEFT_QUOT_STATE },

    /* 如果当前状态是AT_PARA_LEFT_QUOT_STATE，atCheckQuot成功，则进入AT_PARA_RIGHT_QUOT_STATE */
    {   At_CheckQuot ,    AT_PARA_RIGHT_QUOT_STATE },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配右引号状态表 */
AT_SUB_STATE_STRU AT_PARA_RIGHT_QUOT_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_RIGHT_QUOT_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {   At_CheckRightBracket  ,   AT_PARA_RIGHT_BRACKET_STATE   },

    /* 如果当前状态是AT_PARA_RIGHT_QUOT_STATE，atCheckColon成功，则进入AT_PARA_QUOT_COLON_STATE */
    {   atCheckComma ,   AT_PARA_QUOT_COLON_STATE },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配字母状态表 */
AT_SUB_STATE_STRU AT_PARA_LETTER_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_LETTER_STATE，atCheckLetter成功，则进入AT_PARA_LETTER_STATE */
    {   At_CheckLetter    ,   AT_PARA_LETTER_STATE },

    /* 如果当前状态是AT_PARA_LETTER_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {   At_CheckRightBracket  ,   AT_PARA_RIGHT_BRACKET_STATE   },

    /* 如果当前状态是AT_PARA_LETTER_STATE，atCheckCharSub成功，则进入AT_PARA_LETTER_STATE */
    {   At_CheckCharSub   ,   AT_PARA_LETTER_STATE },

    /* 如果当前状态是AT_PARA_LETTER_STATE，atCheckColon成功，则进入AT_PARA_LETTER_STATE */
    {   atCheckColon     ,   AT_PARA_LETTER_STATE },

    /* 如果当前状态是AT_PARA_LETTER_STATE，atCheckblank成功，则进入AT_PARA_LETTER_STATE */
    {   atCheckblank     ,   AT_PARA_LETTER_STATE },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};

/* 参数匹配数字逗号状态表 */
AT_SUB_STATE_STRU AT_PARA_NUM_COLON_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_NUM_COLON_STATE，atCheckZero成功，则进入AT_PARA_ZERO_STATE */
    {At_CheckChar0,  AT_PARA_ZERO_STATE},

    /* 如果当前状态是AT_PARA_NUM_COLON_STATE，atCheckDigit成功，则进入AT_PARA_NUM_STATE */
    {At_CheckDigit, AT_PARA_NUM_STATE},

    /* 子状态表结束 */
    {NULL,          AT_BUTT_STATE},
};

/* 参数匹配引号逗号状态表 */
AT_SUB_STATE_STRU AT_PARA_QUOT_COLON_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_QUOT_COLON_STATE，atCheckQuot成功，则进入AT_PARA_LEFT_QUOT_STATE */
    {   At_CheckQuot  ,   AT_PARA_LEFT_QUOT_STATE },

    /* 子状态表结束 */
    {    NULL    ,    AT_BUTT_STATE    },
};


/* 支持八进制、十六进制参数 BEGIN */

/* 参数匹配数字0状态表 */
AT_SUB_STATE_STRU AT_PARA_ZERO_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_ZERO_STATE，at_CheckHex成功，则进入AT_PARA_HEX_STATE */
    {atCheckHex,          AT_PARA_HEX_STATE},

    /* 如果当前状态是AT_PARA_ZERO_STATE，atCheckDigit成功，则进入AT_PARA_NUM_STATE */
    {At_CheckDigit,        AT_PARA_NUM_STATE},

    /* 如果当前状态是AT_PARA_ZERO_STATE，atCheckColon成功，则进入AT_PARA_NUM_COLON_STATE */
    {atCheckComma,        AT_PARA_NUM_COLON_STATE},

    /* 如果当前状态是AT_PARA_ZERO_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {At_CheckRightBracket, AT_PARA_RIGHT_BRACKET_STATE},

    /* 如果当前状态是AT_PARA_ZERO_STATE，atCheckCharSub成功，则进入AT_PARA_SUB_STATE */
    {At_CheckCharSub,      AT_PARA_SUB_STATE},

    /* 子状态表结束 */
    {NULL,                 AT_BUTT_STATE},
};

/* 参数匹配十六进制状态表 */
AT_SUB_STATE_STRU AT_PARA_HEX_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_HEX_STATE，at_CheckHex成功，则进入AT_PARA_HEX_NUM_STATE */
    {atCheckHexNum, AT_PARA_HEX_NUM_STATE},

    /* 子状态表结束 */
    {NULL,           AT_BUTT_STATE},
};

/* 参数匹配十六进制数字状态表 */
AT_SUB_STATE_STRU AT_PARA_HEX_NUM_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_ZERO_STATE，at_CheckHex成功，则进入AT_PARA_HEX_STATE */
    {atCheckHexNum,       AT_PARA_HEX_NUM_STATE},

    /* 如果当前状态是AT_PARA_ZERO_STATE，atCheckColon成功，则进入AT_PARA_NUM_COLON_STATE */
    {atCheckComma,        AT_PARA_NUM_COLON_STATE},

    /* 如果当前状态是AT_PARA_ZERO_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {At_CheckRightBracket, AT_PARA_RIGHT_BRACKET_STATE},

    /* 如果当前状态是AT_PARA_ZERO_STATE，atCheckCharSub成功，则进入AT_PARA_SUB_STATE */
    {At_CheckCharSub,      AT_PARA_SUB_STATE},

    /* 子状态表结束 */
    {NULL,                 AT_BUTT_STATE},
};

/* 参数匹配范围数字0状态表 */
AT_SUB_STATE_STRU AT_PARA_ZERO_SUB_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_ZERO_SUB_STATE，at_CheckHex成功，则进入AT_PARA_HEX_SUB_STATE */
    {atCheckHex,          AT_PARA_HEX_SUB_STATE},

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckDigit成功，则进入AT_PARA_NUM_SUB_STATE */
    {At_CheckDigit,        AT_PARA_NUM_SUB_STATE    },

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckColon成功，则进入AT_PARA_NUM_SUB_COLON_STATE */
    {atCheckComma,        AT_PARA_NUM_SUB_COLON_STATE  },

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {At_CheckRightBracket, AT_PARA_RIGHT_BRACKET_STATE   },

    /* 子状态表结束 */
    {NULL,                 AT_BUTT_STATE    },
};

/* 参数匹配范围十六进制状态表 */
AT_SUB_STATE_STRU AT_PARA_HEX_SUB_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_HEX_SUB_STATE，at_CheckHex成功，则进入AT_PARA_HEX_NUM_SUB_STATE */
    {atCheckHexNum, AT_PARA_HEX_NUM_SUB_STATE},

    /* 子状态表结束 */
    {NULL,           AT_BUTT_STATE},
};

/* 参数匹配范围十六进制数字状态表 */
AT_SUB_STATE_STRU AT_PARA_HEX_NUM_SUB_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_HEX_NUM_SUB_STATE，at_CheckHexNum成功，则进入AT_PARA_HEX_NUM_SUB_STATE */
    {atCheckHexNum,       AT_PARA_HEX_NUM_SUB_STATE},

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckColon成功，则进入AT_PARA_NUM_SUB_COLON_STATE */
    {atCheckComma,        AT_PARA_NUM_SUB_COLON_STATE  },

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {At_CheckRightBracket, AT_PARA_RIGHT_BRACKET_STATE   },

    /* 子状态表结束 */
    {NULL,                 AT_BUTT_STATE    },
};

/* 支持八进制、十六进制参数 END */

/* 支持无双引号包括字符串 BEGIN */

/* 参数匹配无双引号包括字母状态表 */
AT_SUB_STATE_STRU AT_PARA_NO_QUOT_LETTER_STATE_TAB[]=
{
    /* 如果当前状态是AT_PARA_NO_QUOT_LETTER_STATE，atCheckLetter成功，则进入AT_PARA_NO_QUOT_LETTER_STATE */
    {At_CheckLetter,       AT_PARA_NO_QUOT_LETTER_STATE },

    /* 如果当前状态是AT_PARA_NO_QUOT_LETTER_STATE，atCheckRightBracket成功，则进入AT_PARA_RIGHT_BRACKET_STATE */
    {At_CheckRightBracket, AT_PARA_RIGHT_BRACKET_STATE   },

    /* 子状态表结束 */
    {NULL,                 AT_BUTT_STATE    },
};

/* 支持无双引号包括字符串 END */

/* 参数匹配主状态表 */
AT_MAIN_STATE_STRU AT_MAIN_PARA_STATE_TAB[] =
{
    /* 如果当前状态是AT_NONE_STATE，则进入AT_PARA_NONE_STATE_TAB子状态表 */
    {   AT_NONE_STATE   ,   AT_PARA_NONE_STATE_TAB  },

    /* 如果当前状态是AT_PARA_LEFT_BRACKET_STATE，则进入AT_PARA_LEFT_BRACKET_STATE_TAB子状态表 */
    {   AT_PARA_LEFT_BRACKET_STATE  , AT_PARA_LEFT_BRACKET_STATE_TAB    },

    /* 如果当前状态是AT_PARA_RIGHT_BRACKET_STATE，则进入AT_PARA_RIGHT_BRACKET_STATE_TAB子状态表 */
    {   AT_PARA_RIGHT_BRACKET_STATE  , AT_PARA_RIGHT_BRACKET_STATE_TAB    },

    /* 如果当前状态是AT_PARA_LETTER_STATE，则进入AT_PARA_LETTER_STATE_TAB子状态表 */
    {   AT_PARA_LETTER_STATE    ,   AT_PARA_LETTER_STATE_TAB    },

    /* 如果当前状态是AT_PARA_NUM_STATE，则进入AT_PARA_NUM_STATE_TAB子状态表 */
    {   AT_PARA_NUM_STATE    ,   AT_PARA_NUM_STATE_TAB  },

    /* 如果当前状态是AT_PARA_NUM_COLON_STATE，则进入AT_PARA_NUM_COLON_STATE_TAB子状态表 */
    {   AT_PARA_NUM_COLON_STATE ,    AT_PARA_NUM_COLON_STATE_TAB  },

    /* 如果当前状态是AT_PARA_SUB_STATE，则进入AT_PARA_SUB_STATE_TAB子状态表 */
    {   AT_PARA_SUB_STATE    ,   AT_PARA_SUB_STATE_TAB  },

    /* 如果当前状态是AT_PARA_NUM_SUB_STATE，则进入AT_PARA_NUM_SUB_STATE_TAB子状态表 */
    {   AT_PARA_NUM_SUB_STATE   ,  AT_PARA_NUM_SUB_STATE_TAB  },

    /* 如果当前状态是AT_PARA_NUM_SUB_COLON_STATE，则进入AT_PARA_NUM_SUB_COLON_STATE_TAB子状态表 */
    {   AT_PARA_NUM_SUB_COLON_STATE  ,   AT_PARA_NUM_SUB_COLON_STATE_TAB  },

    /* 如果当前状态是AT_PARA_COLON_STATE，则进入AT_PARA_COLON_STATE_TAB子状态表 */
    {   AT_PARA_COLON_STATE ,    AT_PARA_COLON_STATE_TAB  },

    /* 如果当前状态是AT_PARA_LEFT_QUOT_STATE，则进入AT_PARA_LEFT_QUOT_STATE_TAB子状态表 */
    {   AT_PARA_LEFT_QUOT_STATE ,   AT_PARA_LEFT_QUOT_STATE_TAB    },

    /* 如果当前状态是AT_PARA_RIGHT_QUOT_STATE，则进入AT_PARA_RIGHT_QUOT_STATE_TAB子状态表 */
    {   AT_PARA_RIGHT_QUOT_STATE    ,   AT_PARA_RIGHT_QUOT_STATE_TAB    },

    /* 如果当前状态是AT_PARA_QUOT_COLON_STATE，则进入AT_PARA_QUOT_COLON_STATE_TAB子状态表 */
    {   AT_PARA_QUOT_COLON_STATE    ,   AT_PARA_QUOT_COLON_STATE_TAB    },

    /* 如果当前状态是AT_PARA_ZERO_STATE，则进入AT_PARA_ZERO_STATE_TAB子状态表 */
    {AT_PARA_ZERO_STATE,           AT_PARA_ZERO_STATE_TAB},

    /* 如果当前状态是AT_PARA_ZERO_SUB_STATE，则进入AT_PARA_ZERO_SUB_STATE_TAB子状态表 */
    {AT_PARA_ZERO_SUB_STATE,       AT_PARA_ZERO_SUB_STATE_TAB},

    /* 如果当前状态是AT_PARA_HEX_STATE，则进入AT_PARA_HEX_STATE_TAB子状态表 */
    {AT_PARA_HEX_STATE,            AT_PARA_HEX_STATE_TAB},

    /* 如果当前状态是AT_PARA_HEX_NUM_STATE，则进入AT_PARA_HEX_NUM_STATE_TAB子状态表 */
    {AT_PARA_HEX_NUM_STATE,        AT_PARA_HEX_NUM_STATE_TAB},

    /* 如果当前状态是AT_PARA_HEX_SUB_STATE，则进入AT_PARA_HEX_SUB_STATE_TAB子状态表 */
    {AT_PARA_HEX_SUB_STATE,        AT_PARA_HEX_SUB_STATE_TAB},

    /* 如果当前状态是AT_PARA_HEX_NUM_SUB_STATE，则进入AT_PARA_HEX_NUM_SUB_STATE_TAB子状态表 */
    {AT_PARA_HEX_NUM_SUB_STATE,    AT_PARA_HEX_NUM_SUB_STATE_TAB},

    /* 如果当前状态是AT_PARA_NO_QUOT_LETTER_STATE，则进入AT_PARA_NO_QUOT_LETTER_STATE_TAB子状态表 */
    {AT_PARA_NO_QUOT_LETTER_STATE, AT_PARA_NO_QUOT_LETTER_STATE_TAB},

    /* 主状态表结束 */
    {   AT_BUTT_STATE   ,   NULL  },
};

pAtChkFuncType      pgAtCheckFunc   = NULL;



VOS_UINT32 atCmparePara(VOS_VOID)
{
    /*检查输入参数*/
    if(NULL != pgAtCheckFunc)
    {
        /*检查参数列表对应的参数是否存在*/
        if(0 < gastAtParaList[g_stATParseCmd.ucParaCheckIndex].usParaLen)
        {
            /*如果参数检查错误*/
            if(AT_SUCCESS != pgAtCheckFunc(&gastAtParaList[g_stATParseCmd.ucParaCheckIndex]))
            {
                return AT_FAILURE;           /*  返回错误*/
            }
        }

        /*记录已检查参数的全局变量加1*/
        g_stATParseCmd.ucParaCheckIndex++;

        /*清空，以备比较下一个参数,否则，对比参数继续增加*/
        if(0 != g_stATParseCmd.ucParaNumRangeIndex)
        {
            g_stATParseCmd.ucParaNumRangeIndex = 0;
            TAF_MEM_SET_S(g_stATParseCmd.astParaNumRange,sizeof(g_stATParseCmd.astParaNumRange), 0x00, sizeof(g_stATParseCmd.astParaNumRange));
        }

        if(0 != g_stATParseCmd.ucParaStrRangeIndex)
        {
            g_stATParseCmd.ucParaStrRangeIndex = 0;
            TAF_MEM_SET_S(g_stATParseCmd.auStrRange, sizeof(g_stATParseCmd.auStrRange), 0x00, sizeof(g_stATParseCmd.auStrRange));
        }

        pgAtCheckFunc = NULL;

        return AT_SUCCESS;                   /*  返回正确*/
    }
    else
    {
        return AT_FAILURE;                   /*  返回错误*/
    }
}



VOS_UINT32 atParsePara( VOS_UINT8 * pData, VOS_UINT16 usLen)
{
    VOS_UINT16 usLength = 0;                        /*  记录当前已经处理的字符个数*/
    VOS_UINT8 *pucCurrPtr = pData;                  /*  指向当前正在处理的字符*/
    VOS_UINT8 *pucCopyPtr = pData;                  /*  拷贝内容的起始指针 */
    AT_STATE_TYPE_ENUM curr_state = AT_NONE_STATE;  /*  设置初始状态 */
    AT_STATE_TYPE_ENUM new_state = AT_NONE_STATE;   /*  设置初始状态 */

    /* 依次分析字符串中的每个字符*/
    while( (usLength++ < usLen) && (g_stATParseCmd.ucParaNumRangeIndex < AT_MAX_PARA_NUMBER) && (g_stATParseCmd.ucParaStrRangeIndex < AT_MAX_PARA_NUMBER))        /* 依次比较每个字符 */
    {
        curr_state = new_state;                 /*  当前状态设置为新状态*/

        /* 根据当前处理的字符和当前状态查表得到新状态 */
        new_state = atFindNextMainState(AT_MAIN_PARA_STATE_TAB,*pucCurrPtr,curr_state);

        switch(new_state)                       /* 状态处理*/
        {
        case AT_PARA_NUM_STATE:                 /* 参数状态*/
        case AT_PARA_LETTER_STATE:              /* 字母状态*/
        case AT_PARA_LEFT_QUOT_STATE:           /* 参数左引号状态*/
        case AT_PARA_NUM_SUB_STATE:             /* 范围数字状态*/
        case AT_PARA_ZERO_STATE:                /* 数字0状态 */
        case AT_PARA_ZERO_SUB_STATE:            /* 范围数字0状态 */
        case AT_PARA_NO_QUOT_LETTER_STATE:      /* 无双引号包括字母状态 */
            if(curr_state != new_state)         /* 新状态部分不等于当前状态*/
            {
                pucCopyPtr = pucCurrPtr;            /*  准备接收参数*/
            }
            break;

        case AT_PARA_HEX_STATE:                 /* 十六进制状态 */
            /* continue */
        case AT_PARA_HEX_NUM_STATE:             /* 十六进制数字状态 */
            /* continue */
        case AT_PARA_HEX_SUB_STATE:             /* 匹配范围十六进制状态 */
            /* continue */
        case AT_PARA_HEX_NUM_SUB_STATE:         /* 匹配范围十六进制数字状态 */
            break;

        case AT_PARA_QUOT_COLON_STATE:          /* 引号逗号状态 */

            /* 拷贝到字符参数脚本列表 */
            /* 字符参数索引加1*/

            if ((VOS_UINT32)(pucCurrPtr - pucCopyPtr) < sizeof(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex]))
            {
                atRangeCopy(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex++],pucCopyPtr,pucCurrPtr);
            }
            else
            {
                return AT_FAILURE;                  /* 返回错误 */
            }
            break;

        case AT_PARA_NUM_SUB_COLON_STATE:

            /* 转成数字拷贝到数字参数脚本列表的上界 */
            /* 数字参数索引加1*/
            g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex++].ulBig = atRangeToU32(pucCopyPtr,pucCurrPtr);

            break;

        case AT_PARA_NUM_COLON_STATE:

            /* 转成数字拷贝到数字参数脚本列表的下界 */
            g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex].ulSmall = atRangeToU32(pucCopyPtr,pucCurrPtr);

            /* 上界等于下界*/
            g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex].ulBig = g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex].ulSmall;

            /* 数字参数索引加1*/
            g_stATParseCmd.ucParaNumRangeIndex++;

            break;

        case AT_PARA_SUB_STATE:

            /* 转成数字拷贝到数字参数脚本列表的下界 */
            g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex].ulSmall = atRangeToU32(pucCopyPtr,pucCurrPtr);

            break;

        case AT_PARA_RIGHT_BRACKET_STATE:             /* 参数匹配右括号状态 */
            switch (curr_state)
            {
            case AT_PARA_NUM_SUB_STATE:          /* 参数匹配范围数字状态 */
                /* continue */
            case AT_PARA_ZERO_SUB_STATE:
                /* continue */
            case AT_PARA_HEX_NUM_SUB_STATE:

                /* 转成数字拷贝到数字参数脚本列表的上界 */
                /* 数字参数索引加1*/
                g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex++].ulBig = atRangeToU32(pucCopyPtr,pucCurrPtr);

                pgAtCheckFunc = atCheckNumPara;    /* 设置参数比较函数 */

                break;

            case AT_PARA_NUM_STATE:
                /* continue */
            case AT_PARA_ZERO_STATE:
                /* continue */
            case AT_PARA_HEX_NUM_STATE:

                /* 转成数字拷贝到数字参数脚本列表的下界 */
                g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex].ulSmall = atRangeToU32(pucCopyPtr,pucCurrPtr);

                /* 上界等于下界*/
                g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex].ulBig = g_stATParseCmd.astParaNumRange[g_stATParseCmd.ucParaNumRangeIndex].ulSmall;

                /* 数字参数索引加1*/
                g_stATParseCmd.ucParaNumRangeIndex++;

                pgAtCheckFunc = atCheckNumPara;    /* 设置参数比较函数 */

                break;

            case AT_PARA_LETTER_STATE:

                /* 拷贝到字符参数脚本列表 */
                if ((VOS_UINT32)(pucCurrPtr - pucCopyPtr) < sizeof(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex]))
                {
                    atRangeCopy(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex],pucCopyPtr,pucCurrPtr);
                }
                else
                {
                    return AT_FAILURE;                  /* 返回错误 */
                }

                /* 设置参数比较函数 */
                pgAtCheckFunc = At_CheckStringPara;

                break;

            case AT_PARA_NO_QUOT_LETTER_STATE:      /* 无双引号包括字母状态 */

                /* 拷贝到字符参数脚本列表 */
                if ((VOS_UINT32)(pucCurrPtr - pucCopyPtr) < sizeof(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex]))
                {
                    atRangeCopy(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex],pucCopyPtr,pucCurrPtr);
                }
                else
                {
                    return AT_FAILURE;                  /* 返回错误 */
                }

                /* 设置参数比较函数 */
                pgAtCheckFunc = atCheckNoQuotStringPara;

                break;

            default:    /* AT_PARA_RIGHT_QUOT_STATE */

                /* 拷贝到字符参数脚本列表 */
                if ((VOS_UINT32)(pucCurrPtr - pucCopyPtr) < sizeof(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex]))
                {
                    atRangeCopy(g_stATParseCmd.auStrRange[g_stATParseCmd.ucParaStrRangeIndex++],pucCopyPtr,pucCurrPtr);
                }
                else
                {
                    return AT_FAILURE;                  /* 返回错误 */
                }

                /* 设置参数比较函数 */
                pgAtCheckFunc = atCheckCharPara;

                break;
            }

            if(AT_FAILURE == atCmparePara())       /* 参数比较 */
            {
                return AT_FAILURE;                   /* 返回错误 */
            }
            break;

        case AT_BUTT_STATE:                     /* 无效状态 */
            return AT_FAILURE;                  /* 返回错误 */

        default:
            break;
        }
        pucCurrPtr++;                               /*  继续分析下一个字符*/
    }

    if(AT_NONE_STATE == new_state)                  /* 初始状态 */
    {
        return AT_FAILURE;                          /* 返回错误 */
    }

    return AT_SUCCESS;                          /*  返回正确*/
}




