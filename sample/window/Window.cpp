
#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "../../call_thunk.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;								// Application Instance

class CWindow
{
public:
	CWindow()
		: m_thunk(4, NULL, call_thunk::cc_stdcall)
	{
		m_thunk.bind(*this, &CWindow::WndProc);
	}
	virtual ~CWindow() { }

protected:
	WNDPROC GetWindowProc() const { return m_thunk; }

private:
	call_thunk::unsafe_thunk m_thunk;
	virtual LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)=0;
};

class CAboutDialog : public CWindow
{
	enum { IDD=IDD_ABOUTBOX };
public:
	CAboutDialog() { }
	INT_PTR DoModal(HWND hParentWnd)
	{
		return DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hParentWnd, (DLGPROC)GetWindowProc());
	}

private:
	virtual LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

class CMyWindow : public CWindow
{
public:
	ATOM RegisterClass(HINSTANCE hInstance);
	CMyWindow();
	BOOL Create(HINSTANCE hInstance, LPCTSTR lpszTitle, int nCmdShow);

	void Destroy()
	{
		if(m_hWnd) DestroyWindow(m_hWnd);
	}

private:
	HWND m_hWnd;
	TCHAR szWindowClass[MAX_LOADSTRING];

	virtual LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

};

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	MSG msg;
	HACCEL hAccelTable;

	TCHAR szTitle[MAX_LOADSTRING];
	hInst = hInstance; 
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	CMyWindow window;
	if(!window.RegisterClass(hInstance))
		return FALSE;
	
	if (!window.Create(hInstance, szTitle, nCmdShow))
	{
		DWORD dwError=GetLastError();
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINEXAM));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

CMyWindow::CMyWindow()
{
	m_hWnd=NULL;
	LoadString(hInst, IDC_WINEXAM, szWindowClass, MAX_LOADSTRING);
}

ATOM CMyWindow::RegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= GetWindowProc();
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINEXAM));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WINEXAM);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL CMyWindow::Create(HINSTANCE hInstance, LPCTSTR lpszTitle, int nCmdShow)
{
   HWND hWnd = CreateWindow(szWindowClass, lpszTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(m_hWnd, nCmdShow);
   UpdateWindow(m_hWnd);

   return TRUE;
}

LRESULT CMyWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		if(m_hWnd==NULL) m_hWnd=hWnd;
		return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			{
				CAboutDialog dialog;
				dialog.DoModal(m_hWnd);
			}
			break;
		case IDM_EXIT:
			DestroyWindow(m_hWnd);
			break;
		default:
			return DefWindowProc(m_hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


LRESULT CAboutDialog::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
