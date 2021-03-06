#include "stdafx.h"
#include "broad_form.h"
#include "module/emoji/richedit_util.h"
#include "module/session/session_manager.h"
#include "callback/team/team_callback.h"
#include "module/session/session_util.h"


using namespace ui;
namespace nim_comp
{
const LPCTSTR BroadForm::kClassName = L"BroadForm";

BroadForm::BroadForm()
{

}

BroadForm::~BroadForm()
{

}

std::wstring BroadForm::GetSkinFolder()
{
	return L"broad";
}

std::wstring BroadForm::GetSkinFile()
{
	return L"broad.xml";
}

std::wstring BroadForm::GetWindowClassName() const
{
	return kClassName;
}

std::wstring BroadForm::GetWindowId() const
{
	return kClassName;
}

UINT BroadForm::GetClassStyle() const
{
	return (UI_CLASSSTYLE_FRAME | CS_DBLCLKS);
}
void BroadForm::InitWindow()
{
	m_pRoot->AttachBubbledEvent(ui::kEventAll, nbase::Bind(&BroadForm::Notify, this, std::placeholders::_1));
	m_pRoot->AttachBubbledEvent(ui::kEventClick, nbase::Bind(&BroadForm::OnClicked, this, std::placeholders::_1));

	re_title_ = (RichEdit*) FindControl(L"re_title");
	error_1_ = (Label*) FindControl(L"error_1");

	re_content_ = (RichEdit*) FindControl(L"re_content");
	error_2_ = (Label*) FindControl(L"error_2");

	re_title_->SetLimitText(20);
	re_content_->SetLimitText(300);

	btn_commit_ = (Button*) FindControl(L"commit");
}

LRESULT BroadForm::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN && wParam == 'V')
	{
		if (::GetKeyState(VK_CONTROL) < 0)
		{
			if (re_title_->IsFocused())
			{
				re_title_->PasteSpecial(CF_TEXT);
				return 1;
			}
			else if (re_content_->IsFocused())
			{
				re_content_->PasteSpecial(CF_TEXT);
				return 1;
			}
		}
	}
	return __super::HandleMessage(uMsg, wParam, lParam);
}

void BroadForm::SetTid( const std::string &tid )
{
	tid_ = tid;
}

bool BroadForm::Notify(ui::EventArgs* arg)
{
	std::wstring name = arg->pSender->GetName();
	if(arg->Type == kEventTextChange)
	{
		if(name == L"re_title")
			error_1_->SetVisible(false);
		else if(name == L"re_content")
			error_2_->SetVisible(false);
	}
	return false;
}

bool BroadForm::OnClicked( ui::EventArgs* arg )
{
	std::wstring name = arg->pSender->GetName();
	if(name == L"commit")
	{
		
		std::string title;
		{
			std::wstring str = GetRichText(re_title_);
			StringHelper::Trim(str);
			if( str.empty() )
			{
				error_1_->SetText(MutiLanSupport::GetInstance()->GetStringViaID(L"STRID_TEAM_BOARD_TITLE_NON_EMPTY"));
				error_1_->SetVisible(true);
				return false;
			}
			title = nbase::UTF16ToUTF8(str);
		}
		
		std::string content;
		{
			std::wstring str = GetRichText(re_content_);
			StringHelper::Trim(str);
			if( str.empty() )
			{
				error_2_->SetText(MutiLanSupport::GetInstance()->GetStringViaID(L"STRID_TEAM_BOARD_CONTENT_NON_EMPTY"));
				error_2_->SetVisible(true);
				return false;
			}
			content = nbase::UTF16ToUTF8(str);
		}

		btn_commit_->SetEnabled(false);

		Json::Value broad;
		broad["title"] = title;
		broad["content"] = content;
		broad["creator"] = LoginManager::GetInstance()->GetAccount();
		broad["time"] = nbase::Time::Now().ToTimeT();

		Json::Value broads;
		broads.append(broad);

		Json::FastWriter writer;
		nim::TeamInfo param;
		param.SetAnnouncement(writer.write(broads));
		param.SetTeamID(tid_);

		nim::Team::UpdateTeamInfoAsync(tid_, param, nbase::Bind(&BroadForm::OnUpdateBroadCb, this, std::placeholders::_1));
	}
	return false;
}

void BroadForm::OnUpdateBroadCb(const nim::TeamEvent& team_event)
{
	QLOG_APP(L"update broad: code={0} notify_id={1} tid={2}") << team_event.res_code_ << team_event.notification_id_ << team_event.team_id_;
	
	if (team_event.res_code_ == 200)
	{
		SessionBox* session = SessionManager::GetInstance()->FindSessionBox(tid_);
		if (session)
			session->InvokeGetTeamInfo();

		this->Close();
	}
	else
	{
		btn_commit_->SetEnabled(true);
		ShowMsgBox(m_hWnd, MsgboxCallback(), L"STRID_TEAM_BOARD_PUBLISH_FAIL");
	}
}
}