// LogItemView.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "LogViewer.h"
#include "LogItemView.h"
#include "LogManager.h"
#include "MachinePidTidTreeView.h"
#include "DialogGoTo.h"
#include <ftlShell.h>
#include <regex>

struct strColumnInfo
{
    LPCTSTR pszColumnText;
    int		nColumnWidth;
};

static strColumnInfo	columnInfos[] = 
{
    {TEXT("SeqNum"), 50,},
    {TEXT("Machine"), 50,},
    {TEXT("PID"), 50,},
    {TEXT("TID"), 80,},
    {TEXT("Time"), 100,},
    {TEXT("Elapse"), 70,},
    {TEXT("Level"), 50,},
    {TEXT("ModuleName"), 20,},
    {TEXT("FunName"), 20,},
    {TEXT("SourceFile"), 20,},
    {TEXT("TraceInfo"), 800,}
};

static LPCTSTR pszTraceLevel[] = 
{
    TEXT("Detail"),
    TEXT("Info"),
    TEXT("Trace"),
    TEXT("Warn"),
    TEXT("Error"),
    TEXT("Unknown"),
};
// CLogItemView

IMPLEMENT_DYNCREATE(CLogItemView, CListView)

CLogItemView::CLogItemView()
{
    m_bInited = FALSE;
    m_SortContentType = type_Sequence;
    m_bSortAscending = TRUE;
    m_dwDefaultStyle |= ( LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA );
    m_ptContextMenuClick.SetPoint(-1, -1);
    m_nLastGotToSeqNumber = 0L;
}

CLogItemView::~CLogItemView()
{
}

BEGIN_MESSAGE_MAP(CLogItemView, CListView)
    ON_COMMAND_EX(ID_EDIT_GOTO, &CLogItemView::OnEditGoTo)
    ON_NOTIFY(HDN_ITEMCLICK, 0, &CLogItemView::OnHdnItemclickListAllLogitems)
    //ON_NOTIFY_RANGE(LVN_COLUMNCLICK,0,0xffff,OnColumnClick)
    //ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
    ON_NOTIFY_REFLECT(NM_CLICK, &CLogItemView::OnNMClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, &CLogItemView::OnNMDblclk)
    ON_COMMAND(ID_DETAILS_HIGHLIGHT_SAME_THREAD, &CLogItemView::OnDetailsHighLightSameThread)
    ON_COMMAND(ID_DETAILS_COPY_ITEM_TEXT, &CLogItemView::OnDetailsCopyItemText)
    ON_COMMAND(ID_DETAILS_COPY_LINE_TEXT, &CLogItemView::OnDetailsCopyLineText)
    ON_COMMAND(ID_DETAILS_COPY_FULL_LOG, &CLogItemView::OnDetailsCopyFullLog)
    ON_COMMAND(ID_DETAILS_DELETE_SELECT_ITEMS, &CLogItemView::OnDetailDeleteSelectItems)
    ON_COMMAND(ID_DETAILS_SELECT_CURRENT_PID, &CLogItemView::OnDetailSelectCurrentPid)
    ON_COMMAND(ID_DETAILS_SELECT_CURRENT_TID, &CLogItemView::OnDetailSelectCurrentTid)
    ON_WM_CONTEXTMENU()
    ON_WM_ERASEBKGND()
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CLogItemView::OnLvnItemchanged)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_SELECTED_LOGITEM, &CLogItemView::OnUpdateIndicatorSelectedLogItem)
END_MESSAGE_MAP()


// CLogItemView ���

#ifdef _DEBUG
void CLogItemView::AssertValid() const
{
    CListView::AssertValid();
}

void CLogItemView::Dump(CDumpContext& dc) const
{
    CListView::Dump(dc);
}
#endif //_DEBUG


// CLogItemView ��Ϣ�������

void CLogItemView::PrepareCache(int /*iFrom*/, int /*iTo*/)
{

}

void CLogItemView::OnInitialUpdate()
{
    //BOOL bRet = FALSE;
    CListView::OnInitialUpdate();
    CListCtrl& ListCtrl = GetListCtrl();
    if (FALSE == m_bInited)
    {
        ListCtrl.ModifyStyle(LVS_ICON,LVS_REPORT | LVS_OWNERDATA,0);
        DWORD dwExStyle = ListCtrl.GetExtendedStyle();
        //TODO: ��Ȼ���� LVS_EX_TRACKSELECT �������ʾ��ǰѡ�е��У�������ʱ���Զ���������λ��ѡ��
        dwExStyle |= LVS_EX_FULLROWSELECT  | LVS_EX_ONECLICKACTIVATE 
            | LVS_EX_DOUBLEBUFFER|LVS_EX_GRIDLINES; //|LVS_EX_TRACKSELECT;// LVS_EX_INFOTIP
        ListCtrl.SetExtendedStyle(dwExStyle);
        m_ctlHeader.SubclassWindow(ListCtrl.GetHeaderCtrl()->GetSafeHwnd());

        LV_COLUMN lvc;
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
        for(int i = 0; i< _countof(columnInfos); i++)
        {
            lvc.iSubItem = i;
            lvc.pszText = (LPTSTR)columnInfos[i].pszColumnText;
            lvc.cx = columnInfos[i].nColumnWidth;
            lvc.fmt = LVCFMT_LEFT;
            ListCtrl.InsertColumn(i,&lvc);
        }
        m_bInited = TRUE;
    }
    CLogViewerDoc* pDoc = GetDocument();
    LONG lLogItemCount = pDoc->m_FTLogManager.GetDisplayLogItemCount();
    ListCtrl.SetItemCount(lLogItemCount);
#if 0
    ListCtrl.DeleteAllItems();

    LV_ITEM lvi = {0};
    int itemIndex = 0;
    CString strFormat;
    for (LONG index = 0; index < lLogItemCount; index++)
    {
        LPLogItem pLogItem = pDoc->m_FTLogManager.GetLogItem(index);
        lvi.mask = LVIF_TEXT;// | LVIF_STATE;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        strFormat.Format(TEXT("%d"),pLogItem->lSeqNum); 
        lvi.pszText = (LPTSTR)(LPCTSTR)strFormat;
        itemIndex = ListCtrl.InsertItem(&lvi);
        ASSERT(itemIndex == index);

        strFormat.Format(TEXT("%d"),pLogItem->lThreadId);
        API_VERIFY(ListCtrl.SetItemText(itemIndex,type_ThreadId,strFormat));

        if (pLogItem->stTime.dwHighDateTime != 0 && pLogItem->stTime.dwLowDateTime != 0)
        {
            CTime time(pLogItem->stTime);
            API_VERIFY(ListCtrl.SetItemText(itemIndex,type_Time,time.Format(TEXT("%H:%M:%S"))));
        }
        else
        {
            API_VERIFY(ListCtrl.SetItemText(itemIndex,type_Time,TEXT("<NoTime>")));
        }

        API_VERIFY(ListCtrl.SetItemText(itemIndex,type_TraceInfo,pLogItem->pszTraceInfo));
        API_VERIFY(ListCtrl.SetItemData(itemIndex,(DWORD_PTR)pLogItem));
    }
#endif
}

void CLogItemView::OnHdnItemclickListAllLogitems(NMHDR *pNMHDR, LRESULT *pResult)
{
    //LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);

    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    const LogItemContentType sortType = (LogItemContentType)pNMListView->iItem;

    // if it's a second click on the same column then reverse the sort order,
    // otherwise sort the new column in ascending order.
    Sort( sortType, sortType == m_SortContentType ? !m_bSortAscending : TRUE );
    *pResult = 0;
}

//int CALLBACK CLogItemView::CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData )
//{
//    CLogItemView* pThis = reinterpret_cast<CLogItemView*>( lParamData );
//
//    LPLogItem pLogItem1 = LPLogItem(lParam1);
//    LPLogItem pLogItem2 = LPLogItem(lParam2);
//
//    int nResult = 0;
//    switch (pThis->m_SortContentType)
//    {
//    case type_Sequence:
//        nResult = pLogItem1->lSeqNum - pLogItem2->lSeqNum;
//        break;
//    case type_ThreadId:
//        nResult = pLogItem1->lThreadId - pLogItem2->lThreadId;
//        break;
//    case type_Time:
//        {
//            nResult = pLogItem1->stTime.dwHighDateTime - pLogItem2->stTime.dwHighDateTime;
//            if(0 == nResult)
//            {
//                nResult = pLogItem1->stTime.dwLowDateTime - pLogItem2->stTime.dwLowDateTime;
//            }
//        }
//        break;
//    case type_TraceLevel:
//        nResult = pLogItem1->level - pLogItem2->level;
//        break;
//    case type_TraceInfo:
//        nResult = _tcscmp(pLogItem1->pszTraceInfo,pLogItem2->pszTraceInfo);
//        break;
//    default:
//        ASSERT(FALSE);
//        break;
//    }
//
//    if (pThis->m_bSortAscending)
//    {
//        return nResult;
//    }
//    else
//    {
//        return -nResult;
//    }
//}

void CLogItemView::Sort(LogItemContentType contentType, BOOL bAscending )
{
    m_SortContentType = contentType;
    m_bSortAscending = bAscending;
    m_ctlHeader.SetSortArrow(m_SortContentType,m_bSortAscending);
    GetDocument()->m_FTLogManager.SortDisplayItem(m_SortContentType,bAscending);
    Invalidate();
    //VERIFY( GetListCtrl().SortItems( CompareFunction, reinterpret_cast<DWORD>( this ) ) );
}

void CLogItemView::GetDispInfo(LVITEM* pItem)
{
    BOOL bRet = FALSE;
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    // called when the listview needs to display data

    //���Ƶ��� LVIF_STATE(״̬), LVIF_IMAGE(ͼ��), LVIF_INDENT, LVIF_PARAM 
    if(pItem->mask & LVIF_TEXT) 
    {
        FTL::CFConversion conv;
        CString strFormat;
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(pItem->iItem);
        if (!pLogItem)
        {
            return ;
        }
        switch(pItem->iSubItem)
        {
        case type_Sequence:
            strFormat.Format(TEXT("%d"),pLogItem->seqNum);
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_Machine:
            strFormat = conv.UTF8_TO_TCHAR(pLogItem->machine.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_ProcessId:
            strFormat = conv.UTF8_TO_TCHAR(pLogItem->processId.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_ThreadId:
            strFormat = conv.UTF8_TO_TCHAR(pLogItem->threadId.c_str());
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            break;
        case type_Time:
            if (pLogItem->time != 0)
            {
                int microSec = 0;
                SYSTEMTIME st = {0};
                if(pLogItem->time > MIN_TIME_WITH_DAY_INFO){
                    //�����ڵ�ʱ��
                    //FILETIME localFileTime = {0};
                    FILETIME tm;
                    tm.dwHighDateTime = HILONG(pLogItem->time);//(pLogItem->time & 0xFFFFFFFF00000000) >> 32;
                    tm.dwLowDateTime = LOLONG(pLogItem->time);// ( pLogItem->time & 0xFFFFFFFF);
                    API_VERIFY(FileTimeToSystemTime(&tm,&st));
                    strFormat.Format(TEXT("%4d-%02d-%02d %02d:%02d:%02d:%03d"),
                        st.wYear, st.wMonth, st.wDay,
                        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

                }else{
                    microSec = pLogItem->time % TIME_RESULT_TO_MILLISECOND / (10 * 1000);
                    LONGLONG tmpTime = pLogItem->time / TIME_RESULT_TO_MILLISECOND;
                    st.wSecond = tmpTime % 60;
                    tmpTime /= 60;
                    st.wMinute = tmpTime % 60;
                    st.wHour = (WORD)tmpTime / 60;
                    strFormat.Format(TEXT("%02d:%02d:%02d.%03d"),
                        st.wHour, st.wMinute, st.wSecond, microSec);
                }

                StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            }
            else
            {
                StringCchCopy(pItem->pszText,pItem->cchTextMax -1,TEXT("<NoTime>"));
            }
            break;
        case type_ElapseTime:
            {
                strFormat.Format(TEXT("%.3f s"), (double)pLogItem->elapseTime * 100 / NANOSECOND_PER_SECOND);
                StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,(LPCTSTR)strFormat);
            }
            break;
        case type_ModuleName:
            if (NULL != pLogItem->pszModuleName)
            {
                StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)pLogItem->pszModuleName);
            }
            break;
        case type_FunName:
            {
                if (NULL != pLogItem->pszFunName)
                {
                    StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)pLogItem->pszFunName);
                }
                break;
            }
        case type_FileName:
            {
                if (NULL != pLogItem->pszSrcFileName)
                {
                    strFormat.Format(TEXT("%s(%d)"), pLogItem->pszSrcFileName, pLogItem->srcFileline);
                    StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)strFormat);
                }
                break;
            }
        case type_TraceLevel:
            StringCchCopy(pItem->pszText,pItem->cchTextMax - 1,pszTraceLevel[pLogItem->level]);
            break;
        case type_TraceInfo:
            if (NULL != pLogItem->pszTraceInfo){
                StringCchCopy(pItem->pszText,pItem->cchTextMax - 1, (LPCTSTR)pLogItem->pszTraceInfo);
            }
            break;
        default:
            ASSERT(FALSE);
            break;
        }
    }
}

int CLogItemView::FindItem(int /*iStart*/, LVFINDINFO* /*plvfi*/)
{
    //���ҵ�Ҫ���������ݺ�Ӧ�ðѸ����ݵ��������кţ����أ����û���ҵ����򷵻�-1
    FTLASSERT(FALSE);
    return -1;
}

//�������Ϣ������ �������Ƿ񵥻��� check box ������?
//void CLogItemView::OnClickList(NMHDR* pNMHDR, LRESULT* pResult){   ... 
//     hitinfo.pt = pNMListView->ptAction;
//     int item = m_list.HitTest(&hitinfo);
//     if( (hitinfo.flags & LVHT_ONITEMSTATEICON) != 0){ ... } 
//}

BOOL CLogItemView::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    //���ֱ��ʹ��CListCtrl��Ӧ���ڶԻ�������Ӧ��������Ϣ��
    //���ʹ��CListCtrl�����࣬����������������Ӧ��������Ϣ�ķ�����Ϣ
    //  ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
    if(message == WM_NOTIFY)
    {
        NMHDR* phdr = (NMHDR*)lParam;

        // these 3 notifications are only sent by virtual listviews
        switch(phdr->code)
        {
        case LVN_GETDISPINFO:   //��Ҫ���ݵ�ʱ��
            {
                NMLVDISPINFO* pLvdi;
                pLvdi = (NMLVDISPINFO*)lParam;
                GetDispInfo(&pLvdi->item);
            }
            if(pResult != NULL)
            {
                *pResult = 0;
            }
            break;
        case LVN_ODCACHEHINT:   //������ȡ����������(���DB������ȱȽ����ĵط���ȡ����ʱ)
            {
                NMLVCACHEHINT* pHint = (NMLVCACHEHINT*)lParam;
                PrepareCache(pHint->iFrom, pHint->iTo);
            }
            if(pResult != NULL)
            {
                *pResult = 0;
            }
            break;
        case LVN_ODFINDITEM:    //�û���ͼ����ĳ��Ԫ�ص�ʱ��(����Դ�������� ֱ��������ĸ��λ)
            {
                NMLVFINDITEM* pFindItem = (NMLVFINDITEM*)lParam;
                int i = FindItem(pFindItem->iStart, &pFindItem->lvfi);
                if(pResult != NULL)
                {
                    *pResult = i;
                }
            }
            break;
        default:
            return CListView::OnChildNotify(message, wParam, lParam, pResult);
        }
    }
    else
    {
        return CListView::OnChildNotify(message, wParam, lParam, pResult);
    }
    return TRUE;

}
void CLogItemView::OnUpdate(CView* pSender, LPARAM /*lHint*/, CObject* /*pHint*/)
{
    //if (pSender != this)
    {
        CLogViewerDoc* pDoc = GetDocument();
        LONG lLogItemCount = pDoc->m_FTLogManager.GetDisplayLogItemCount();
        CListCtrl& ListCtrl = GetListCtrl();
        ListCtrl.SetItemCount(lLogItemCount);
        FTLTRACE(TEXT("CLogItemView::OnUpdate, logItemCount=%ld"), lLogItemCount);
    }
}

void CLogItemView::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    UNREFERENCED_PARAMETER(pResult);

    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    UNREFERENCED_PARAMETER(pNMListView);

    TRACE(TEXT("click at iItem = %d,iSubItem=%d\n"), pNMListView->iItem, pNMListView->iSubItem);
}

void CLogItemView::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
    BOOL bRet = FALSE;
    //LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
//     CString strFormat;
//     strFormat.Format(TEXT("iItem = %d,iSubItem=%d"),
//         pNMListView->iItem, pNMListView->iSubItem);
//     AfxGetMainWnd()->SetWindowText(strFormat);
    if (pNMListView->iItem >= 0)
    {
        CString strFileName;
        int line = 0;
        CLogManager& rLogManager = GetDocument()->m_FTLogManager;
        LogItemPointer pLogItem = rLogManager.GetDisplayLogItem(pNMListView->iItem);

        if (pLogItem->pszSrcFileName != NULL && pLogItem->srcFileline != 0)
        {
            strFileName = pLogItem->pszSrcFileName;
            line = pLogItem->srcFileline;
        }
        else
        {
            CString strTraceInfo(pLogItem->pszTraceInfo);

            int leftBraPos = strTraceInfo.Find(TEXT('(')); //������
            int rightBraPos = strTraceInfo.Find(TEXT(')'));//������
            if (rightBraPos != -1)
            {
                int semPos = strTraceInfo.Find(TEXT(':'),rightBraPos);
                if (-1 != semPos)  //�ҵ��ֺţ����ܴ����ļ�����·��
                {
                    strTraceInfo.SetAt(semPos,TEXT('\0'));
                    if (-1 != leftBraPos && -1 != rightBraPos && rightBraPos > leftBraPos)  
                    {
                        CString strLine = strTraceInfo.Mid(leftBraPos + 1, rightBraPos - leftBraPos - 1);
                        line = _ttoi(strLine);

                        strFileName = strTraceInfo.Left(leftBraPos);
                        rLogManager.TryReparseRealFileName(strFileName);
                    }
                }
            }
        }
        if (!strFileName.IsEmpty() && line != 0)
        {
            TCHAR szPathFull[MAX_PATH] = {0};
            if (PathIsRelative(strFileName))
            {
                if (rLogManager.NeedScanSourceFiles())
                {
                    CString strExistSourceDir = AfxGetApp()->GetProfileString(SECTION_CONFIG, ENTRY_SOURCE_DIR);

                    CFDirBrowser dirBrowser(TEXT("Choose Project Source Root Path"), m_hWnd, strExistSourceDir);
                    if (dirBrowser.DoModal())
                    {
                        CString strFolderPath = dirBrowser.GetSelectPath();
                        AfxGetApp()->WriteProfileString(SECTION_CONFIG, ENTRY_SOURCE_DIR, strFolderPath);
                        rLogManager.ScanSourceFiles(strFolderPath);
                    }
                }

                SameNameFilePathListPtr spFilePathList = rLogManager.FindFileFullPath(ATLPath::FindFileName(strFileName));
                if (spFilePathList != nullptr)
                {
                    int nSameFileNameCount = spFilePathList->size();
                    if (nSameFileNameCount > 1)
                    {
                        FTLTRACEEX(FTL::tlWarn, TEXT("find %d source files with %s"), nSameFileNameCount, strFileName);
                    }
                    //TODO: if there are more than one file with same name, then prompt user choose
                    StringCchCopy(szPathFull, _countof(szPathFull), spFilePathList->front());
                }
            }
            else
            {
                StringCchCopy(szPathFull, _countof(szPathFull), strFileName);
            }
            CString path(szPathFull);
            if (!path.IsEmpty())
            {
                path.Replace(_T('/'), _T('\\'));
                API_VERIFY(GetDocument()->GoToLineInSourceCode(path, line));
            }
        }
     }
    *pResult = 0;
}

void CLogItemView::OnContextMenu(CWnd* pWnd, CPoint point)
{
    //if(KEY_DOWN(VK_CONTROL))
    {
        CMenu menuDetails;
        BOOL bRet = FALSE;
        API_VERIFY(menuDetails.LoadMenu(IDR_MENU_DETAIL));
        CMenu* pDetailMenu = menuDetails.GetSubMenu(0);
        FTLASSERT(pDetailMenu);
        m_ptContextMenuClick = point;
#if ENABLE_COPY_FULL_LOG
        pDetailMenu->EnableMenuItem(ID_DETAILS_COPY_FULL_LOG, MF_ENABLED | MF_BYCOMMAND);
#else 
        pDetailMenu->EnableMenuItem(ID_DETAILS_COPY_FULL_LOG, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
#endif
        API_VERIFY(pDetailMenu->TrackPopupMenu(TPM_TOPALIGN|TPM_LEFTBUTTON,point.x,point.y,pWnd));
    }
}

void CLogItemView::OnDetailsHighLightSameThread()
{
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
    if (pos != NULL)
    {
        int nItem = ListCtrl.GetNextSelectedItem(pos);
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
        if (pLogItem)
        {
            _HighlightSameThread(pLogItem);
        }
    }
}

LVHITTESTINFO CLogItemView::GetCurrentSelectInfo()
{
    //CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    LVHITTESTINFO lvHistTestInfo = {0};
    lvHistTestInfo.pt = m_ptContextMenuClick;
    ListCtrl.ScreenToClient(&lvHistTestInfo.pt);
    ListCtrl.SubItemHitTest(&lvHistTestInfo);
    //ListCtrl.HitTest(&lvHistTestInfo);
    
    return lvHistTestInfo;
}
void CLogItemView::OnDetailsCopyItemText()
{
    BOOL bRet = FALSE;
    CString strText;

    CListCtrl& listCtrl = GetListCtrl();
    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
    if (lvHistTestInfo.iItem != -1 && lvHistTestInfo.iSubItem != -1)
    {
        strText = listCtrl.GetItemText(lvHistTestInfo.iItem, lvHistTestInfo.iSubItem);
        if(!strText.IsEmpty())
        {
            API_VERIFY(CFSystemUtil::CopyTextToClipboard(strText)); //, m_hWnd);
        }
    }
    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR, 
            TEXT("pos=[%d,%d], text=%s, Last Error=%d"), 
            lvHistTestInfo.iItem, lvHistTestInfo.iSubItem, strText, GetLastError());
    }
}

void CLogItemView::OnDetailsCopyLineText()
{
    BOOL bRet = FALSE;
    CString strLineText, strItemText;

    CListCtrl& listCtrl = GetListCtrl();
    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
    if (lvHistTestInfo.iItem != -1)
    {
        int columnCount = _countof(columnInfos);
        for (int i = 0; i < columnCount; i++)
        {
            strItemText = listCtrl.GetItemText(lvHistTestInfo.iItem, i);
            strLineText += strItemText;
            if (i != columnCount - 1)
            {
                strLineText += TEXT(",");
            }
        }
        API_VERIFY(CFSystemUtil::CopyTextToClipboard(strLineText)); //, m_hWnd);
    }
    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR, 
            TEXT("pos=[%d,%d], text=%s, Last Error=%d"), 
            lvHistTestInfo.iItem, lvHistTestInfo.iSubItem, strLineText, GetLastError());
    }
}

void CLogItemView::OnDetailsCopyFullLog()
{
#if ENABLE_COPY_FULL_LOG
    BOOL bRet = FALSE;
    CString strText;

    //CListCtrl& listCtrl = GetListCtrl();
    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
    if (lvHistTestInfo.iItem != -1)
    {
        const LogItemPointer pLogItem = GetDocument()->m_FTLogManager.GetDisplayLogItem(lvHistTestInfo.iItem);
        if (pLogItem && pLogItem->pszFullLog != NULL)
        {
            API_VERIFY(CFSystemUtil::CopyTextToClipboard(pLogItem->pszFullLog)); //, m_hWnd);
        }
    }
    if (!bRet)
    {
        FormatMessageBox(m_hWnd, TEXT("CopyText Error"), MB_OK | MB_ICONERROR, 
            TEXT("pos=[%d,%d], text=%s, Last Error=%d"), 
            lvHistTestInfo.iItem, lvHistTestInfo.iSubItem, strText, GetLastError());
    }
#endif
}

void CLogItemView::OnDetailDeleteSelectItems() {
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    int nSelectedCount = ListCtrl.GetSelectedCount();
    if (nSelectedCount <= 0)
    {
        return;
    }

    if (nSelectedCount > 1)
    {
        if (FTL::FormatMessageBox(m_hWnd, TEXT("Confirm"), MB_OKCANCEL
            , TEXT("Do you want delete %d log items"), nSelectedCount) != IDOK)
        {
            return;
        }
    }

    int nSelectItem = -1;
    std::set<LONG> delItemsSeqNum;
    std::list<INT> delItemsIndex;
    POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
    while (pos != NULL)
    {
        nSelectItem = ListCtrl.GetNextSelectedItem(pos);
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(nSelectItem);
        if (pLogItem)
        {
            delItemsSeqNum.insert(pLogItem->seqNum);
            delItemsIndex.push_back(nSelectItem);
            //TRACE("will delete seq: %ld\n", pLogItem->seqNum);
        }
    }
    if (!delItemsSeqNum.empty())
    {
        logManager.DeleteItems(delItemsSeqNum);
        
        //clear select(is there better way?)
        std::list<INT>::iterator iter = delItemsIndex.begin();
        for ( iter++;  //skip the first select
            iter != delItemsIndex.end(); ++iter)
        {
            ListCtrl.SetItemState(*iter, 0, LVIS_SELECTED | LVIS_FOCUSED);
        }
        Invalidate();
    }
}

void CLogItemView::_HighlightSameThread(LogItemPointer pCompareLogItem)
{
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    int nTotalCount = ListCtrl.GetItemCount();
    for (int nItem = 0; nItem < nTotalCount; nItem++)
    {
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
        FTLASSERT(pLogItem);
        if (pLogItem) {
            if (pLogItem->machine == pCompareLogItem->machine
                && pLogItem->processId == pCompareLogItem->processId
                && pLogItem->threadId == pCompareLogItem->threadId){
                ListCtrl.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
                ListCtrl.SetSelectionMark(nItem);
            }
        }
    }
}

int CLogItemView::_GetSelectedIdTypeValue(MachinePIdTIdType& idType) {
    CLogManager& logManager = GetDocument()->m_FTLogManager;

    LVHITTESTINFO lvHistTestInfo = GetCurrentSelectInfo();
    if (lvHistTestInfo.iItem != -1)
    {
        const LogItemPointer pLogItem = logManager.GetDisplayLogItem(lvHistTestInfo.iItem);
        if (pLogItem) {
            idType.machine = pLogItem->machine;
            idType.pid = pLogItem->processId;
            idType.tid = pLogItem->threadId;
        }
    }
    return lvHistTestInfo.iItem;
}

void CLogItemView::OnDetailSelectCurrentPid() {
    int oldSelectIndex = 0;
    MachinePIdTIdType selectedIdType;

    if(-1 != (oldSelectIndex = _GetSelectedIdTypeValue(selectedIdType))) {
        CLogManager& logManager = GetDocument()->m_FTLogManager;

        logManager.OnlySelectSpecialItems(selectedIdType, ONLY_SELECT_TYPE::ostProcessId);
        GetDocument()->UpdateAllViews(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_PID, (CObject*)&selectedIdType);
        OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_PID, (CObject*)&selectedIdType);
        Invalidate();
    }
}

void CLogItemView::OnDetailSelectCurrentTid() {
    int oldSelectIndex = 0;
    MachinePIdTIdType selectedIdType;
    if (-1 != (oldSelectIndex = _GetSelectedIdTypeValue(selectedIdType))) {
        CLogManager& logManager = GetDocument()->m_FTLogManager;

        logManager.OnlySelectSpecialItems(selectedIdType, ONLY_SELECT_TYPE::ostThreadId);
        GetDocument()->UpdateAllViews(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_TID, (CObject*)&selectedIdType);
        OnUpdate(this, VIEW_UPDATE_HINT_FILTER_BY_CHOOSE_TID, (CObject*)&selectedIdType);
        Invalidate();
    }
}

BOOL CLogItemView::OnEraseBkgnd(CDC* pDC)
{
//������˸
   return __super::OnEraseBkgnd(pDC);
   //return TRUE;
}


//void CLogItemView::OnPaint()
//{
//    //��ӦWM_PAINT��Ϣ
//    CPaintDC dc(this); // device context for painting
//    CRect rect;
//    CRect headerRect;
//    CDC MenDC;//�ڴ�ID��  
//    CBitmap MemMap;
//    GetClientRect(&rect);   
//    GetDlgItem(0)->GetWindowRect(&headerRect);  
//    MenDC.CreateCompatibleDC(&dc);  
//    MemMap.CreateCompatibleBitmap(&dc,rect.Width(),rect.Height());
//    MenDC.SelectObject(&MemMap);
//    MenDC.FillSolidRect(&rect,RGB(228,236,243));  
//
//    //��һ���ǵ���Ĭ�ϵ�OnPaint(),��ͼ�λ����ڴ�DC����  
//    DefWindowProc(WM_PAINT,(WPARAM)MenDC.m_hDC,(LPARAM)0);      
//    //���  
//    dc.BitBlt(0,headerRect.Height(),rect.Width(),  rect.Height(),&MenDC,0, headerRect.Height(),SRCCOPY);  
//    MenDC.DeleteDC();
//    MemMap.DeleteObject();
//}


void CLogItemView::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;
    ATLTRACE(TEXT("OnLvnItemchanged, iItem=%d,iSubItem=%d, uNewState=%d, uOldState=%d\n"), 
        pNMLV->iItem, pNMLV->iSubItem, pNMLV->uNewState, pNMLV->uOldState);

    //����Ӧ����
    if (pNMLV->iItem >= 0 
        && ((pNMLV->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))!= 0) )
    {
        GetDocument()->m_FTLogManager.setActiveItemIndex(pNMLV->iItem);
        GetDocument()->UpdateAllViews(this);
    }
}

void CLogItemView::OnUpdateIndicatorSelectedLogItem(CCmdUI *pCmdUI)
{
    CLogManager& logManager = GetDocument()->m_FTLogManager;
    CListCtrl& ListCtrl = GetListCtrl();

    int nFirstSeqNum = 0;
    int nSelectedCount = 0;
    POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
    if (pos != NULL)
    {
        int nItem = ListCtrl.GetNextSelectedItem(pos);
        LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
        if (pLogItem)
        {
            nFirstSeqNum = pLogItem->seqNum;
        }
    }
    nSelectedCount = ListCtrl.GetSelectedCount();

    CString strFormat;
    strFormat.Format(ID_INDICATOR_SELECTED_LOGITEM, nFirstSeqNum, nSelectedCount);
    pCmdUI->SetText(strFormat);
}

BOOL CLogItemView::OnEditGoTo(UINT nID)
{
    BOOL bRet = TRUE;
    CDialogGoTo dlg(m_nLastGotToSeqNumber, this);
    if (dlg.DoModal())
    {
        CLogManager& logManager = GetDocument()->m_FTLogManager;
        UINT nGotoSeqNumber = dlg.GetGotoSeqNumber();
        if (nGotoSeqNumber >= 0 && nGotoSeqNumber < logManager.GetTotalLogItemCount())
        {
            m_nLastGotToSeqNumber = nGotoSeqNumber;

            CListCtrl& ListCtrl = GetListCtrl();

            POSITION pos = ListCtrl.GetFirstSelectedItemPosition();
            while (pos != NULL)
            {
                int oldSelectItem = ListCtrl.GetNextSelectedItem(pos);
                ListCtrl.SetItemState(oldSelectItem, 0, LVIS_SELECTED);
            }
            Invalidate();

            int nClosestItem = 0, nClosestItemSeqNum = 0;

            //ѡ���û�ָ����
            SortContent sortContent = logManager.GetFirstSortContent();
            if (type_Sequence == sortContent.contentType && sortContent.bSortAscending
                && logManager.GetTotalLogItemCount() == logManager.GetDisplayLogItemCount()  //no filter
                )
            {
                nClosestItem = nGotoSeqNumber - 1; //listCtrl ���� 0 ��ַ��
                nClosestItemSeqNum = nGotoSeqNumber;
            }
            else {
                //��Ҫ����
                int nTotalCount = ListCtrl.GetItemCount();
                for (int nItem = 0; nItem < nTotalCount; nItem++)
                {
                    LogItemPointer pLogItem = logManager.GetDisplayLogItem(nItem);
                    FTLASSERT(pLogItem);
                    if (pLogItem) {
                        if (pLogItem->seqNum == nGotoSeqNumber) {
                            //׼ȷ�ҵ�
                            nClosestItem = nItem;
                            nClosestItemSeqNum = nGotoSeqNumber;
                            break;
                        }

                        LONG diff1 = FTL_ABS((LONG)nGotoSeqNumber - pLogItem->seqNum);
                        LONG diff2 = FTL_ABS((LONG)nGotoSeqNumber - nClosestItemSeqNum);
                        if ( diff1 <= diff2)
                        {
                            nClosestItem = nItem;
                            nClosestItemSeqNum = pLogItem->seqNum;
                        }
                    }
                }
            }

            FTLTRACE(TEXT("try got %d, real %d, item index=%d"), 
                nGotoSeqNumber, nClosestItemSeqNum, nClosestItem);

            API_VERIFY(ListCtrl.EnsureVisible(nClosestItem, TRUE));
            ListCtrl.SetItemState(nClosestItem, LVIS_SELECTED, LVIS_SELECTED);
            ListCtrl.SetSelectionMark(nClosestItem);

        }
    }
    return bRet;
}
