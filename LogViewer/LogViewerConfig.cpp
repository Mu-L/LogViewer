#include "StdAfx.h"
#include "LogViewerConfig.h"

struct ItemMapInfo  
{
    LPCTSTR pszKeyItem;
    INT*	pItemIndex;
};

struct LevelMapInfo
{
    LPCTSTR			pszLevelItem;
    std::string*	pstrLevelText;
};

CLogViewerConfig::CLogViewerConfig(void)
{
    m_nItemTime = INVLIAD_ITEM_MAP;
    m_nItemLevel = INVLIAD_ITEM_MAP;
    m_nItemMachine = INVLIAD_ITEM_MAP;
    m_nItemPId = INVLIAD_ITEM_MAP;
    m_nItemTId = INVLIAD_ITEM_MAP;
    m_nItemModule = INVLIAD_ITEM_MAP;
    m_nItemFun = INVLIAD_ITEM_MAP;
    m_nItemFile = INVLIAD_ITEM_MAP;
    m_nItemLine = INVLIAD_ITEM_MAP;
    m_nItemLog = INVLIAD_ITEM_MAP;
    m_dateTimeType = dttDateTime;
}

CLogViewerConfig::~CLogViewerConfig(void)
{
}

BOOL CLogViewerConfig::LoadConfig(LPCTSTR pszConfigFile)
{
    BOOL bRet = TRUE;
    API_VERIFY(m_config.SetFileName(pszConfigFile));
    if (m_config.IsIniFileExist())
    {
        m_config.GetString(SECTION_COMMON, KEY_REGULAR, DEFAULT_NULL_VALUE, m_strLogRegular);
        m_config.GetString(SECTION_COMMON, KEY_SOURCE_FILE_EXTS, _T("*.*"), m_strSourceFileExts);
        m_config.GetString(SECTION_COMMON, KEY_TIMEFORMAT, DEFAULT_NULL_VALUE, m_strTimeFormat);
        m_config.GetString(SECTION_COMMON, KEY_DISPLAY_TIMEFORMAT, m_strTimeFormat, m_strDisplayTimeFormat);

        _LoadItemMaps();
        _LoadLevelMaps();
    }

    return bRet;
}

FTL::TraceLevel CLogViewerConfig::GetTraceLevelByText(const std::string& strLevel)
{
    FTL::TraceLevel level = tlTrace;
    for (int i = 0; i < _countof(m_strLevelsText); i++)
    {
        if (strLevel.compare(m_strLevelsText[i]) == 0)
        {
            level = FTL::TraceLevel(i);
            break;
        }
    }
    return level;
}


BOOL CLogViewerConfig::_LoadItemMaps()
{
    BOOL bRet = TRUE;

    ItemMapInfo itemMapInfos[] = {
        { KEY_ITEM_TIME, &m_nItemTime},
        { KEY_ITEM_LEVEL, &m_nItemLevel},
        { KEY_ITEM_MACHINE, &m_nItemMachine},
        { KEY_ITEM_PID,	 &m_nItemPId},
        { KEY_ITEM_TID,	 &m_nItemTId},
        { KEY_ITEM_MODULE, &m_nItemModule},
        { KEY_ITEM_FUN, &m_nItemFun},
        { KEY_ITEM_FILE, &m_nItemFile},
        { KEY_ITEM_LINE, &m_nItemLine},
        { KEY_ITEM_LOG, &m_nItemLog},
    };

    CString strItemValue;
    for (int i = 0; i < _countof(itemMapInfos); i++)
    {
        m_config.GetString(SECTION_REGMAP, itemMapInfos[i].pszKeyItem, DEFAULT_NULL_VALUE, strItemValue);
        *itemMapInfos[i].pItemIndex = _ConvertItemMapValue(strItemValue);
    }

    return bRet;
}

BOOL CLogViewerConfig::_LoadLevelMaps()
{
    BOOL bRet = FALSE;

    LevelMapInfo levelMapInfos[] = {
        { KEY_LEVEL_DETAIL, &m_strLevelsText[FTL::tlDetail]},
        { KEY_LEVEL_INFO, &m_strLevelsText[FTL::tlInfo]},
        { KEY_LEVEL_TRACE,	 &m_strLevelsText[FTL::tlTrace]},
        { KEY_LEVEL_WARN, &m_strLevelsText[FTL::tlWarn]},
        { KEY_LEVEL_ERROR, &m_strLevelsText[FTL::tlError]},
    };

    FTL::CFConversion conv;
    //CString strItemValue;
    for (int i = 0; i < _countof(levelMapInfos); i++)
    {
        CString strLevelValue;
        m_config.GetString(SECTION_LEVEL_MAP, levelMapInfos[i].pszLevelItem, DEFAULT_NULL_VALUE, strLevelValue);
        if (!strLevelValue.IsEmpty())
        {
            *levelMapInfos[i].pstrLevelText = conv.TCHAR_TO_UTF8(strLevelValue);
        }
    }

    return bRet;
}

INT CLogViewerConfig::_ConvertItemMapValue(const CString& strItemValue){
    INT nItemValue = -1;
    if (!strItemValue.IsEmpty())
    {
        if (strItemValue.GetLength() >= 2 && strItemValue[0]==_T('$'))
        {
            nItemValue = _ttoi((LPCTSTR)strItemValue.Mid(1));
        }
    }
    return nItemValue;
}