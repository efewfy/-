#include "stdafx.h"
#include "taskbar_manager.h"
#include "gui/session/session_form.h"
#include "gui/session/session_box.h"
#include "dwm_util.h"
#include <shobjidl.h>

using namespace ui;
namespace nim_comp
{
TaskbarTabItem::TaskbarTabItem(ui::Control *bind_control)
{
	ASSERT(NULL != bind_control);
	bind_control_ = bind_control;
	is_win7_or_greater_ = IsWindows7OrGreater();
	taskbar_manager_ = NULL;
}

ui::Control* TaskbarTabItem::GetBindControl()
{
	return bind_control_;
}

void TaskbarTabItem::Init(const std::wstring &taskbar_title)
{
	if (!is_win7_or_greater_)
		return;

	Create(NULL, taskbar_title.c_str(), WS_OVERLAPPED, 0, false);

	HRESULT ret = S_OK;
	BOOL truth = TRUE;
	ret |= DwmSetWindowAttribute(m_hWnd, DWMWA_HAS_ICONIC_BITMAP, &truth, sizeof(truth));
	ret |= DwmSetWindowAttribute(m_hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &truth, sizeof(truth));
	if (ret != S_OK)
	{
		is_win7_or_greater_ = false;
		QLOG_ERR(L"DwmSetWindowAttribute error: {0}") << ret;
	}
}

void TaskbarTabItem::UnInit()
{
	if (NULL != m_hWnd)
		DestroyWindow(m_hWnd);
}

void TaskbarTabItem::SetTaskbarTitle(const std::wstring &title)
{
	::SetWindowTextW(m_hWnd, title.c_str());
}

void TaskbarTabItem::SetTaskbarManager(TaskbarManager *taskbar_manager)
{
	taskbar_manager_ = taskbar_manager;
}

nim_comp::TaskbarManager* TaskbarTabItem::GetTaskbarManager()
{
	return taskbar_manager_;
}

bool TaskbarTabItem::InvalidateTab()
{
	if (!is_win7_or_greater_ || NULL == taskbar_manager_)
		return false;

	return (S_OK == DwmInvalidateIconicBitmaps(this->GetHWND()));
}

void TaskbarTabItem::OnSendThumbnail(int width, int height)
{
	if (!is_win7_or_greater_ || NULL == taskbar_manager_)
		return;

	HBITMAP bitmap = taskbar_manager_->GenerateBindControlBitmap(bind_control_, width, height);
	DwmSetIconicThumbnail(m_hWnd, bitmap, 0);

	DeleteObject(bitmap);
}

void TaskbarTabItem::OnSendPreview()
{
	if (!is_win7_or_greater_ || NULL == taskbar_manager_)
		return;

	HBITMAP bitmap = taskbar_manager_->GenerateBindControlBitmapWithForm(bind_control_);
	DwmSetIconicLivePreviewBitmap(m_hWnd, bitmap, NULL, 0);

	DeleteObject(bitmap);
}

std::wstring TaskbarTabItem::GetWindowClassName() const
{
	return L"Nim.TaskbarItem";
}

LRESULT TaskbarTabItem::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DWMSENDICONICTHUMBNAIL)
	{
		OnSendThumbnail(HIWORD(lParam), LOWORD(lParam));
		return 0;
	}
	else if (uMsg == WM_DWMSENDICONICLIVEPREVIEWBITMAP)
	{
		OnSendPreview();
		return 0;
	}
	else if (uMsg == WM_GETICON)
	{
		InvalidateTab();
	}
	else if (uMsg == WM_CLOSE)
	{
		if (NULL != taskbar_manager_)
			taskbar_manager_->OnTabItemClose(*this);

		return 0;
	}
	else if (uMsg == WM_ACTIVATE)
	{
		if (NULL != taskbar_manager_)
		{
			if (wParam != WA_INACTIVE)
			{
				taskbar_manager_->OnTabItemClicked(*this);
			}
		}
			
		return 0;
	}

	return __super::HandleMessage(uMsg, wParam, lParam);
}

void TaskbarTabItem::OnFinalMessage(HWND hWnd)
{
	__super::OnFinalMessage(hWnd);
	delete this;
}


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

TaskbarManager::TaskbarManager()
{
	parent_window_ = NULL;
	taskbar_list_ = NULL;
}

void TaskbarManager::Init(SessionForm *parent_window)
{
	ASSERT(NULL != parent_window);
	parent_window_ = parent_window;

	::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbar_list_));
	if (taskbar_list_)
	{
		taskbar_list_->HrInit();

		BOOL truth = FALSE;
		DwmSetWindowAttribute(parent_window->GetHWND(), DWMWA_HAS_ICONIC_BITMAP, &truth, sizeof(truth));
		DwmSetWindowAttribute(parent_window->GetHWND(), DWMWA_FORCE_ICONIC_REPRESENTATION, &truth, sizeof(truth));
	}
		
}

bool TaskbarManager::RegisterTab(TaskbarTabItem &tab_item)
{
	if (taskbar_list_ && NULL == tab_item.GetTaskbarManager())
	{
		if (S_OK == taskbar_list_->RegisterTab(tab_item.GetHWND(), parent_window_->GetHWND()))
		{
			if (S_OK == taskbar_list_->SetTabOrder(tab_item.GetHWND(), NULL))
			{
				tab_item.SetTaskbarManager(this);
				return true;
			}			
		}
	}

	return false;
}

bool TaskbarManager::UnregisterTab(TaskbarTabItem &tab_item)
{
	if (taskbar_list_)
	{
		tab_item.SetTaskbarManager(NULL);
		return (S_OK == taskbar_list_->UnregisterTab(tab_item.GetHWND()));
	}
	else
		return false;
}

bool TaskbarManager::SetTabOrder(const TaskbarTabItem &tab_item, const TaskbarTabItem &tab_item_insert_before)
{
	if (taskbar_list_)
	{
		return (S_OK == taskbar_list_->SetTabOrder(tab_item.GetHWND(), tab_item_insert_before.GetHWND()));
	}
	else
		return false;
}

bool TaskbarManager::SetTabActive(const TaskbarTabItem &tab_item)
{
	if (taskbar_list_)
	{
		return (S_OK == taskbar_list_->SetTabActive(tab_item.GetHWND(), parent_window_->GetHWND(), 0));
	}
	else
		return false;
}

HBITMAP TaskbarManager::GenerateBindControlBitmapWithForm(ui::Control *control)
{
	ASSERT( NULL != control);
	if ( NULL == control)
		return NULL;

	int window_width = 0, window_height = 0;
	RECT rc_wnd;
	bool check_wnd_size = false;
	if (::IsIconic(parent_window_->GetHWND())) //????????????????????????
	{
		WINDOWPLACEMENT placement{ sizeof(WINDOWPLACEMENT) };
		::GetWindowPlacement(parent_window_->GetHWND(), &placement);
		if (placement.flags == WPF_RESTORETOMAXIMIZED) //??????????????????????????????
		{
			MONITORINFO oMonitor = { sizeof(MONITORINFO) };
			::GetMonitorInfo(::MonitorFromWindow(parent_window_->GetHWND(), MONITOR_DEFAULTTONEAREST), &oMonitor);
			rc_wnd = oMonitor.rcWork;
		}
		else
		{
			rc_wnd = placement.rcNormalPosition;
			check_wnd_size = true; //??????????????????WINDOWPLACEMENT::rcNormalPosition?????????
		}
	}
	else
		::GetWindowRect(parent_window_->GetHWND(), &rc_wnd);
	window_width = rc_wnd.right - rc_wnd.left;
	window_height = rc_wnd.bottom - rc_wnd.top;
	if (window_width == 0 || window_height == 0)
		return nullptr;

	// 1.????????????dc
	auto render = GlobalManager::CreateRenderContext();
	render->Resize(window_width, window_height);

	// 2.???????????????????????????????????????dc
	render->BitBlt(0, 0, window_width, window_height, parent_window_->GetRenderContext()->GetDC());

	// 3.??????????????????????????????????????????dc???????????????????????????????????????
	UiRect rcPaint = control->GetPos();
	if (rcPaint.IsRectEmpty())
		return NULL;
	rcPaint.Intersect(UiRect(0, 0, window_width, window_height));

	// ???????????????????????????????????????????????????
	{
		AutoClip rectClip(render.get(), rcPaint);

		bool visible = control->IsInternVisible();
		control->SetInternVisible(true);
		control->Paint(render.get(), rcPaint);
		control->SetInternVisible(visible);
	}

	// 4.?????????????????????alpha??????
	render->RestoreAlpha(rcPaint);

	return render->DetachBitmap();
}

HBITMAP TaskbarManager::GenerateBindControlBitmap(ui::Control *control, const int dest_width, const int dest_height)
{
	ASSERT(dest_width > 0 && dest_height > 0 && NULL != control);
	if (dest_width <= 0 || dest_height <= 0 || NULL == control)
		return NULL;

	int window_width = 0, window_height = 0;
	RECT rc_wnd;
	bool check_wnd_size = false;
	if (::IsIconic(parent_window_->GetHWND())) //????????????????????????
	{
		WINDOWPLACEMENT placement{ sizeof(WINDOWPLACEMENT) };
		::GetWindowPlacement(parent_window_->GetHWND(), &placement);
		if (placement.flags == WPF_RESTORETOMAXIMIZED) //??????????????????????????????
		{
			MONITORINFO oMonitor = { sizeof(MONITORINFO) };
			::GetMonitorInfo(::MonitorFromWindow(parent_window_->GetHWND(), MONITOR_DEFAULTTONEAREST), &oMonitor);
			rc_wnd = oMonitor.rcWork;
		}
		else
		{
			rc_wnd = placement.rcNormalPosition;
			check_wnd_size = true; //??????????????????WINDOWPLACEMENT::rcNormalPosition?????????
		}
	}
	else
		::GetWindowRect(parent_window_->GetHWND(), &rc_wnd);
	window_width = rc_wnd.right - rc_wnd.left;
	window_height = rc_wnd.bottom - rc_wnd.top;
	if (window_width == 0 || window_height == 0)
		return nullptr;

	// 1.????????????dc
	auto render = GlobalManager::CreateRenderContext();
	render->Resize(window_width, window_height);

	// 2.??????????????????????????????????????????dc???????????????????????????????????????
	UiRect rcPaint = control->GetPos();
	if (rcPaint.IsRectEmpty())
		return NULL;
	rcPaint.Intersect(UiRect(0, 0, window_width, window_height));

	// ???????????????????????????????????????????????????
	{
		AutoClip rectClip(render.get(), rcPaint);

		bool visible = control->IsInternVisible();
		control->SetInternVisible(true);
		control->Paint(render.get(), rcPaint);
		control->SetInternVisible(visible);
	}

	// 3.?????????????????????alpha??????
	render->RestoreAlpha(rcPaint);

	// 4.?????????????????????
	UiRect rcControl = control->GetPos();
	return ResizeBitmap(dest_width, dest_height, render->GetDC(), rcControl.left, rcControl.top, rcControl.GetWidth(), rcControl.GetHeight());
}

HBITMAP TaskbarManager::ResizeBitmap(int dest_width, int dest_height, HDC src_dc, int src_x, int src_y, int src_width, int src_height)
{
	auto render = GlobalManager::CreateRenderContext();
	if (render->Resize(dest_width, dest_height))
	{
		int scale_width = 0;
		int scale_height = 0;

		float src_scale = (float)src_width / (float)src_height;
		float dest_scale = (float)dest_width / (float)dest_height;
		if (src_scale >= dest_scale)
		{
			scale_width = dest_width;
			scale_height = (int)(dest_width * (float)src_height / (float)src_width);
		}
		else
		{
			scale_height = dest_height;
			scale_width = (int)(dest_height * (float)src_width / (float)src_height);
		}

		render->AlphaBlend((dest_width - scale_width) / 2, (dest_height - scale_height) / 2, scale_width, scale_height, src_dc, src_x, src_y, src_width, src_height);
	}

	return render->DetachBitmap();
}

void TaskbarManager::OnTabItemClose(TaskbarTabItem &tab_item)
{
	ui::Control *control = tab_item.GetBindControl();
	if (control)
	{
		SessionBox *session_box = dynamic_cast<SessionBox*>(control);
		if (session_box)
		{
			parent_window_->CloseSessionBox(session_box->GetSessionId());
		}
	}
}

void TaskbarManager::OnTabItemClicked(TaskbarTabItem &tab_item)
{
	ui::Control *control = tab_item.GetBindControl();
	if (control)
	{
		SessionBox *session_box = dynamic_cast<SessionBox*>(control);
		if (session_box)
		{
			parent_window_->SetActiveSessionBox(session_box->GetSessionId());
		}
	}
}

}