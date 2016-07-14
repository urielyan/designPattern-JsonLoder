/****************************************Copyright (c)****************************************************
**
**                                       D.H. InfoTech
**
**--------------File Info---------------------------------------------------------------------------------
** File name:                  JsonLoader_p.h
** Latest Version:             
** Latest modified Date:       2016/05/26
** Modified by:                
** Descriptions:               
**
**--------------------------------------------------------------------------------------------------------
** Created by:                 Chen Honghao
** Created date:               2016/05/26
** Descriptions:               JsonLoader库的私有头文件，包含部分公共宏定义，例如配置信息等
** 
*********************************************************************************************************/
#ifndef __JSONLOADER_P_H__
#define __JSONLOADER_P_H__

/*
 *  @macro JSON_LOADER_VERSION
 *  @brief JsonLoader的版本号，请与头文件的Copyright保持一致
 */
#define JSON_LOADER_VERSION                 "V1.3.0"

/**
 *  @macro JSON_LOADER_DEBUGGING_LEVEL
 *  @brief 调试等级：是否使能自动报告错误、状态输出等调试功能，如果禁用该功能，请手动绑定error信号以获取错误信息
 */
#define JSON_LOADER_DEBUGGING_LEVEL         1
/**
 *  @macro ENABLE_TS_FILE
 *  @brief 是否使能TS文件相关操作，例如执行翻译、创建翻译文件等
 */
#define ENABLE_TS_FILE                      1
/**
 *  @macro ENABLE_MEM_POOL
 *  @brief 是否使能内存池，在某些平台下使能内存池可以提高解析器的运行效率
 */
#define ENABLE_MEM_POOL                     0
/**
 *  @macro ENABLE_LEGACY_KEYWORDS
 *  @brief 是否使能旧版本的关键字（例如metaType在新版本中已经调整为.type），如果不需要兼容旧的json文件请关闭以提高效率
 */
#define ENABLE_LEGACY_KEYWORDS              1
/**
 *  @macro ENABLE_CUSTOM_OBJECT_FACTORY
 *  @brief 是否使能自定义的对象工厂，其优势在于不需要用户类提供拷贝构造函数，且效率更高
 *  @note  需要注意旧版本的关键字与新版本的自定义对象工厂不兼容
 */
#if !defined(ENABLE_CUSTOM_OBJECT_FACTORY)
#if ENABLE_LEGACY_KEYWORDS && !defined(JSON_LOADER_LIBRARY)
// 当且仅当某些项目使用了1.0版本的JsonLoader时，才禁用自定义对象工厂 [6/16/2016 CHENHONGHAO]
#define ENABLE_CUSTOM_OBJECT_FACTORY        0
#else
#define ENABLE_CUSTOM_OBJECT_FACTORY        1
#endif
#endif

/**
 *  @macro ENABLE_DYNAMIC_LIBRARY
 *  @brief 是否以动态链接库的形式使用JsonLoader，目前仅支持Windows平台
 */
#ifndef ENABLE_DYNAMIC_LIBRARY
#define ENABLE_DYNAMIC_LIBRARY              1
#endif
#if ENABLE_DYNAMIC_LIBRARY
#ifdef JSON_LOADER_LIBRARY
#define JSON_LOADER_EXPORT                  Q_DECL_EXPORT
#else
#define JSON_LOADER_EXPORT                  Q_DECL_IMPORT
#endif
#else
#define JSON_LOADER_EXPORT
#endif

#if 0
#include <QObject>
#include <QList>
#include <QHash>
#include <QSet>
#include <QVariant>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#endif


#define DECLARE_CLASS_WRAPPER(_class)                                                                   \
class _class##Wrapper : public _class                                                                   \
{                                                                                                       \
public:                                                                                                 \
    _class##Wrapper() {}                                                                                \
    _class##Wrapper(const _class##Wrapper& other) {}                                                    \
};

#if !defined(Q_ENABLE_COPY) && 1
/**
 *  @macro Q_ENABLE_COPY
 *  @brief 表示一个QObject的子类可以被拷贝（对应Q_DISABLE_COPY系统宏），仅用于支持元对象系统的对象动态创建
 */
#define Q_ENABLE_COPY(_type)                                                                            \
    public: _type(const _type& other) {                                                                 \
        Q_ASSERT_X(false, #_type" Meta Copy-Constructor", #_type" Meta-Objects can NOT be copied.");    \
    }                                                                                                   
#endif

#if ENABLE_CUSTOM_OBJECT_FACTORY
#define REGISTER_SIMPLE_TYPE_METHOD         ObjectType::registerSimpleObjectType
#define REGISTER_METATYPE_METHOD            ObjectType::registerMetaObjectType
#define GET_METATYPE_ID_METHOD(_name)       ObjectType::type(_name)
#define GET_METATYPE_NAME_METHOD(_id)       ObjectType::typeName(_id)
#define CREATE_METATYPE_OBJECT_METHOD(_id)  ObjectType::create(_id)
#define GET_METAOBJECT_METHOD(_id)          ObjectType::metaObjectForType(_id)
#else
#define REGISTER_SIMPLE_TYPE_METHOD         qRegisterMetaType
#define REGISTER_METATYPE_METHOD            qRegisterMetaType
#define GET_METATYPE_ID_METHOD(_name)       QMetaType::type(QString(_name).toLatin1().constData())
#define GET_METATYPE_NAME_METHOD(_id)       QMetaType::typeName(_id)
#define CREATE_METATYPE_OBJECT_METHOD(_id)  QMetaType::create(_id)
#define GET_METAOBJECT_METHOD(_id)          QMetaType::metaObjectForType(_id)
#endif

/**
 *  @macro REGISTER_METATYPE_X
 *  @brief 注册一个元对象类型，允许指定与类名称不同的注册名称
 */
#define REGISTER_METATYPE_X(_type, _name) {                                                             \
    if (REGISTER_SIMPLE_TYPE_METHOD<_type*>(_name"*") == QMetaType::UnknownType) {                      \
        Q_ASSERT_X(0, "REGISTER_METATYPE_X", "Failed to register MetaType " _name "*");                 \
    }                                                                                                   \
    if (REGISTER_METATYPE_METHOD<_type>(_name) == QMetaType::UnknownType) {                             \
        Q_ASSERT_X(0, "REGISTER_METATYPE_X", "Failed to register MetaType " _name );                    \
    }                                                                                                   \
}

/**
 *  @macro REGISTER_META_TYPE
 *  @brief 注册一个元对象类型，默认注册名称与类名称相同
 */
#define REGISTER_METATYPE(_type) REGISTER_METATYPE_X(_type, #_type)

/**
 *  @macro REGISTER_GENERIC_LIST_PARSER
 *  @brief 注册一个QList<XXX*>样式的数组解析器到指定的JsonLoader对象
 */
#define REGISTER_GENERIC_LIST_PARSER(_type, _loader) {                                                  \
    QString typeName = QString::fromLatin1(#_type);                                                     \
    if (typeName.endsWith(QLatin1Char('*'))) typeName.chop(1);                                          \
    int metaType = GET_METATYPE_ID_METHOD(typeName);                                                    \
    Q_ASSERT_X(metaType != QMetaType::UnknownType, "REGISTER_GENERIC_LIST_PARSER",                      \
        "Unknown MetaType "#_type", call REGISTER_METATYPE first.");                                    \
    (_loader).registerArrayValueParser(new GenericListValueParser<_type>(&(_loader), metaType));        \
}

#endif
/*********************************************************************************************************
** End of file
*********************************************************************************************************/