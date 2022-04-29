// LogViewerDoc.h : interface of the CLogViewerDoc class
//


#pragma once

#include "LogManager.h"
#include "VsIdeHandler.h"
//class CLogFilterManager;
#include <ftlSystem.h>
#include "../LogFilterBase.h"

#define SECTION_CONFIG          TEXT("Config")
#define ENTRY_ACTIVE_INI        TEXT("ActiveIni")
#define ENTRY_SOURCE_DIR        TEXT("SourceDir")
#define ENTRY_SOURCES_HISTORY   TEXT("SourcesHistory")

#define SOURCES_HISTORY_COUNT   5

class CLogViewerDoc : public CDocument
{
protected: // create from serialization only
    CLogViewerDoc();
    DECLARE_DYNCREATE(CLogViewerDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);

// Implementation
public:
    virtual ~CLogViewerDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

public:
    CLogManager   m_FTLogManager;
    BOOL GoToLineInSourceCode(LPCTSTR pszFileName,int line);
    void GetFileFilterString(CString & strFilter);
protected:
    CVsIdeHandler   m_VSIdeHandler;
// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
public:

    afx_msg void OnFileOpen();
    afx_msg void OnFileSave();
    afx_msg void OnFileClose();
    afx_msg void OnFileReload();
    //afx_msg void OnFileGenerateLog();
    
private:
    BOOL _SelectActiveIde();
};


