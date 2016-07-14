/****************************************Copyright (c)****************************************************
**
**                                       D.H. InfoTech
**
**--------------File Info---------------------------------------------------------------------------------
** File name:                  JsonLoader.h
** Latest Version:             V1.3.0
** Latest modified Date:       2016/6/15
** Modified by:                Chen Honghao
** Descriptions:               V1.0.1: 修正枚举类型的属性无法载入的问题[2016/3/23]
**                             V1.1.2: 修正多处问题，强化针对String->int和String->bool等内部类型的自动转换[2016/04/11]
**                             V1.2.0: 强化多处功能[2016/05/09]：
**                                     [1] 保留属性关键字全部以.开头，避免与用户自定义属性冲突，保留属性关键字包括：
**                                         .id/.type/.objects/.connections/.qparent/.qchildren
**                                     [2] 被注册的用户类型不再要求提供拷贝构造函数
**                                     [3] 容器（QList/QVector/自定义）属性的匹配使用正则表达式，从而实现用户扩展
**                                     [4] 支持默认创建对象类型设置，方便直接在顶层载入大型数组时JSON文件的编写（省略.type）
**                                     [5] 加强JSON语法错误时的错误信息提示，可直接显示错误处的代码（类似于C++编译器）
**                                     [6] 加强信号/槽名字的fuzzy-match功能
**                             V1.2.1: [1] 支持发布动态链接库版本，拆分独立的JsonLoader_p.h用于配置，外部无需包含此文件
**                             V1.2.2: [1] 增加.ref关键字，从而支持对象使用prototype进行增量创建，也就是在现有对象的基础上，
**                                         仅修改需要改动的内容，而其他属性保持prototype的默认值
**                             V1.2.3: [1] 支持QVariant类型的对象属性，可使用任意QVariant原生支持的值给属性赋值，例如QString
**                             V1.3.0: 增加多处新特性[2016/06/15]:
**                                     [1] 支持QVariantList类型的对象属性
**                                     [2] .ref可配合.copy关键字使用，以被引用的对象作为原型创建新对象，必须提供拷贝构造函数
**                                     [3] .ref关键字支持与.id关键字同时使用，将被引用的对象重命名
**                                     [4] .ref关键字支持直接指向文件名，例如：".ref": "../modules/MyModule.json"，类似于#include
**                                     [5] 支持属性的嵌套！例如"son.grandSon.propertyOfGrandSon": "Hello"，
**                                         前提是属性对象已经创建，可考虑在构造函数中之间创建对应的属性对象
**                                     [6] 支持对属性加载顺序的设置，从而解决父子对象的属性依赖问题（仍不支持双向递归依赖）
**                                     [7] 优化各个KeyParser的顺序，提升JSON加载速度
**
**--------------------------------------------------------------------------------------------------------
** Created by:                 Chen Honghao
** Created date:               2016/3/1
** Descriptions:               JSON对象解析器，可以加载JSON文件中描述的QObject对象
**                             [1] 解析器支持对象创建及属性初始化、属性绑定（支持notify）、信号/槽绑定
**                             [2] 需要被动态创建的对象，必须在载入前先使用REGISTER_METATYPE宏声明动态类
**                             [3] 解析器支持C-Style的单行注释（//），目前不支持多行注释
**                             [4] 解析器支持翻译功能，只需在对象载入后执行translateAllStrings操作，
**                                 中文将自动加入翻译列表（推荐），特殊需要翻译的英文需要使用`tr`标签，如：
**                                 "implicitStringToTranslate": "`tr`EnglishIsNotTranslatedByDefault"
**                             [5] 解析器支持通过注册外部Parser来进行语法扩展，主要包括以下几类：
**                                 (a) 创建对象的方法，需要注册ObjectCreator
**                                 (b) 自定义Key解析器，需要注册外部KeyParser
**                                 (c) 自定义ArrayValue解析器，需要注册外部ArrayValueParser
**                                 (d) 自定义StringValue解析器，需要注册外部StringValueParser
**                                 以上内容可参考Parser.h中默认已注册的解析器，也可联系开发者。
** 
*********************************************************************************************************/
#ifndef __JSONLOADER_H__
#define __JSONLOADER_H__


#include <QObject>
#include <QList>
#include <QHash>
#include <QSet>
#include <QVariant>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include "Object.h"
#include "Parser.h"

/**
 *  @class JsonLoader
 *  @brief JSON对象解析器（通常整个程序只需使用一个JsonLoader对象）
 */
class JSON_LOADER_EXPORT JsonLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    JsonLoader();

public:
    /**
     *  @enum  ErrorCode
     *  @brief 错误码
     */
    enum ErrorCode
    {
        NoError = 0,                        //!< 未发生错误
        JsonLoaderError = 64,               //!< 一般JSON解析器错误
        InvalidFile,                        //!< 非法输入文件（文件不存在、无访问权限等）
        EmptyDocument,                      //!< 空的JSON文件，或者该文件仅包含注释
        InvalidDocument,                    //!< 无效的JSON文件（可通过错误信息检查文件错误偏移量）
        InvalidRootValue,                   //!< 无效的JSON根对象/数组数据，说明子JSON文件并未载入任何对象
        InvalidArrayType,                   //!< 无效的数组类型
        InvalidKeyValue,                    //!< 无效的JSON Key
        InvalidStringValue,                 //!< 无效的JSON Value
        UnsupportedFeature,                 //!< 不支持的特性，例如数组嵌套等
        ObjectCreatorError,                 //!< 对象创建器错误
        KeyParserError,                     //!< Key解析器错误
        StringValueParserError,             //!< StringValue解析器错误
        ArrayValueParserError,              //!< ArrayValue解析器错误
        TranslationError                    //!< 翻译错误
    };

    /**
     *  @enum  PropertyDependencyMode
     *  @brief 被加载的对象树的属性依赖关系
     *  @note  目前不支持双向递归依赖，如果确实存在此问题，应在应用程序中尝试延迟初始化来解除递归依赖关系
     */
    enum PropertyDependencyMode
    {
        ParentDependsOnChildren = 1,        //!< 父对象依赖于子对象，将先加载子对象，然后逐级向上加载
        ChildrenDependsOnParent = 2,        //!< 子对象依赖于父对象，将先加载父对象，然后逐级向下加载

        Default = ParentDependsOnChildren   //!< 默认依赖
    };

public: 
    /*! 
     * 载入内存中的JSON数据（例如来自网络的、代码中的JSON）
     * @param[in]  jsonData         内存中的JSON数据
     * @param[in]  parentKey        通常需要为该JSON数据指定一个Key，用于标识对象树的主分支
     * @param[in]  defaultMetaType  如果JSON对象未指定类型，则使用此默认类型创建对象（仅使用一次）
     * @return     加载得到的根对象或根数组
     */
    QVariant load(const QByteArray& jsonData, const QString& parentKey, int defaultMetaType = QMetaType::UnknownType);

    /*! 
     * 载入文件中的JSON数据
     * @param[in]  jsonFile         JSON文件路径
     * @param[in]  defaultMetaType  如果JSON对象未指定类型，则使用此默认类型创建对象（仅使用一次）
     * @return     加载得到的根对象或根数组
     * @note       该JSON文件的根对象将指定默认Key为文件路径
     */
    QVariant load(const QString& jsonFile, int defaultMetaType = QMetaType::UnknownType);

    /**
     * 添加一个全局对象，使该对象可以被本JsonLoader内部的各个QObject对象所引用，绑定信号/槽等
     * @param[in]    object 全局对象
     * @return       操作成功返回true
     */
    bool addGlobalObject(QObject* object);

    /**
     * 移除已经添加的全局对象
     * @param[in]    object         已经添加的全局对象
     * @param[in]    removeChildren 是否移除子对象，目前不支持不移除
     * @return       操作成功返回true
     */
    bool removeGlobalObject(QObject* object, bool removeChildren = true);

    /**
     * 根据对象名称从上到下查找已经加载的对象
     * @param[in]    objectName 对象名称
     * @return       查找结果，未找到则返回NULL
     */
    QObject* findObject(const QString& objectName);

    /**
     * 根据对象名称从上到下查找已经加载的对象，并将对象指针cast为指定的类型
     * @param[in]    objectName 对象名称
     * @return       查找结果，未找到则返回NULL
     */
    template <typename T>
    T findObject(const QString& objectName)
    {
        QObject* object = findObject(objectName);
        return qobject_cast<T>(object);
    }

    /*!  
     * Getter/Setter for defaultMetaType
     */
    int defaultMetaType() const 
    { 
        return m_defaultMetaType; 
    }
    void setDefaultMetaType(int defaultMetaType) 
    { 
        m_defaultMetaType = defaultMetaType; 
    }

    /*!
     * Getter/Setter for propertyDependencyMode
     */
    PropertyDependencyMode propertyDependencyMode() const
    {
        return m_propertyDependencyMode;
    }
    void setPropertyDependencyMode(PropertyDependencyMode propertyDependencyMode)
    {
        m_propertyDependencyMode = propertyDependencyMode;
    }

    /*! 
     * 清除载入过程中使用的临时缓冲区等，释放内存
     * @note 执行本操作需要一定时间，仅用于内存资源受限的设备，并仅应在全部对象已经载入后使用一次
     */
    void cleanup();

    /**
     * 翻译/重新翻译全部的可翻译字符串（中文字符串或标记了`tr`的强制翻译的特殊字符串）
     * @return      成功翻译的字符串个数
     */
    int translateAllStrings();

#if ENABLE_TS_FILE
    /*! 
     * 将当前JsonLoader已经载入的可翻译字符串保存至QT预言家的ts文件，用于人工翻译
     * @param[in]  tsFile 保存文件路径，若文件已存在，将覆盖该文件，目前不支持文件合并
     * @return     操作成功返回true
     */
    bool createTranslationFile(const QString& tsFile) const;
#endif

    /*! 
     * 错误信号，可通过绑定该信号获取错误通知
     * @param[in]  code     错误码
     * @param[in]  message  错误消息
     */
    Q_SIGNAL void error(int code, const QString& message) const;

    /**
     * 注册一个外部的对象创建器，用于语法扩展
     * @param[in]    creator    对象创建器
     * @return       操作成功返回true
     */
    bool registerObjectCreator(ObjectCreator* creator);

    /**
     * 注册一个外部的Key解析器，用于语法扩展
     * @param[in]    parser    Key解析器
     * @return       操作成功返回true
     */
    bool registerKeyParser(KeyParser* parser);

    /**
     * 注册一个外部的ArrayValue解析器，用于语法扩展
     * @param[in]    parser    ArrayValue解析器
     * @return       操作成功返回true
     */
    bool registerArrayValueParser(ArrayValueParser* parser);

    /**
     * 注册一个外部的StringValue解析器，用于语法扩展
     * @param[in]    parser    StringValue解析器
     * @return       操作成功返回true
     */
    bool registerStringValueParser(StringValueParser* parser);

protected: 
    /*! 
     * 载入内存中的JSON数据（例如来自网络的、代码中的JSON）
     * @param[in]  jsonData             内存中的JSON数据
     * @param[in]  parentContext        该JSON数据的父对象，该JSON中的全部对象将被挂载于父对象下方
     * @param[in]  parentKey            通常需要为该JSON数据指定一个Key，用于标识对象树的主分支
     * @param[in]  possibleObjectList   可能是对象的对象上下文（ObjectContext）列表，追加模式，原内容不清空
     * @param[in]  jsonObjectList       可能是JSON文件的对象上下文（ObjectContext）列表，追加模式，原内容不清空
     * @return     加载得到的根对象或根数组
     */
    QVariant load(    
        const QByteArray& jsonData, 
        ObjectContext& parentContext, 
        const QString& parentKey,
        QList<ObjectContext*>& possibleObjectList,
        QList<ObjectContext*>& jsonObjectList
        );

    /*! 
     * 载入文件中的JSON数据
     * @param[in]  jsonFile             JSON文件路径
     * @param[in]  parentContext        该JSON数据的父对象，该JSON中的全部对象将被挂载于父对象下方
     * @param[in]  parentKey            通常需要为该JSON数据指定一个Key，用于标识对象树的主分支
     * @param[in]  possibleObjectList   可能是对象的对象上下文（ObjectContext）列表，追加模式，原内容不清空
     * @param[in]  jsonObjectList       可能是JSON文件的对象上下文（ObjectContext）列表，追加模式，原内容不清空
     * @return     加载得到的根对象或根数组
     */
    QVariant load(
        const QString& jsonFile,
        ObjectContext& parentContext, 
        const QString& parentKey,
        QList<ObjectContext*>& possibleObjectList,
        QList<ObjectContext*>& jsonObjectList
        );

    /*! 
     * 移除JSON数据中的注释（由于JSON原生语法不支持注释，这里人为引入C-Style注释并在解析前移除）
     * @param[in]  jsonData 含注释的JSON数据
     * @return     不含注释的JSON数据
     */
    QByteArray removeComments(const QByteArray& jsonData) const;

    /*! 
     * 根据指定的JSON数据及其子数据，创建（可能的）对象上下文（ObjectContext）树
     * @param[in]  jsonValue            定的JSON数据
     * @param[in]  parentContext        该JSON数据的父对象，该JSON中的全部对象将被挂载于父对象下方
     * @param[in]  parentKey            通常需要为该JSON数据指定一个Key，用于标识对象树的主分支
     * @param[in]  possibleObjectList   可能是对象的对象上下文（ObjectContext）列表，追加模式，原内容不清空
     * @return     对象上下文（ObjectContext）树新增节点个数
     */
    int createObjectContextTree( 
        const QJsonValue& jsonValue, 
        ObjectContext& parentContext, 
        const QString& parentKey, 
        QList<ObjectContext*>& possibleObjectList 
        );

    /*! 
     * 为指定的对象上下文创建QObject对象
     * @param[in]  objectContext 指定的对象上下文
     * @return     bool
     */
    bool createQObject(ObjectContext& objectContext);

    /*! 
     * 查找指定的对象上下文中可能嵌套的JSON文件
     * @param[in]  objectContext  指定的对象上下文
     * @param[in]  jsonObjectList 可能是JSON文件的对象上下文（ObjectContext）列表，追加模式，原内容不清空
     * @return     指定的对象上下文确实嵌套了JSON文件则返回true
     */
    bool findJsonObject(ObjectContext& objectContext, QList<ObjectContext*>& jsonObjectList);

    /*! 
     * 解析指定的对象上下文的一个JSON Key
     * @param[in]  objectContext 指定的对象上下文
     * @param[in]  iter          指定的对象上下文的Key迭代器（的当前值）
     * @return     操作成功返回true
     */
    bool parseKey(ObjectContext& objectContext, KeyObjectContextMapConstIter iter);

    /*! 
     * 解析指定的对象上下文的全部JSON Key
     * @param[in]  objectContext 指定的对象上下文
     * @return     全部操作成功返回true
     */
    bool parseKeys(ObjectContext& objectContext);

    /*! 
     * 解析Key或者Value中的tags
     * @param[inout] string JSON中的原始字符串，以及裁减后的字符串输出
     * @param[in]    tags   解析得到的tags
     * @return       解析得到的tag个数
     */
    int parseTags(QString& string, QStringList& tags) const;

    /*! 
     * 解析指定的对象上下文的一个JSON Value
     * @param[in]  objectContext 指定的对象上下文
     * @param[in]  metaTypeHint  Value的元数据类型的提示
     * @return     该JSON Value解析得到的QVariant对象
     */
    QVariant parseValue(ObjectContext& objectContext, int metaTypeHint = QMetaType::UnknownType);

    /*! 
     * 解析一个JSON Value Array的数组元素类型，通常根据Key Property的类型来unwind得到元素类型
     * @param[in]  propertyMetaTypeId  （属性）数组本身的metaType
     * @param[in]  propertyMetaTypeName 属性类型名称
     * @return     数组元素的metaType
     */
    int parseArrayElementType(int propertyMetaTypeId, const QString& propertyMetaTypeName);

    /*! 
     * 解析一个JSON Value Array
     * @param[in]  valueArray   Value Array
     * @param[in]  metaTypeHint Value Array的元数据类型的提示
     * @return     该JSON Value Array解析得到的QVariant对象
     */
    QVariant parseArrayValue(const QVariantList& valueArray, int metaTypeHint = QMetaType::UnknownType);

    /*! 
     * 添加指定的对象上下文对应的一条翻译信息（一一对应）
     * @param[in]  objectContext 指定的对象上下文
     * @return     操作成功返回true
     */
    bool addTranslation(ObjectContext& objectContext);

    /*! 
     * 移除指定的对象上下文对应的一条翻译信息（一一对应）
     * @param[in]  objectContext 指定的对象上下文
     * @return     操作成功返回true
     */
    bool removeTranslation(ObjectContext& objectContext);

    /*! 
     * 读取一个JSON文件的全部数据，去除注释并缓存，从而加快多次载入的文件的处理速度
     * @param[in]  jsonFile JSON文件路径
     * @return     载入的JSON数据
     */
    QByteArray readJsonFile(const QString& jsonFile);

    /*! 
     * 分配一个对象上下文，可能使用内存池
     * @param[in]  parentKey    用于初始化该对象上下文的parentKey
     * @param[in]  jsonValue    用于初始化该对象上下文的jsonValue
     * @return     分配得到的对象上下文指针
     */
    ObjectContext* allocObjectContext(const QString& parentKey, const QJsonValue& jsonValue);

    /*! 
     * 释放一个对象上下文，可能使用内存池
     * @param[in]  objectContext 分配得到的对象上下文指针
     * @return     操作成功返回true
     */
    bool freeObjectContext(ObjectContext* objectContext);

#if JSON_LOADER_DEBUGGING_LEVEL >= 1
    /*! 
     * JsonLoader错误的默认处理槽函数，输出错误打印信息，可通过宏配置禁用该功能
     * @param[in]  code     错误码
     * @param[in]  message  错误消息
     */
    Q_SLOT void reportError(int code, const QString& message) const;
#endif

    /*! 
     * 转储JSON数据，用于在Qt的JSON解析库报错时，显示错误位置对应的代码
     * @param[in]  data     完整的JSON数据，建议以换行作为格式化方法
     * @param[in]  offset   需要转储的中心位置，如果使用换行作为格式化方法，则转储前后各几行，否则转储前后各一段长度
     * @return     转储结果，可直接显示
     */
    QString dumpJsonData(const QByteArray& data, int offset) const;


private:
    ObjectContext                   m_rootObjectContext;                //!< 根对象上下文
    QSet<ObjectContext*>            m_translations;                     //!< 可翻译字符串列表

    QList<ObjectCreator*>           m_objectCreators;                   //!< 对象创建器容器
    QList<KeyParser*>               m_keyParsers;                       //!< Key解析器容器
    QHash<int, ArrayValueParser*>   m_arrayValueParsers;                //!< ArrayValue解析器容器
    QHash<int, StringValueParser*>  m_stringValueParsers;               //!< StringValue解析器容器

    QHash<QString, QByteArray>      m_jsonDataBuffer;                   //!< 已载入的JSON文件内容的缓冲区
#if ENABLE_MEM_POOL
    QList<ObjectContext>            m_objectContextBuffer;              //!< 用于分配对象上下文的内存池
#endif

    int                             m_defaultMetaType;                  //!< 载入顶层JSON数据时，提供的默认MetaType提示
    PropertyDependencyMode          m_propertyDependencyMode;           //!< 对象树的属性依赖关系

    /*
     * @brief 由于IParser中使用了JsonLoader的保护操作，这里声明为友元
     */
    friend class IParser;
};

#endif
/*********************************************************************************************************
** End of file
*********************************************************************************************************/