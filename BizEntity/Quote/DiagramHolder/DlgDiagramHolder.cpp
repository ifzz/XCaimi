// DlgDiagramHolder.cpp : 实现文件
//

#include "stdafx.h"
#include "DiagramHolder.h"
#include "DlgDiagramHolder.h"
#include "WinnerApplication.h"
#include "..\..\..\HelperLib\GeneralHelper.h"
#include "KeyBoardStruct.h"

// CDlgDiagramHolder 对话框

IMPLEMENT_DYNAMIC(CDlgDiagramHolder, CHSBizDialog)

CDlgDiagramHolder::CDlgDiagramHolder(CWnd* pParent /*=NULL*/)
	: CHSBizDialog(CDlgDiagramHolder::IDD, pParent)
{
	m_pDiagramView = NULL;
	m_bFixedMode = FALSE;
}

CDlgDiagramHolder::~CDlgDiagramHolder()
{
	UnregisterLinkageMsg(QUOTE_MESSAGE_CODE_CHANGE);
	UnregisterLinkageMsg(QUOTE_MESSAGE_NOTIFY_DIAGRAM_LAYOUT);
	UnregisterLinkageMsg(QUOTE_MESSAGE_CODE_PAGE_UP_DOWN);
}

BOOL CDlgDiagramHolder::HSDataSourceSink_OnCommNotify(void* pData)
{
	if(m_pDiagramView)
		return m_pDiagramView->HSDataSourceSink_OnCommNotify(pData);
	return FALSE;
}

BOOL CDlgDiagramHolder::HSDataSourceSink_OnRecvData(void* pData, int nLen)
{
	if(m_pDiagramView)
		return m_pDiagramView->HSDataSourceSink_OnRecvData(pData,nLen);
	return FALSE;
}

BOOL CDlgDiagramHolder::OnInit()
{
	RegisterLinkageMsg(QUOTE_MESSAGE_CODE_CHANGE);
	RegisterLinkageMsg(QUOTE_MESSAGE_NOTIFY_DIAGRAM_LAYOUT);
	RegisterLinkageMsg(QUOTE_MESSAGE_CODE_PAGE_UP_DOWN);

	m_bAutoKeyboard = TRUE;
	m_pDiagramView = new CDiagramView();
	if( !m_pDiagramView->CreateView(this) )
	{
		delete m_pDiagramView;
		m_pDiagramView = NULL;
		return FALSE;
	}

	//获取数据引擎接口指针
	m_pDataSource = (IDataSourceEx*)CWinnerApplication::GetObject(OBJ_HSDATAENGINE);
	if (m_pDataSource && !IsBadReadPtr(m_pDataSource,1))
	{
		m_nDataSourceID = m_pDataSource->HSDataSource_DataSouceInit(this,NULL);
	}

	m_pDiagramView->SetDataSource(m_pDataSource,m_nDataSourceID);

 	m_strFixedLayout = m_pParam->GetStringParam("fixed_layout");
	if(!m_strFixedLayout.IsEmpty())
	{
		m_bFixedMode = TRUE;
		StockUserInfo emptyCode;
		emptyCode.SetEmpty();
		m_pDiagramView->ChangeDiagramLayer(m_strFixedLayout,&emptyCode);
		//m_pDiagramView->OpenFixedLayout(m_strFixedLayout);
	}
	else
	{
		//默认打开报价图层
		StockUserInfo emptyCode;
		emptyCode.SetEmpty();
		m_pDiagramView->ChangeDiagramLayer(Layout_QuoteReport,&emptyCode);
		m_pDiagramView->GetRequestData(NULL);
	}

	CWinnerApplication::FormatLog(INFO_LOG_LEVEL,DiagramHolderLoggerName,"[%s|%s-%d]图元容器，数据引擎ID = %d",
		__FUNCTION__,__FILE__,__LINE__,m_nDataSourceID);
	return TRUE;

}

BOOL CDlgDiagramHolder::OnOpenKeyboard(int nCode)
{
	if (m_pDiagramView->IsActiveDiagramLayer(Layout_MiniQuote))
	{// 交易五档行情 要过滤掉键盘精灵
		return TRUE;
	}
	
	CRect rc;
	GetClientRect(&rc);
	ClientToScreen(&rc);
	HSShowKeyboardInfo keyboardinfo;
	keyboardinfo.m_cStyle = HSShowKeyboardInfo::bottom | HSShowKeyboardInfo::right;
	keyboardinfo.m_oRect = rc;
	

	if(m_pDiagramView->IsActiveDiagramLayer(Layout_QuoteReport))
	{
		keyboardinfo.m_nPageType = PageInfo_Quote;
	}
	else if(m_pDiagramView->IsActiveDiagramLayer(Layout_TechAnalysis)) 
	{
		keyboardinfo.m_nPageType = PageInfo_Tech;
	}
	else
	{

	}

	if(m_pKeyboard == NULL)
		m_pKeyboard = (IKeyboard*)CWinnerApplication::GetObject(OBJ_KEYBOARD);
	if(m_pKeyboard == NULL)
		return FALSE;

	m_pKeyboard->ShowKeyboard(nCode,&keyboardinfo,this->m_hWnd);

	return TRUE;
}

BOOL CDlgDiagramHolder::OnKeyBoardNotify(WPARAM wParam, LPARAM lParam)
{
	if( m_pDiagramView->OnKeyBoardNotify(wParam,lParam) )
		return TRUE;
	else
		return FALSE;
}

void CDlgDiagramHolder::OnKeyBoardCodeNotify(StockUserInfo* pStkInfo)
{
	if(IsFixedMode())
		return;

	if(pStkInfo == NULL)
		return;

	if(  m_pDiagramView->IsActiveDiagramLayer(Layout_F10Info) || 
	     m_pDiagramView->IsActiveDiagramLayer(Layout_DealDetail) ||
		 m_pDiagramView->IsActiveDiagramLayer(Layout_PriceBook)  )
	{
		
	}
	else
	{
		m_pDiagramView->ChangeDiagramLayer(Layout_TechAnalysis,pStkInfo);
	}
	m_pDiagramView->SetNeighbourStockInfo(pStkInfo);
	m_pDiagramView->GetRequestData(pStkInfo);

	m_pDiagramView->SetFocus();
}

void CDlgDiagramHolder::OnLinkageMsg(CLinkageParam* pParam)
{
	if(!IsShow())
		return;

	CWndUpdateLocker(this->m_pDiagramView);

	if(pParam->m_nMsg == QUOTE_MESSAGE_CODE_CHANGE)
	{
		OnMsgCodeChanged(pParam);
	}
	else if( pParam->m_nMsg == QUOTE_MESSAGE_NOTIFY_DIAGRAM_LAYOUT)
	{
		OnMsgNotifyDiagramLayout(pParam);
	}
	else if( pParam->m_nMsg == QUOTE_MESSAGE_CODE_PAGE_UP_DOWN)
	{
		OnMsgCodePageUpDown(pParam);
	}
	
}

void CDlgDiagramHolder::OnMsgCodePageUpDown( CLinkageParam* pParam )
{
	CString strDirection = pParam->GetStringField(PARAM_CODE_PAGE_UP_DOWN_DIRECTION);
	if(strDirection == PARAM_CODE_PAGE_UP_DOWN_DIRECTION_UP)
	{
		m_pDiagramView->OnPrior();
	}
	else
	{
		m_pDiagramView->OnNext();
	}
}

void CDlgDiagramHolder::OnMsgCodeChanged( CLinkageParam* pParam )
{
	CString strMarketType = pParam->GetStringField(PARAM_CODE_CHANGE_MARK_TYPE);
	CString strStockCode = pParam->GetStringField(PARAM_CODE_CHANGE_STOCK_CODE);

	CodeInfo code;
	code.m_cCodeType = atoi(strMarketType);
	strncpy(code.m_cCode,strStockCode,6);
	StockUserInfo stockUserInfo;

	BOOL bFind = m_pDataSource->HSDataSourceEx_GetStockUserInfo(&code,stockUserInfo);
	StockUserInfo* pStock = bFind ? &stockUserInfo: NULL;
	if(!IsFixedMode())
	{
		if (bFind)
		{
			m_pDiagramView->ChangeDiagramLayer(Layout_TechAnalysis, pStock);
			m_pDiagramView->SetNeighbourStockInfo(pStock);
			m_pDiagramView->GetRequestData(pStock);
		}	
	}
	else if(m_pDiagramView->IsActiveDiagramLayer(Layout_MiniQuote))
	{
		int nMsgFrome = pParam->GetIntegerField(PARAM_CODE_CHANGE_MSG_FROM);
		if(nMsgFrome == atoi(PARAM_CODE_CHANGE_MSG_FROM_TRADE) )
		{
			m_pDiagramView->GetRequestData(pStock);
		}
	}
	else
	   m_pDiagramView->GetRequestData(pStock);
}

void CDlgDiagramHolder::OnMsgNotifyDiagramLayout( CLinkageParam* pParam )
{
	//if(IsFixedMode())
	//	return;

	CWndUpdateLocker(this->m_pDiagramView);

	CDrawBaseParam drawparam;
	CString strDiagramMsgID = pParam->GetStringField(PARAM_DIAGRAMMSGID);
	CString strDiagramLayout = pParam->GetStringField(PARAM_DIAGRAM_LAYOUT_NAME);
	int nDiagramMsgId = atoi(strDiagramMsgID);

	CString strMarketType = pParam->GetStringField(Param_Open_MarketType);
	CString strStockCode = pParam->GetStringField(Param_Open_StockCode);
	CodeInfo openCode;
	openCode.m_cCodeType = atoi(strMarketType);
	strncpy(openCode.m_cCode,strStockCode,6);

	//当代码不空时，根据代码打开指定图层
	CDiagramLayer* pLayer = NULL;
	if(!openCode.GetCode().IsEmpty())
	{
		StockUserInfo stockUserInfo;
		if( !m_pDataSource->HSDataSourceEx_GetStockUserInfo(&openCode,stockUserInfo) )
			return;
		pLayer = m_pDiagramView->ChangeDiagramLayer(strDiagramLayout,&stockUserInfo);
		m_pDiagramView->SetNeighbourStockInfo(&stockUserInfo);
		m_pDiagramView->GetRequestData(&stockUserInfo);
	}
	else if(!strDiagramLayout.IsEmpty())
	{
		pLayer = m_pDiagramView->ChangeDiagramLayer(strDiagramLayout,NULL);
	}
	else
	{
		pLayer = m_pDiagramView->GetCurrentLayout();
	}

	if(pLayer == NULL)
		return;

	POSITION pos;
	CString strParamName,strValue;
	for( pos = pParam->GetStartPosition(); pos != NULL; )
	{
		pParam->GetNextAssoc( pos, strParamName, strValue);
		drawparam.AddParam(strParamName,strValue);
	}
	if(pLayer)
	{
		int nMsgId = atoi(strDiagramMsgID);
		pLayer->SendNotifyToDrawBase(nMsgId,&drawparam);
	}
}

void CDlgDiagramHolder::OnExternalChangeContent(IHSParam* pParam)
{
	CString strMarketType = pParam->GetStringParam("mark_type");
	CString strStockCode = pParam->GetStringParam("stock_code");

	CodeInfo code;
	code.m_cCodeType = atoi(strMarketType);
	strncpy(code.m_cCode,strStockCode,6);
	StockUserInfo stockUserInfo;
	if( !m_pDataSource->HSDataSourceEx_GetStockUserInfo(&code,stockUserInfo) )
		return;

	if(m_strFixedLayout.IsEmpty())
	{
		m_pDiagramView->ChangeDiagramLayer(Layout_TechAnalysis,&stockUserInfo);
		m_pDiagramView->SetNeighbourStockInfo(&stockUserInfo);
	}
	
	m_pDiagramView->GetRequestData(&stockUserInfo);

	m_pDiagramView->SetFocus();
}


BEGIN_MESSAGE_MAP(CDlgDiagramHolder, CHSBizDialog)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CDlgDiagramHolder 消息处理程序

void CDlgDiagramHolder::OnSize(UINT nType, int cx, int cy)
{
	if (GetSafeHwnd() == NULL || m_pDiagramView == NULL) 
		return;

	CRect rectClient;
	GetClientRect(rectClient);

	m_pDiagramView->SetWindowPos (NULL, -1, -1, rectClient.Width(), rectClient.Height(), 
		SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}


BOOL CDlgDiagramHolder::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}



BOOL CDlgDiagramHolder::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return __super::OnSetCursor(pWnd, nHitTest, message);
}
