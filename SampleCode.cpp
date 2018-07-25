/******** Notes for sample code **************
 * 1. MUST implement BC28 wrapper functions.
 * 2. Call BC28_PushReceivedByte() in ReceiveThread() to handle received data via UART.
 * 3. Call BC28_Init() in OnBnClickedButtonOpen() after UART is ready.
 * 4. Sample code to send AT command in OnBnClickedButtonSendAt().
 * 5. Sample code to connect MQTT broker in OnBnClickedButtonConnectMqtt().
 * 6. Sample code to publish message in OnBnClickedButtonPublish().
 * 7. Sample code to subscribe topic in OnBnClickedButtonSubscribe().
 * 8. Need to call BC28_SetSocketListener() after subscribing successfully to handle incoming messages.
 *********************************************/

#include "stdafx.h"
#include "SimWRL8500.h"
#include "SimWRL8500Dlg.h"
#include "MQTT/MQTT.h"
#include "BC28/BC28.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSimWRL8500Dlg *g_pInstDlg = NULL;

/**
  * BC28 wrapper functions
  */
void *BC28_Wrap_Memory_Alloc(uint32_t size)
{
	return new BYTE[size];
}

void BC28_Wrap_Memory_Free(void *ptr)
{
	delete [] ptr;
}

void BC28_Wrap_Sleep(int ms)
{
	::Sleep(ms);
}

int BC28_Wrap_Send(const uint8_t *data, int size)
{
	DWORD num = 0;

	if(g_pInstDlg != NULL && g_pInstDlg->m_flagConnectComm)
	{
		WriteFile(g_pInstDlg->m_hComm, data, size, &num, NULL);
	}

	return (int)num;
}

typedef struct tagBC28_TASK_PARAM {
	BC28_TASK task;
	int param1;
	int param2;
} BC28_TASK_PARAM;

//static BC28_TASK_PARAM paramBC28Task;

UINT BC28TaskThread(PVOID arg)
{
	BC28_TASK_PARAM * param = (BC28_TASK_PARAM *)arg;

	param->task(param->param1, param->param2);

	delete param;

	return 0;
}

void BC28_Wrap_PostTask(BC28_TASK task, int param1, int param2)
{
	BC28_TASK_PARAM *paramBC28Task = new BC28_TASK_PARAM;

	paramBC28Task->task = task;
	paramBC28Task->param1 = param1;
	paramBC28Task->param2 = param2;

	::AfxBeginThread((AFX_THREADPROC)BC28TaskThread, paramBC28Task);
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CSimWRL8500Dlg dialog




CSimWRL8500Dlg::CSimWRL8500Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSimWRL8500Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSimWRL8500Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSimWRL8500Dlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CSimWRL8500Dlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_SEND_AT, &CSimWRL8500Dlg::OnBnClickedButtonSendAt)
	ON_LBN_SELCHANGE(IDC_LIST_DEBUG, &CSimWRL8500Dlg::OnLbnSelchangeListDebug)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT_MQTT, &CSimWRL8500Dlg::OnBnClickedButtonConnectMqtt)
	ON_BN_CLICKED(IDC_BUTTON_PUBLISH, &CSimWRL8500Dlg::OnBnClickedButtonPublish)
	ON_BN_CLICKED(IDC_BUTTON_SUBSCRIBE, &CSimWRL8500Dlg::OnBnClickedButtonSubscribe)
	ON_WM_TIMER()
END_MESSAGE_MAP()


void CSimWRL8500Dlg::DetectCOMPort(void)
{
#if 1
	CComboBox *pComPortBox = (CComboBox *)this->GetDlgItem(IDC_COMBO_COM);
	pComPortBox->ResetContent();

	TCHAR lpPath[512];
    DWORD queryReturn;
    int PortCount = 0; 
    
    for( int i = 0 ; i < 255 ; i++ ) // checking ports from COM0 to COM255
    {
        CString str;
        str.Format(_T("%d"),i);
        CString ComPort = CString("COM") + CString(str);
        
        queryReturn = QueryDosDevice( ComPort, (LPTSTR)lpPath, 512 );

        if( queryReturn != 0 ) 
        {
			pComPortBox->InsertString( PortCount, (CString)ComPort ); 
            PortCount++;
        }

        if( ::GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            lpPath[1024]; 
            continue;
        }

    }

	pComPortBox->SetCurSel(0);
#else
	CArray<SSerInfo,SSerInfo&> asi;
	EnumSerialPorts(asi,FALSE);
	const TCHAR str_name[USB_COM_NAME_NUM][256] = {
		_T("Communication Device Class ASF example")
	};

	//Do 2 times to find USB drive name
	for(int index = 0; index < USB_COM_NAME_NUM; index++)  // Chili Add 05/28/2008 
	{
		for (int ii=0; ii<asi.GetSize(); ii++)
		{
		    CString temp = asi[ii].strFriendlyName;
	  
	    	if (temp.Find(str_name[index]) != -1)
			{
				CString com;
		        int indexf = temp.Find('(');
		        int indexe = temp.Find(')');
		        com=temp.Mid(indexf+1, indexe-indexf-1);
		        *pstrCOM = com;
                return TRUE;
			}
		}
	}
#endif
}

// CSimWRL8500Dlg message handlers

BOOL CSimWRL8500Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	g_pInstDlg = this;

	DetectCOMPort();

	m_hComm = INVALID_HANDLE_VALUE;
	m_flagConnectComm = 0;

	m_flagConnectMQTT = 0;

	((CEdit*)GetDlgItem(IDC_EDIT_IP))->SetWindowText(_T(DEFAULT_MQTT_BROKER_IP));
	((CEdit*)GetDlgItem(IDC_EDIT_PORT))->SetWindowText(_T(DEFAULT_MQTT_BROKER_PORT));
	((CEdit*)GetDlgItem(IDC_EDIT_USER_NAME))->SetWindowText(_T(DEFAULT_MQTT_USER_NAME));
	((CEdit*)GetDlgItem(IDC_EDIT_PASSWD))->SetWindowText(_T(DEFAULT_MQTT_PASSWORD));
	((CEdit*)GetDlgItem(IDC_EDIT_CLIENT_ID))->SetWindowText(_T(DEFAULT_MQTT_CLIENT_ID));
	((CEdit*)GetDlgItem(IDC_EDIT_TOPIC))->SetWindowText(_T(DEFAULT_MQTT_PUBLISH_TOPIC));
	((CEdit*)GetDlgItem(IDC_EDIT_MESSAGE))->SetWindowText(_T(DEFAULT_MQTT_PUBLISH_MSG));
	((CEdit*)GetDlgItem(IDC_EDIT_TOPIC_SUB))->SetWindowText(_T(DEFAULT_MQTT_SUBSCRIBE_TOPIC));

	((CButton *)GetDlgItem(IDC_BUTTON_SEND_AT))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(FALSE);
	((CButton *)GetDlgItem(IDC_BUTTON_PUBLISH))->EnableWindow(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSimWRL8500Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSimWRL8500Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSimWRL8500Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSimWRL8500Dlg::AddStringToDebugList(CString msg)
{
	CListBox *pListBox = (CListBox *)GetDlgItem(IDC_LIST_DEBUG);

	pListBox->AddString(msg);
	pListBox->SetTopIndex(pListBox->GetCount() - 1);
}


void CSimWRL8500Dlg::PushReceivedByte(BYTE b)
{
	BC28_PushReceivedByte(b);
/*
	m_bufReceived[m_tailBuf++] = b;

	if(b == '\n' && m_tailBuf > 2)
	{
		m_bufReceived[m_tailBuf] = 0;

		TCHAR *bufMsg = new TCHAR[m_tailBuf];
		int count = 0;
		for(int i=0; i<m_tailBuf; i++)
		{
			if(m_bufReceived[i] != '\r' && m_bufReceived[i] != '\n')
				bufMsg[count++] = (TCHAR)m_bufReceived[i];
		}
		bufMsg[count] = 0;

		CString msg((LPCTSTR)bufMsg);
		AddStringToDebugList(msg);

		delete [] bufMsg;

		m_tailBuf = 0;
	}
*/	
/*
	int nBytes = 0;

	EnterCriticalSection(&m_csReceiveTask);

	m_bufReceiced[m_tailBuf++];
	if(m_tailBuf >= RECEIVED_BUF_SIZE)
		m_tailBuf = 0;
	if(m_tailBuf == m_headBuf)
		m_headBuf++;
	if(m_headBuf >= RECEIVED_BUF_SIZE)
		m_headBuf = 0;

	if(m_tailBuf >= m_headBuf)
		nBytes = m_tailBuf - m_headBuf;
	else
		nBytes = (RECEIVED_BUF_SIZE - m_headBuf) + m_tailBuf;

	LeaveCriticalSection(&m_csReceiveTask);
*/	
}

UINT ReceiveThread(PVOID arg)
{
	CSimWRL8500Dlg *pDlg = (CSimWRL8500Dlg*)arg;

	while(pDlg->m_flagConnectComm)
	{
		BYTE b;
		DWORD num = 0;

		while(pDlg->m_flagConnectComm && ReadFile(pDlg->m_hComm, &b, 1, &num, NULL) && num > 0)
		{
			pDlg->PushReceivedByte(b);
		}

		Sleep(10);
	}

	delete [] pDlg->m_bufReceived;
	pDlg->m_bufReceived = NULL;
	DeleteCriticalSection(&pDlg->m_csReceiveTask);

	return 0;
}


void CSimWRL8500Dlg::OnBnClickedButtonOpen()
{
	// TODO: Add your control notification handler code here
	if(m_flagConnectComm == 0)
	{
		// open comm port
		CComboBox *pComboBoxPort = (CComboBox *)this->GetDlgItem(IDC_COMBO_COM);
		CString szCommPort;

		 pComboBoxPort->GetLBText(pComboBoxPort->GetCurSel(), szCommPort);
 
		if(szCommPort.GetLength() > 4)
			szCommPort = CString(_T("\\\\.\\")) + szCommPort;

		// open com
		if(m_hComm != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hComm);
			m_hComm = INVALID_HANDLE_VALUE;
		}

		{
			DCB dcb;
			

			m_hComm = CreateFile(szCommPort,
					GENERIC_READ | GENERIC_WRITE,
					0,    /* comm devices must be opened w/exclusive-access */
					NULL, /* no security attrs */
					OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
					0,    /* not overlapped I/O */
					NULL  /* hTemplate must be NULL for comm devices */
			);

			if(m_hComm == INVALID_HANDLE_VALUE){
				::AfxMessageBox(_T("Fail to open Comm"));
				return;
			}
			GetCommState(m_hComm, &dcb);
			dcb.BaudRate = CBR_9600;
			dcb.ByteSize = 8;
			dcb.Parity = NOPARITY;
			dcb.StopBits = ONESTOPBIT;
			dcb.fBinary=TRUE;
			dcb.fDsrSensitivity=false;
			dcb.fOutX=false;
			dcb.fInX=false;
			dcb.fNull=false;
			dcb.fAbortOnError=FALSE;//TRUE;
			dcb.fOutxCtsFlow=FALSE;
			dcb.fOutxDsrFlow=false;
			dcb.fDtrControl=DTR_CONTROL_DISABLE;
			dcb.fDsrSensitivity=false;
			dcb.fRtsControl=RTS_CONTROL_DISABLE;
			dcb.fOutxCtsFlow=false;
			dcb.fOutxCtsFlow=false;

			SetCommState(m_hComm, &dcb);
			SetupComm(m_hComm, 0, 0);
			COMMTIMEOUTS timeouts={1,1,1,1,1};
			SetCommTimeouts(m_hComm, &timeouts);
		}

		m_bufReceived = new BYTE[RECEIVED_BUF_SIZE];
		m_headBuf = 0;
		m_tailBuf = 0;

		m_flagConnectComm = 1;

		InitializeCriticalSection(&m_csReceiveTask);

		m_pthreadReceive = ::AfxBeginThread((AFX_THREADPROC)ReceiveThread, this);

		if(BC28_Init() == 1)
		{
			CButton *pButton = (CButton *)GetDlgItem(IDC_BUTTON_OPEN);
			pButton->SetWindowText(_T("Close"));

			((CButton *)GetDlgItem(IDC_BUTTON_SEND_AT))->EnableWindow(TRUE);
			((CButton *)GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(TRUE);
			((CButton *)GetDlgItem(IDC_BUTTON_PUBLISH))->EnableWindow(FALSE);
		}
		else
		{
			m_flagConnectComm = 0;

			if(m_hComm != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hComm);
				m_hComm = INVALID_HANDLE_VALUE;
			}

			::AfxMessageBox(_T("Fail to communicate with BC28!"));
		}
	}
	else
	{
		((CButton *)GetDlgItem(IDC_BUTTON_OPEN))->EnableWindow(FALSE);

		if(m_flagConnectMQTT == 1)
		{
			CloseMQTT();
		}

		m_flagConnectComm = 0;

		if(m_hComm != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hComm);
			m_hComm = INVALID_HANDLE_VALUE;
		}

		CButton *pButton = (CButton *)GetDlgItem(IDC_BUTTON_OPEN);
		pButton->SetWindowText(_T("Open"));

		((CButton *)GetDlgItem(IDC_BUTTON_OPEN))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_BUTTON_SEND_AT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_BUTTON_PUBLISH))->EnableWindow(FALSE);
	}
}

void CSimWRL8500Dlg::OnBnClickedButtonSendAt()
{
	// TODO: Add your control notification handler code here
	CString szEdit;
	CEdit *pCmdEdit = (CEdit*)this->GetDlgItem(IDC_EDIT_AT_COMMAND);
	pCmdEdit->GetWindowText(szEdit);
	if(szEdit.GetLength() > 0 && szEdit.Left(2).CompareNoCase(_T("AT")) == 0)
	{
		AddStringToDebugList(szEdit);

		char at_cmd[1024], szRcv[1024];
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			at_cmd[i] = (BYTE)pBuf[i];
		}

		at_cmd[i++] = '\r';
		at_cmd[i++] = 0;

		//WriteFile(m_hComm, at_cmd, i, &num, NULL);
		if(BC28_SendATCmdWaitRcv(at_cmd, szRcv, 1024, 5000) != 0)
		{
			int str_len = strlen(szRcv);
			TCHAR *bufMsg = new TCHAR[str_len];
			int count = 0;
			for(int i=0; i<str_len; i++)
			{
				if(szRcv[i] != '\r' && szRcv[i] != '\n')
				{
					bufMsg[count++] = (TCHAR)szRcv[i];
				}
				else if(szRcv[i] == '\n')
				{
					bufMsg[count] = 0;

					CString msg((LPCTSTR)bufMsg);
					AddStringToDebugList(msg);

					count = 0;
				}
			}

			delete [] bufMsg;
		}
		else
		{
			CString szMsg(_T("Timeout!"));
			AddStringToDebugList(szMsg);
		}
	}
}

void CSimWRL8500Dlg::OnLbnSelchangeListDebug()
{
	// TODO: Add your control notification handler code here
}

void CSimWRL8500Dlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	if(m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}

	CDialog::OnClose();
}

UINT KeepAliveThread(PVOID arg)
{
	CSimWRL8500Dlg *pDlg = (CSimWRL8500Dlg*)arg;

	pDlg->m_lastConnectTick = ::GetTickCount();

	while(pDlg->m_flagConnectMQTT)
	{
		DWORD curTick = ::GetTickCount();

		if(curTick - pDlg->m_lastConnectTick > 50000)
		{
			BYTE buf[64];
			int len = MQTT_PingRequestMessage(buf, 64);
			if(BC28_WriteTcpSocket(pDlg->m_hSocket, buf, len) > 0)
			{
				pDlg->m_lastConnectTick = ::GetTickCount();

				//wait ack
				int times = 5000/500;
				int count = 0;
				while(count < 2 && times--)
				{
					::Sleep(500);
					count += BC28_ReadTcpSocket(pDlg->m_hSocket, &buf[count], 2 - count);
				}

				if(!(count > 0 && MQTT_GetMessageType(buf) == MQTT_MSG_TYPE_PINGRESP))
				{
					//no response, need to reconnect

				}
			}
		}

		::Sleep(500);
	}

	return 0;
}

UINT ConnectMqttThread(PVOID arg)
{
	CSimWRL8500Dlg *pDlg = (CSimWRL8500Dlg*)arg;

	char ip[16] = "", port[8] = "", user_name[128] = "", passwd[128] = "", client_id[128] = "";
	CString szEdit;
	BOOL isValid = TRUE;

	//IP
	((CEdit*)pDlg->GetDlgItem(IDC_EDIT_IP))->GetWindowText(szEdit);
	if(szEdit.GetLength() < 8 || szEdit.GetLength() > 15 || szEdit.Find('.') == -1)
	{
		isValid = FALSE;
	}
	else
	{
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			ip[i] = (BYTE)pBuf[i];
			if(!(ip[i] == '.' || (ip[i] >= '0' && ip[i] <= '9')))
			{
				isValid = FALSE;
				break;
			}
		}

		ip[i] = 0;
		isValid = TRUE;
	}

	if(!isValid)
	{
		::AfxMessageBox(_T("Invalid IP address!"));
		((CButton *)pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(TRUE);
		return 1;
	}

	//PORT
	((CEdit*)pDlg->GetDlgItem(IDC_EDIT_PORT))->GetWindowText(szEdit);
	if(szEdit.GetLength() < 1 || szEdit.GetLength() > 5)
	{
		isValid = FALSE;
	}
	else
	{
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			port[i] = (BYTE)pBuf[i];
			if(!(port[i] >= '0' && port[i] <= '9'))
			{
				isValid = FALSE;
				break;
			}
		}

		port[i] = 0;
		isValid = TRUE;
	}

	if(!isValid)
	{
		::AfxMessageBox(_T("Invalid Port Number!"));
		((CButton *)pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(TRUE);
		return 1;
	}

	//user name
	((CEdit*)pDlg->GetDlgItem(IDC_EDIT_USER_NAME))->GetWindowText(szEdit);
	if(szEdit.GetLength() == 0)
	{
		isValid = TRUE;
	}
	else if(szEdit.GetLength() < 1 || szEdit.GetLength() > 127)
	{
		isValid = FALSE;
	}
	else
	{
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			user_name[i] = (BYTE)pBuf[i];
			if(user_name[i] < ' ')
			{
				isValid = FALSE;
				break;
			}
		}

		user_name[i] = 0;
		isValid = TRUE;
	}

	if(!isValid)
	{
		::AfxMessageBox(_T("Invalid User Name!"));
		((CButton *)pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(TRUE);
		return 1;
	}

	//password
	((CEdit*)pDlg->GetDlgItem(IDC_EDIT_PASSWD))->GetWindowText(szEdit);
	if(szEdit.GetLength() == 0)
	{
		isValid = TRUE;
	}
	else if(szEdit.GetLength() < 1 || szEdit.GetLength() > 127)
	{
		isValid = FALSE;
	}
	else
	{
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			passwd[i] = (BYTE)pBuf[i];
			if(passwd[i] < ' ')
			{
				isValid = FALSE;
				break;
			}
		}

		passwd[i] = 0;
		isValid = TRUE;
	}

	if(!isValid)
	{
		::AfxMessageBox(_T("Invalid Password!"));
		((CButton *)pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(TRUE);
		return 1;
	}

	//client ID
	((CEdit*)pDlg->GetDlgItem(IDC_EDIT_CLIENT_ID))->GetWindowText(szEdit);
	if(szEdit.GetLength() == 0)
	{
		strcpy(client_id, BC28_GetIMSI());
		isValid = TRUE;
	}
	else if(szEdit.GetLength() < 1 || szEdit.GetLength() > 127)
	{
		isValid = FALSE;
	}
	else
	{
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			client_id[i] = (BYTE)pBuf[i];
			if(client_id[i] < ' ')
			{
				isValid = FALSE;
				break;
			}
		}

		client_id[i] = 0;
		isValid = TRUE;
	}

	if(!isValid)
	{
		::AfxMessageBox(_T("Invalid Client ID!"));
		((CButton *)pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(TRUE);
		return 1;
	}

	if(!BC28_WaitReady(5000))
	{
		::AfxMessageBox(_T("No Network!"));
		((CButton *)pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(TRUE);
		return 1;
	}

	strcpy(pDlg->m_serverIP, ip);
	strcpy(pDlg->m_serverPort, port);

	//compose message
	BYTE connect_msg[1024];
	int msg_size = MQTT_ConnectMessage(connect_msg, 1024, ip, port, client_id, user_name, passwd, 30, 60, 1);

	//open tcp socket
	if((pDlg->m_hSocket = BC28_OpenTcpSocket(ip, port)) == -1)
	{
		BC28_Reboot();
		goto flagConnectFailed;
	}

	//connect MQTT
	if(BC28_WriteTcpSocket(pDlg->m_hSocket, connect_msg, msg_size) > 0)
	{
		BYTE buf[8];
		int count = 0, times = 10000/200;

		while(count < MQTT_MSG_SIZE_CONNACK && times--)
		{
			::Sleep(200);
			count += BC28_ReadTcpSocket(pDlg->m_hSocket, &buf[count], MQTT_MSG_SIZE_CONNACK - count);
		}

		if(count > 2 && MQTT_CheckConnectAck(buf) != MQTT_CONNACK_ACCEPTED)
		{
			goto flagConnectFailed;
		}
	}
	else
	{
		goto flagConnectFailed;
	}

	pDlg->m_flagConnectMQTT = 1;

	//set keep alive task
	::AfxBeginThread((AFX_THREADPROC)KeepAliveThread, pDlg);
	
	((CButton *)(pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT)))->SetWindowText(_T("MQTT Disconnect"));	
	((CButton *)(pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT)))->EnableWindow(TRUE);
	((CButton *)(pDlg->GetDlgItem(IDC_BUTTON_PUBLISH)))->EnableWindow(TRUE);

	return 0;

flagConnectFailed:
	::AfxMessageBox(_T("Fail to connect!"));
	if(pDlg->m_hSocket != -1)
		BC28_CloseTcpSocket(pDlg->m_hSocket);

	((CButton *)(pDlg->GetDlgItem(IDC_BUTTON_CONNECT_MQTT)))->EnableWindow(TRUE);

	return 1;
}


void CSimWRL8500Dlg::OnBnClickedButtonConnectMqtt()
{
	// TODO: Add your control notification handler code here
	if(m_flagConnectMQTT == 0)
	{
		((CButton *)GetDlgItem(IDC_BUTTON_CONNECT_MQTT))->EnableWindow(FALSE);
		::AfxBeginThread((AFX_THREADPROC)ConnectMqttThread, this);
	}
	else
	{
		CloseMQTT();
	}
}

void CSimWRL8500Dlg::OnBnClickedButtonPublish()
{
	// TODO: Add your control notification handler code here
	char topic[256], msg[512];
	CString szEdit;

	// get topic
	((CEdit*)GetDlgItem(IDC_EDIT_TOPIC))->GetWindowText(szEdit);
	if(szEdit.GetLength() < 1 || szEdit.GetLength() > 255)
	{
		::AfxMessageBox(_T("No Topic!"));
		return;
	}
	else
	{
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			topic[i] = (BYTE)pBuf[i];
			if(topic[i] < ' ')
			{
				break;
			}
		}

		topic[i] = 0;
	}

	// get message
	((CEdit*)GetDlgItem(IDC_EDIT_MESSAGE))->GetWindowText(szEdit);
	if(szEdit.GetLength() < 1 || szEdit.GetLength() > 480)
	{
		::AfxMessageBox(_T("No Message!"));
		return;
	}
	else
	{
		DWORD num, i;
		TCHAR *pBuf = szEdit.GetBuffer(0);

		//strcpy(msg, BC28_GetIMSI());
		//num = strlen(msg);
		num = 0;

		for(i=0; i<szEdit.GetLength(); i++)
		{
			msg[num] = (BYTE)pBuf[i];
			if(msg[num] < ' ')
			{
				break;
			}
			num++;
		}

		msg[num] = 0;
	}

	BYTE buf[1024];
	int len = MQTT_PublishMessage(buf, 1024, 0, 0, 0, topic, msg, 0);
	if(BC28_WriteTcpSocket(m_hSocket, buf, len) > 0)
	{

	}
	else
	{
		::AfxMessageBox(_T("Failed to Publish!"));
	}
}
/*
int CSimWRL8500Dlg::InitSocketRcvQ(void)
{
	InitializeCriticalSection(&m_csSocketRcvQ);
	m_bufSocketRcvQ = new BYTE[SOCKET_RCV_BUF_SIZE];
	m_headSocketRcvQ = 0;
	m_tailSocketRcvQ = 0;
}

int CSimWRL8500Dlg::PushSocketRcvQ(BYTE *data, int size)
{
	int i = 0;

	EnterCriticalSection(&m_csSocketRcvQ);

	for(; i<size; i++)
	{
		m_bufSocketRcvQ[m_tailSocketRcvQ++] = data[i];
		if(m_tailSocketRcvQ >= SOCKET_RCV_BUF_SIZE)
			m_tailSocketRcvQ = 0;

		//overwrite
		if(m_tailSocketRcvQ == m_headSocketRcvQ)
			m_headSocketRcvQ++;
		if(m_headSocketRcvQ >= SOCKET_RCV_BUF_SIZE)
			m_headSocketRcvQ = 0;
	}

	LeaveCriticalSection(&m_csSocketRcvQ);

	return i;
}

int CSimWRL8500Dlg::PopSocketRcvQ(BYTE *data, int size)
{
	int i = 0;

	EnterCriticalSection(&m_csSocketRcvQ);

	if(m_tailSocketRcvQ == 0 && m_headSocketRcvQ == 0)
	{
		LeaveCriticalSection(&m_csSocketRcvQ);
		return 0;
	}

	for(; i<size; i++)
	{
		data[i] = m_bufSocketRcvQ[m_headSocketRcvQ++];
		if(m_headSocketRcvQ >= SOCKET_RCV_BUF_SIZE)
			m_headSocketRcvQ = 0;
		if(m_tailSocketRcvQ == m_headSocketRcvQ)
		{
			m_headSocketRcvQ = m_tailSocketRcvQ = 0;
			break;
		}
	}

	LeaveCriticalSection(&m_csSocketRcvQ);

	return i;
}

int CSimWRL8500Dlg::DeinitSocketRcvQ(void)
{
	EnterCriticalSection(&m_csSocketRcvQ);

	delete [] m_bufSocketRcvQ;
	m_bufSocketRcvQ = NULL;
	m_headSocketRcvQ = 0;
	m_tailSocketRcvQ = 0;

	LeaveCriticalSection(&m_csSocketRcvQ);
	DeleteCriticalSection(&m_csSocketRcvQ);
}
*/
int CSimWRL8500Dlg::SendATCmd(const char* cmd, char *rcv, int rcv_size, int timeout)
{
	DWORD num;

	WriteFile(m_hComm, cmd, strlen(cmd), &num, NULL);

	//WaitForSingleObject(h, timeout);

	return 0;
}

void CSimWRL8500Dlg::CloseMQTT(void)
{
	((CButton *)(GetDlgItem(IDC_BUTTON_CONNECT_MQTT)))->EnableWindow(FALSE);

	BYTE msg[4];
	int msg_size = MQTT_DisconnectMessage(msg, 4);
	BC28_WriteTcpSocket(m_hSocket, msg, msg_size);
	BC28_CloseTcpSocket(m_hSocket);

	((CButton *)(GetDlgItem(IDC_BUTTON_CONNECT_MQTT)))->SetWindowText(_T("Open MQTT"));
	((CButton *)(GetDlgItem(IDC_BUTTON_CONNECT_MQTT)))->EnableWindow(TRUE);
	((CButton *)GetDlgItem(IDC_BUTTON_PUBLISH))->EnableWindow(FALSE);

	m_flagConnectMQTT = 0;
}

void MqttSubscribeLisetner(int socket, int size)
{
	BYTE msg[1024];
	int count = 0, times = 10000/200;

	while(count < size && times--)
	{
		::Sleep(200);
		count += BC28_ReadTcpSocket(socket, &msg[count], size - count);
	}

	if(count > 2)
	{
		char topic[1024];
		char message[1024];

		if(MQTT_ParsePublishMessage(msg, size, topic, message))
		{
			CString szMsg;
			TCHAR temp[1024];

			for(count=0; count<strlen(topic); count++)
				temp[count] = topic[count];

			temp[count] = 0;
			szMsg = CString(temp);
			g_pInstDlg->AddStringToDebugList(szMsg);

			for(count=0; count<strlen(message); count++)
				temp[count] = message[count];

			temp[count] = 0;
			szMsg = CString(temp);
			g_pInstDlg->AddStringToDebugList(szMsg);
		}
	}
}

void CSimWRL8500Dlg::OnBnClickedButtonSubscribe()
{
	// TODO: Add your control notification handler code here
	char topic[256];
	CString szEdit;

	// get topic
	((CEdit*)GetDlgItem(IDC_EDIT_TOPIC_SUB))->GetWindowText(szEdit);
	if(szEdit.GetLength() < 1 || szEdit.GetLength() > 255)
	{
		::AfxMessageBox(_T("Invalid Topic!"));
		return;
	}
	else
	{
		DWORD num, i;

		TCHAR *pBuf = szEdit.GetBuffer(0);
		for(i=0; i<szEdit.GetLength(); i++)
		{
			topic[i] = (BYTE)pBuf[i];
			if(topic[i] < ' ')
			{
				break;
			}
		}

		topic[i] = 0;
	}

	BYTE buf[512];
	int len = MQTT_SubscribeMessage(buf, 512, topic);
	if(BC28_WriteTcpSocket(m_hSocket, buf, len) > 0)
	{
		//wait ack
		int times = 5000/500;
		int count = 0;
		while(count < MQTT_MSG_SIZE_SUBACK && times--)
		{
			::Sleep(500);
			count += BC28_ReadTcpSocket(m_hSocket, &buf[count], MQTT_MSG_SIZE_SUBACK - count);
		}

		if(count > 2)
		{
			if(MQTT_GetMessageType(buf) == MQTT_MSG_TYPE_SUBACK)
			{
				BC28_SetSocketListener(MqttSubscribeLisetner);
			}
		}
	}
	else
	{
		::AfxMessageBox(_T("Failed to Subscribe!"));
	}
}



void CSimWRL8500Dlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	switch(nIDEvent)
	{
	case TIMER_KEEP_ALIVE:
		break;
	}

	CDialog::OnTimer(nIDEvent);
}
