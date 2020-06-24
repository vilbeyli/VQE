//	VQE
//	Copyright(C) 2020  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#include "VQEngine.h"



#define LOG_WINDOW_MESSAGE_EVENTS 0
static void LogWndMsg(UINT uMsg, HWND hwnd);

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LogWndMsg(uMsg, hwnd);

	IWindow* pWindow = reinterpret_cast<IWindow*> (::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (!pWindow)
	{
		//Log::Warning("WndProc::pWindow=nullptr");
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
		// https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	case WM_CREATE:
		if (pWindow->pOwner) pWindow->pOwner->OnWindowCreate(pWindow);
		return 0;


		// https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
	case WM_SIZE:
	{
		if (pWindow->pOwner) pWindow->pOwner->OnWindowResize(hwnd);
		return 0;
	}

	case WM_KEYDOWN:
		if (pWindow->pOwner) pWindow->pOwner->OnWindowKeyDown(wParam);
		return 0;

	case WM_SYSKEYDOWN:
		if ((wParam == VK_RETURN) && (lParam & (1 << 29))) // Handle ALT+ENTER
		{
			if (pWindow->pOwner)
			{
				pWindow->pOwner->OnToggleFullscreen(hwnd);
				return 0;
			}
		}
		// Send all other WM_SYSKEYDOWN messages to the default WndProc.
		break;

		// https://docs.microsoft.com/en-us/windows/win32/learnwin32/painting-the-window
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_SETFOCUS:
	{
		if (pWindow->pOwner) pWindow->pOwner->OnWindowFocus(pWindow);
		return 0;
	}

	// https://docs.microsoft.com/en-us/windows/win32/learnwin32/closing-the-window
	case WM_CLOSE:
		if (pWindow->pOwner) pWindow->pOwner->OnWindowClose(pWindow);
		return 0;

	case WM_DESTROY:
		return 0;

	}


	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



void VQEngine::OnWindowCreate(IWindow* pWnd)
{
}

void VQEngine::OnWindowResize(HWND hWnd)
{
	// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#handling-window-resizing
	RECT clientRect = {};
	GetClientRect(hWnd, &clientRect);
	int w = clientRect.right - clientRect.left;
	int h = clientRect.bottom - clientRect.top;
	
	// Due to multi-threading, this thread will record the events and 
	// Render Thread will process the queue at the of a render loop
	mWinEventQueue.AddItem(new WindowResizeEvent(w, h, hWnd));
}

void VQEngine::OnToggleFullscreen(HWND hWnd)
{
	// Due to multi-threading, this thread will record the events and 
	// Render Thread will process the queue at the of a render loop
	mWinEventQueue.AddItem(new ToggleFullscreenEvent(hWnd));
}

void VQEngine::OnWindowMinimize(IWindow* pWnd)
{
}

void VQEngine::OnWindowFocus(IWindow* pWindow)
{
	//Log::Info("On Focus!");
}


void VQEngine::OnWindowKeyDown(WPARAM wParam)
{
}

void VQEngine::OnWindowClose(IWindow* pWindow)
{
	mRenderer.GetWindowSwapChain(static_cast<Window*>(pWindow)->GetHWND()).WaitForGPU();

	pWindow->Close();

	if (pWindow == mpWinMain.get())
		PostQuitMessage(0);

}
 


// ----------------------------------------------------------------------------------------------------
static void LogWndMsg(UINT uMsg, HWND hwnd)
{
#if LOG_WINDOW_MESSAGE_EVENTS
#define HANDLE_CASE(EVENT)\
case EVENT: Log::Info(#EVENT"\t(0x%04x)\t\t<hwnd=0x%x>", EVENT, hwnd); break
	switch (uMsg)
	{
		// https://www.autoitscript.com/autoit3/docs/appendix/WinMsgCodes.htm
		HANDLE_CASE(WM_MOUSELAST);
		HANDLE_CASE(WM_ACTIVATE);
		HANDLE_CASE(WM_ACTIVATEAPP);
		HANDLE_CASE(WM_AFXFIRST);
		HANDLE_CASE(WM_AFXLAST);
		HANDLE_CASE(WM_APP);
		HANDLE_CASE(WM_APPCOMMAND);
		HANDLE_CASE(WM_ASKCBFORMATNAME);
		HANDLE_CASE(WM_CANCELJOURNAL);
		HANDLE_CASE(WM_CANCELMODE);
		HANDLE_CASE(WM_CAPTURECHANGED);
		HANDLE_CASE(WM_CHANGECBCHAIN);
		HANDLE_CASE(WM_CHANGEUISTATE);
		HANDLE_CASE(WM_CHAR);
		HANDLE_CASE(WM_CHARTOITEM);
		HANDLE_CASE(WM_CHILDACTIVATE);
		HANDLE_CASE(WM_CLEAR);
		HANDLE_CASE(WM_CLOSE);
		HANDLE_CASE(WM_COMMAND);
		HANDLE_CASE(WM_COMMNOTIFY);
		HANDLE_CASE(WM_COMPACTING);
		HANDLE_CASE(WM_COMPAREITEM);
		HANDLE_CASE(WM_CONTEXTMENU);
		HANDLE_CASE(WM_COPY);
		HANDLE_CASE(WM_COPYDATA);
		HANDLE_CASE(WM_CREATE);
		HANDLE_CASE(WM_CTLCOLORBTN);
		HANDLE_CASE(WM_CTLCOLORDLG);
		HANDLE_CASE(WM_CTLCOLOREDIT);
		HANDLE_CASE(WM_CTLCOLORLISTBOX);
		HANDLE_CASE(WM_CTLCOLORMSGBOX);
		HANDLE_CASE(WM_CTLCOLORSCROLLBAR);
		HANDLE_CASE(WM_CTLCOLORSTATIC);
		HANDLE_CASE(WM_CUT);
		HANDLE_CASE(WM_DEADCHAR);
		HANDLE_CASE(WM_DELETEITEM);
		HANDLE_CASE(WM_DESTROY);
		HANDLE_CASE(WM_DESTROYCLIPBOARD);
		HANDLE_CASE(WM_DEVICECHANGE);
		HANDLE_CASE(WM_DEVMODECHANGE);
		HANDLE_CASE(WM_DISPLAYCHANGE);
		HANDLE_CASE(WM_DRAWCLIPBOARD);
		HANDLE_CASE(WM_DRAWITEM);
		HANDLE_CASE(WM_DROPFILES);
		HANDLE_CASE(WM_ENABLE);
		HANDLE_CASE(WM_ENDSESSION);
		HANDLE_CASE(WM_ENTERIDLE);
		HANDLE_CASE(WM_ENTERMENULOOP);
		HANDLE_CASE(WM_ENTERSIZEMOVE);
		HANDLE_CASE(WM_ERASEBKGND);
		HANDLE_CASE(WM_EXITMENULOOP);
		HANDLE_CASE(WM_EXITSIZEMOVE);
		HANDLE_CASE(WM_FONTCHANGE);
		HANDLE_CASE(WM_GETDLGCODE);
		HANDLE_CASE(WM_GETFONT);
		HANDLE_CASE(WM_GETHOTKEY);
		HANDLE_CASE(WM_GETICON);
		HANDLE_CASE(WM_GETMINMAXINFO);
		HANDLE_CASE(WM_GETOBJECT);
		HANDLE_CASE(WM_GETTEXT);
		HANDLE_CASE(WM_GETTEXTLENGTH);
		HANDLE_CASE(WM_HANDHELDFIRST);
		HANDLE_CASE(WM_HANDHELDLAST);
		HANDLE_CASE(WM_HELP);
		HANDLE_CASE(WM_HOTKEY);
		HANDLE_CASE(WM_HSCROLL);
		HANDLE_CASE(WM_HSCROLLCLIPBOARD);
		HANDLE_CASE(WM_ICONERASEBKGND);
		HANDLE_CASE(WM_IME_CHAR);
		HANDLE_CASE(WM_IME_COMPOSITION);
		HANDLE_CASE(WM_IME_COMPOSITIONFULL);
		HANDLE_CASE(WM_IME_CONTROL);
		HANDLE_CASE(WM_IME_ENDCOMPOSITION);
		HANDLE_CASE(WM_IME_KEYDOWN);
		HANDLE_CASE(WM_IME_KEYUP);
		HANDLE_CASE(WM_IME_NOTIFY);
		HANDLE_CASE(WM_IME_REQUEST);
		HANDLE_CASE(WM_IME_SELECT);
		HANDLE_CASE(WM_IME_SETCONTEXT);
		HANDLE_CASE(WM_IME_STARTCOMPOSITION);
		HANDLE_CASE(WM_INITDIALOG);
		HANDLE_CASE(WM_INITMENU);
		HANDLE_CASE(WM_INITMENUPOPUP);
		HANDLE_CASE(WM_INPUT);
		HANDLE_CASE(WM_INPUTLANGCHANGE);
		HANDLE_CASE(WM_INPUTLANGCHANGEREQUEST);
		HANDLE_CASE(WM_KEYDOWN);
		HANDLE_CASE(WM_KEYLAST);
		HANDLE_CASE(WM_KEYUP);
		HANDLE_CASE(WM_KILLFOCUS);
		HANDLE_CASE(WM_LBUTTONDBLCLK);
		HANDLE_CASE(WM_LBUTTONDOWN);
		HANDLE_CASE(WM_LBUTTONUP);
		HANDLE_CASE(WM_MBUTTONDBLCLK);
		HANDLE_CASE(WM_MBUTTONDOWN);
		HANDLE_CASE(WM_MBUTTONUP);
		HANDLE_CASE(WM_MDIACTIVATE);
		HANDLE_CASE(WM_MDICASCADE);
		HANDLE_CASE(WM_MDICREATE);
		HANDLE_CASE(WM_MDIDESTROY);
		HANDLE_CASE(WM_MDIGETACTIVE);
		HANDLE_CASE(WM_MDIICONARRANGE);
		HANDLE_CASE(WM_MDIMAXIMIZE);
		HANDLE_CASE(WM_MDINEXT);
		HANDLE_CASE(WM_MDIREFRESHMENU);
		HANDLE_CASE(WM_MDIRESTORE);
		HANDLE_CASE(WM_MDISETMENU);
		HANDLE_CASE(WM_MDITILE);
		HANDLE_CASE(WM_MEASUREITEM);
		HANDLE_CASE(WM_MENUCHAR);
		HANDLE_CASE(WM_MENUCOMMAND);
		HANDLE_CASE(WM_MENUDRAG);
		HANDLE_CASE(WM_MENUGETOBJECT);
		HANDLE_CASE(WM_MENURBUTTONUP);
		HANDLE_CASE(WM_MENUSELECT);
		HANDLE_CASE(WM_MOUSEACTIVATE);
		HANDLE_CASE(WM_MOUSEFIRST);
		HANDLE_CASE(WM_MOUSEHOVER);
		HANDLE_CASE(WM_MOUSELEAVE);
		HANDLE_CASE(WM_MOUSEWHEEL);
		HANDLE_CASE(WM_MOVE);
		HANDLE_CASE(WM_MOVING);
		HANDLE_CASE(WM_NCACTIVATE);
		HANDLE_CASE(WM_NCCALCSIZE);
		HANDLE_CASE(WM_NCCREATE);
		HANDLE_CASE(WM_NCDESTROY);
		HANDLE_CASE(WM_NCHITTEST);
		HANDLE_CASE(WM_NCLBUTTONDBLCLK);
		HANDLE_CASE(WM_NCLBUTTONDOWN);
		HANDLE_CASE(WM_NCLBUTTONUP);
		HANDLE_CASE(WM_NCMBUTTONDBLCLK);
		HANDLE_CASE(WM_NCMBUTTONDOWN);
		HANDLE_CASE(WM_NCMBUTTONUP);
		HANDLE_CASE(WM_NCMOUSEHOVER);
		HANDLE_CASE(WM_NCMOUSELEAVE);
		HANDLE_CASE(WM_NCMOUSEMOVE);
		HANDLE_CASE(WM_NCPAINT);
		HANDLE_CASE(WM_NCRBUTTONDBLCLK);
		HANDLE_CASE(WM_NCRBUTTONDOWN);
		HANDLE_CASE(WM_NCRBUTTONUP);
		HANDLE_CASE(WM_NCXBUTTONDBLCLK);
		HANDLE_CASE(WM_NCXBUTTONDOWN);
		HANDLE_CASE(WM_NCXBUTTONUP);
		HANDLE_CASE(WM_NEXTDLGCTL);
		HANDLE_CASE(WM_NEXTMENU);
		HANDLE_CASE(WM_NOTIFY);
		HANDLE_CASE(WM_NOTIFYFORMAT);
		HANDLE_CASE(WM_NULL);
		HANDLE_CASE(WM_PAINT);
		HANDLE_CASE(WM_PAINTCLIPBOARD);
		HANDLE_CASE(WM_PAINTICON);
		HANDLE_CASE(WM_PALETTECHANGED);
		HANDLE_CASE(WM_PALETTEISCHANGING);
		HANDLE_CASE(WM_PARENTNOTIFY);
		HANDLE_CASE(WM_PASTE);
		HANDLE_CASE(WM_PENWINFIRST);
		HANDLE_CASE(WM_PENWINLAST);
		HANDLE_CASE(WM_POWER);
		HANDLE_CASE(WM_POWERBROADCAST);
		HANDLE_CASE(WM_PRINT);
		HANDLE_CASE(WM_PRINTCLIENT);
		HANDLE_CASE(WM_QUERYDRAGICON);
		HANDLE_CASE(WM_QUERYENDSESSION);
		HANDLE_CASE(WM_QUERYNEWPALETTE);
		HANDLE_CASE(WM_QUERYOPEN);
		HANDLE_CASE(WM_QUERYUISTATE);
		HANDLE_CASE(WM_QUEUESYNC);
		HANDLE_CASE(WM_QUIT);
		HANDLE_CASE(WM_RBUTTONDBLCLK);
		HANDLE_CASE(WM_RBUTTONDOWN);
		HANDLE_CASE(WM_RBUTTONUP);
		HANDLE_CASE(WM_RENDERALLFORMATS);
		HANDLE_CASE(WM_RENDERFORMAT);
		HANDLE_CASE(WM_SETCURSOR);
		HANDLE_CASE(WM_SETFOCUS);
		HANDLE_CASE(WM_SETFONT);
		HANDLE_CASE(WM_SETHOTKEY);
		HANDLE_CASE(WM_SETICON);
		HANDLE_CASE(WM_SETREDRAW);
		HANDLE_CASE(WM_SETTEXT);
		HANDLE_CASE(WM_SETTINGCHANGE);
		HANDLE_CASE(WM_SHOWWINDOW);
		HANDLE_CASE(WM_SIZE);
		HANDLE_CASE(WM_SIZECLIPBOARD);
		HANDLE_CASE(WM_SIZING);
		HANDLE_CASE(WM_SPOOLERSTATUS);
		HANDLE_CASE(WM_STYLECHANGED);
		HANDLE_CASE(WM_STYLECHANGING);
		HANDLE_CASE(WM_SYNCPAINT);
		HANDLE_CASE(WM_SYSCHAR);
		HANDLE_CASE(WM_SYSCOLORCHANGE);
		HANDLE_CASE(WM_SYSCOMMAND);
		HANDLE_CASE(WM_SYSDEADCHAR);
		HANDLE_CASE(WM_SYSKEYDOWN);
		HANDLE_CASE(WM_SYSKEYUP);
		HANDLE_CASE(WM_TABLET_FIRST);
		HANDLE_CASE(WM_TABLET_LAST);
		HANDLE_CASE(WM_TCARD);
		HANDLE_CASE(WM_THEMECHANGED);
		HANDLE_CASE(WM_TIMECHANGE);
		HANDLE_CASE(WM_TIMER);
		HANDLE_CASE(WM_UNDO);
		HANDLE_CASE(WM_UNINITMENUPOPUP);
		HANDLE_CASE(WM_UPDATEUISTATE);
		HANDLE_CASE(WM_USER);
		HANDLE_CASE(WM_USERCHANGED);
		HANDLE_CASE(WM_VKEYTOITEM);
		HANDLE_CASE(WM_VSCROLL);
		HANDLE_CASE(WM_VSCROLLCLIPBOARD);
		HANDLE_CASE(WM_WINDOWPOSCHANGED);
		HANDLE_CASE(WM_WINDOWPOSCHANGING);
		HANDLE_CASE(WM_WTSSESSION_CHANGE);
		HANDLE_CASE(WM_XBUTTONDBLCLK);
		HANDLE_CASE(WM_XBUTTONDOWN);
		HANDLE_CASE(WM_XBUTTONUP);

		// duplicate value events
		///HANDLE_CASE(WM_KEYFIRST);
		///HANDLE_CASE(WM_IME_KEYLAST);
		///HANDLE_CASE(WM_MOUSEMOVE);
		///HANDLE_CASE(WM_UNICHAR);
		///HANDLE_CASE(WM_WININICHANGE);
	default: Log::Warning("LogWndMsg not defined for msg=0x%x", uMsg); break;
	}
#undef HANDLE_CASE
#endif
}

