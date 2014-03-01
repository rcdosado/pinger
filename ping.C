

#include "CmnHdr.H"                  
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include "Resource.H"


#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"comctl32.lib")



HANDLE (WINAPI *pIcmpCreateFile)(VOID);
BOOL (WINAPI *pIcmpCloseHandle)(HANDLE);
DWORD (WINAPI *pIcmpSendEcho)(HANDLE,DWORD,LPVOID,WORD,LPVOID,LPVOID,DWORD,DWORD);


struct o
{	unsigned char Ttl,Tos,Flags,OptionsSize,*OptionsData;
};

struct
{	DWORD Address;
	unsigned long  Status,RoundTripTime;
	unsigned short DataSize,Reserved;
	void *Data;
	struct o  Options;
} E;

BOOL g_verbose = TRUE;
BOOL g_stopflag = FALSE;
WSADATA wsaData;
HANDLE hIcmp;
struct hostent *s;
HANDLE g_windowpane_handle;

LPTSTR Help = "Ping v1.0 by 0xrcd - help\r\n" \
"------------------------------------\r\n"\
"-This utility sends a ping packet to a target\r\n"\
"and awaits a reply giving you the time of the\r\n"\
"round trip from this computer to a remote \r\n"\
"address you have specified..\r\n"\

"\r\nExample Addresses:\r\n"\
"192.168.0.100\r\n"\
"google.com\r\n"\
"facebook.com\r\n";


HANDLE	newThread(LPTHREAD_START_ROUTINE thProcedure, LPVOID params)
{
	DWORD	dwThreadID;
	return	CreateThread(0, 0, thProcedure, params, 0, &dwThreadID);
}


/////////////////////////////////////////////////////////////


void ClearStatusWindow( HWND hwnd)
{SetDlgItemText(hwnd,IDC_STATUS,"");
}

/////////////////////////////////////////////////////////////
void PrintStatusWindow(HWND hwnd,LPTSTR msg)
{
int ndx;
HWND hEdit;
	hEdit = GetDlgItem (hwnd, IDC_STATUS);
	ndx = GetWindowTextLength (hEdit);
	SetFocus (hEdit);
	SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) msg));
}
////////////////////////////////////////////////////
void PrintStatusWindowX(HWND hwnd,const char *szFormat, ...)
{
va_list args;
LPSTR pM;
	pM=(LPSTR)HeapAlloc((HANDLE)GetProcessHeap(),HEAP_ZERO_MEMORY,1024);
	va_start(args, szFormat);
	vsprintf(pM,szFormat,args);
	va_end(args);
	PrintStatusWindow(hwnd,pM);
	HeapFree((HANDLE)GetProcessHeap(),0,(LPSTR)pM);
return;
}
/////////////////////////////////////////////////////////////
DWORD TopXY(int wDim, int sDim)
{
    sDim >>= 1;
    wDim >>= 1;
    return sDim - wDim;
}

/////////////////////////////////////////////////////////////
void PrintWin32Error(HWND hwnd, DWORD ErrorCode )
{
	LPVOID lpMsgBuf;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL, ErrorCode, 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf, 0, NULL );
	PrintStatusWindow(hwnd,lpMsgBuf);
	LocalFree( lpMsgBuf );
}
///////////////////////////////////////////////////////////
int init_winsock(HWND hwnd,WORD version,BOOL g_verbose)
{  int err;
	if(g_verbose) PrintStatusWindow(hwnd,"[+] Initialising Winsock..\r\n");
	err = WSAStartup(version, &wsaData);
	if(err)
	{	PrintStatusWindow(hwnd,"[-] Could not initialise Winsock\r\n");
		if(g_verbose) PrintWin32Error(hwnd,WSAGetLastError());
	}
	return err;
}
/////////////////////////////////////////////////////////////

BOOL init_icmp(HWND hwnd)
{  
	if(g_verbose)
	PrintStatusWindow(hwnd,"[+] Initialising ICMP..\r\n");
	
	if( !(hIcmp=LoadLibrary("ICMP.DLL")) && g_verbose )
	{
		PrintWin32Error(hwnd,GetLastError());
		return FALSE;
	}		
	pIcmpCreateFile=GetProcAddress(hIcmp,"IcmpCreateFile");
	pIcmpCloseHandle=GetProcAddress(hIcmp,"IcmpCloseHandle");
	pIcmpSendEcho=GetProcAddress(hIcmp,"IcmpSendEcho");
	if( !pIcmpCreateFile && !pIcmpCloseHandle && !pIcmpSendEcho && g_verbose)
	{
		PrintWin32Error(hwnd,GetLastError());
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
BOOL Dlg_OnInitDialog (HWND hwnd, HWND hwndFocus,
   LPARAM lParam) {
   DWORD wd,wh,wtx,wty;

	HWND hUDC1,hUDC2;
	// Needed by our control


	InitCommonControls();
    hUDC1 =  CreateUpDownControl(UDS_SETBUDDYINT | UDS_NOTHOUSANDS | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
	163,68,15,23,hwnd,
	IDC_NPINGUD,
	(HINSTANCE)hwnd,
	GetDlgItem(hwnd,IDC_NPINGS),
	0,255,
	0x20000);

    hUDC2 =  CreateUpDownControl(UDS_SETBUDDYINT | UDS_NOTHOUSANDS | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
	163,105,15,23,hwnd,
	IDC_TTLUD,
	(HINSTANCE)hwnd,
	GetDlgItem(hwnd,IDC_TTLE),
	0,255,
	0x20000);

	// default values
	SetDlgItemInt(hwnd,IDC_TTLE,100,FALSE);
	SetDlgItemInt(hwnd,IDC_NPINGS,2,FALSE);


   // Associate an icon with the dialog box.
   chSETDLGICONS(hwnd, IDI_PINGER, IDI_PINGER);
   
    // Center our dialog
    wd = 306;
    wh = 433;
    wtx = TopXY(wd, GetSystemMetrics(SM_CXSCREEN));
    wty = TopXY(wh, GetSystemMetrics(SM_CYSCREEN));
	SetWindowPos(hwnd,HWND_TOP,wtx,wty,wd,wh,SWP_SHOWWINDOW);
    


	if(init_winsock(hwnd,MAKEWORD(1, 1),g_verbose)) ExitProcess(FALSE);

	if(!init_icmp(hwnd))
	{
	PrintWin32Error(hwnd,GetLastError());
	WSACleanup();
	ExitProcess(FALSE);
	}
	
	PrintStatusWindow(hwnd,"[+] Winsock & ICMP Initialization Successful...\r\n"\
						   "[+] Waiting for Commands..");
	// default to verbose mode
	CheckDlgButton(hwnd,IDC_VERBOSE,TRUE);

	// focus on the ip/address edit box
	SetFocus(GetDlgItem(hwnd,IDC_ADDRESS));
	

   return FALSE;
}


/////////////////////////////////////////////////////////////
void exit_winsock()
{	
	WSACleanup();
}
////////////////////////////////////////////////////////////
void exit_icmp()
{  
	if(hIcmp) FreeLibrary( hIcmp );

}
////////////////////////////////////////////////////////////

unsigned long hostlookupaddr(HWND hwnd,char *target,BOOL g_verbose)
{	unsigned long iadr;
	s=NULL;

	iadr=inet_addr(target);
	if(iadr==INADDR_NONE)
	{	if(g_verbose) PrintStatusWindow(hwnd,"[+] Trying gethostbyname\r\n");
		
		s=gethostbyname(target);
		if(s==NULL)
		{	PrintStatusWindowX(hwnd,"[-] No entry for IP...(Error Code:%d)\r\n",WSAGetLastError());
			return 0;
		}
		return ((unsigned long *)(s->h_addr_list[0]))[0];
	}
	return iadr;
}

void ping_ip(HWND hwnd,unsigned long dwIPAddr,unsigned char ttl,unsigned int npings)
{	HANDLE hIP;
	struct o I;
	unsigned long d;
	g_stopflag = FALSE;

	if(g_verbose) PrintStatusWindowX(hwnd,"[+] Sending ping(s) to :%s\r\n",inet_ntoa(((struct in_addr *)&dwIPAddr)[0]));
	hIP=pIcmpCreateFile();
	while(npings>0)
	{
		if(g_stopflag)
		break;

		PrintStatusWindowX(hwnd,"[+] Ping %3d: ",npings);
		I.Ttl=ttl;
		pIcmpSendEcho(hIP,dwIPAddr,0,0,&I,&E,sizeof(E),8000);
		d=E.Address;
      if(!d) PrintStatusWindow(hwnd,"Request timed out\r\n");
      else
		{	s=gethostbyaddr((char *)&d,4,PF_INET);
			if(s==NULL)
				PrintStatusWindowX(hwnd,"%s reply: ",inet_ntoa(((struct in_addr *)&d)[0]));
			else
				PrintStatusWindowX(hwnd,"%s reply: ",s->h_name);
			PrintStatusWindowX(hwnd,"RTT: %dms,TTL:%d\r\n",E.RoundTripTime,E.Options.Ttl);
      }
		npings--;

	}
	pIcmpCloseHandle(hIP);

}

////////////////////////////////////////////////////////////
void WINAPI PingAddress(void)
{
char addr[255];
BOOL *pF1=FALSE,*pF2=FALSE;
UINT npings=0,ttl=0;
unsigned long padr=0;
HWND hwnd = g_windowpane_handle;

// check the g_verbose flag
if(IsDlgButtonChecked(hwnd,IDC_VERBOSE)) 
{
	g_verbose = TRUE;
}
//	__asm{int 3 }
if(!GetDlgItemText(hwnd,IDC_ADDRESS,(LPTSTR)addr,255))
{ 
ClearStatusWindow(hwnd);
PrintStatusWindow(hwnd,"\t  ***** ERROR *****\r\n\r\n\t   Target not specified.\r\n");
return;
}
npings=(UINT)GetDlgItemInt(hwnd,IDC_NPINGS,&pF1,FALSE);
if(pF1 == FALSE || npings == NULL)
{
ClearStatusWindow(hwnd);
PrintStatusWindow(hwnd,"\t  ***** ERROR *****\r\n\r\n\t   Invalid Number of Pings\r\n");
return;
}

ttl=(UINT)GetDlgItemInt(hwnd,IDC_TTLE,&pF2,FALSE);
if(pF2 == FALSE || ttl==NULL)
{
ClearStatusWindow(hwnd);
PrintStatusWindow(hwnd,"\t  ***** ERROR *****\r\n\r\n\t   Invalid Time To Live Value\r\n");
return;
}


ClearStatusWindow(hwnd);
PrintStatusWindowX(hwnd,"\t      =Ping by 0xrcd=\r\n\r\n" \
						"[+]  Target Address:%s\r\n" \
						"[+]  Time to Live	:%u\r\n" \
						"[+]  Number of Pings:%u\r\n" \
						"[+]  Verifying Target...\r\n",addr,ttl,npings);



padr = hostlookupaddr(hwnd,addr,g_verbose);
if(!padr)
{
	PrintStatusWindow(hwnd,"[-] Invalid target pinging aborted..\r\n");
	return;
}	
if(g_verbose)
PrintStatusWindow(hwnd,"[+] Target Valid...\r\n[+] Now starting to ping...\r\n");
	
EnableWindow(GetDlgItem(hwnd,IDC_PING),FALSE);
EnableWindow(GetDlgItem(hwnd,IDC_CLOSE),FALSE);
EnableWindow(GetDlgItem(hwnd,IDC_MYHELP),FALSE);

ping_ip((HWND)hwnd,(unsigned long)padr,(unsigned char)ttl,(unsigned int)npings);

EnableWindow(GetDlgItem(hwnd,IDC_PING),TRUE);
EnableWindow(GetDlgItem(hwnd,IDC_CLOSE),TRUE);
EnableWindow(GetDlgItem(hwnd,IDC_MYHELP),TRUE);

PrintStatusWindowX(hwnd,"\r\n[+] The target %s replies back, it's alive!..\r\n" \
				        "[+] ***************** END *******************\r\n",addr);
return;
}
////////////////////////////////////////////////////////////

HANDLE thread = 0;

void Dlg_OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
unsigned long padr = 0;
unsigned char ttl = 0;
unsigned int npings = 0;


   switch (id) {


	  case IDC_VERBOSE:
			if(IsDlgButtonChecked(GetDlgItem(hwnd,IDC_VERBOSE), IDC_VERBOSE))
				g_verbose = TRUE;
			else
				g_verbose = FALSE;
			break;
      case IDC_PING:

			g_windowpane_handle = hwnd;
			//wait to close
			if( thread != 0)
			{
				WaitForSingleObject(thread,INFINITE);
				// create another
				thread=newThread( &PingAddress,0);				
			}
			else
			{
				thread=newThread( &PingAddress,0);	
			}
			break;

      case IDC_CANCEL:
			g_stopflag = TRUE;
			if(!IsWindowEnabled(GetDlgItem(hwnd,IDC_PING)))
			{
				EnableWindow(GetDlgItem(hwnd,IDC_PING),TRUE);
				EnableWindow(GetDlgItem(hwnd,IDC_CLOSE),TRUE);
				EnableWindow(GetDlgItem(hwnd,IDC_MYHELP),TRUE);
			}
			break;

      case IDC_CLOSE:
		    SendMessageA(hwnd, WM_CLOSE, 0, 0);
			break;


      case IDC_MYHELP:
			ClearStatusWindow(hwnd);
			PrintStatusWindow(hwnd,Help);
			break;

	  case WM_CLOSE:
	  case WM_DESTROY:
			g_stopflag = TRUE;
			exit_icmp();
			exit_winsock();

			EndDialog(hwnd,id);
			break;

   }
}


/////////////////////////////////////////////////////////////

BOOL CALLBACK Dlg_Proc (HWND hwnd, UINT uMsg,
   WPARAM wParam, LPARAM lParam) 
{

   switch (uMsg) {
      chHANDLE_DLGMSG(hwnd, WM_INITDIALOG,  Dlg_OnInitDialog);
      chHANDLE_DLGMSG(hwnd, WM_COMMAND,     Dlg_OnCommand);
	  //insert other command handlers here
   }
   return(FALSE);
}


void main( void )
{
    DialogBox((HANDLE)GetModuleHandleA(0), MAKEINTRESOURCE(IDD_PING),NULL, Dlg_Proc);
}