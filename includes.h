/****************************************Copyright (c)****************************************************
**
**                                       D.H. InfoTech
**
**--------------File Info---------------------------------------------------------------------------------
** File name:                  includes.h
** Latest Version:             V1.0.0
** Latest modified Date:       2015/06/29
** Modified by:                
** Descriptions:               全局包含文件，为了保持一致性，每个cpp文件必须包含此文件，即使不需要此文件中的内容
**
**--------------------------------------------------------------------------------------------------------
** Created by:                 Chen Honghao
** Created date:               2015/06/29
** Descriptions:               
** 
*********************************************************************************************************/
#ifndef __INCLUDES_H
#define __INCLUDES_H

/*
 * @brief 包含公共头文件
 */
#if defined(_DEBUG) || 1
#include <QDebug>
#endif

/**
 *  @brief 此处用于处理VS2010编译器的utf-8编码问题，
 *         必须保证每一个包含中文字符的源码文件均使用此pragma
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1600)  
# pragma execution_character_set("utf-8")  
#endif

/**
 *  @macro Q_ENABLE_COPY
 *  @brief 表示一个QObject的子类可以被拷贝（对应Q_DISABLE_COPY系统宏），仅用于支持元对象系统的对象动态创建
 *  @note  本宏仅需要在基类的头文件中使用一次，推荐使用在Q_OBJECT宏的下方一行，或者其他类似的显著位置
 */
#define Q_ENABLE_COPY(_type)                                                                            \
    public: _type(const _type& other) {                                                                 \
        Q_ASSERT_X(false, #_type" Meta Copy-Constructor", #_type" Meta-Objects can NOT be copied.");    \
    }                                                                                                   



#endif                                                                  /* __INCLUDES_H                 */
/*********************************************************************************************************
** End of file
*********************************************************************************************************/
