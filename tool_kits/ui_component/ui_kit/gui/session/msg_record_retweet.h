#pragma once
#include "gui/session/session_box.h"

namespace nim_comp
{
/** @class MsgRecordRetweetForm
  * @brief 消息记录窗体
  * @copyright (c) 2019, NetEase Inc. All rights reserved
  * @author HK
  * @date 2019/10/18
  */
class MsgRecordRetweetForm : public WindowEx
{
public:
	MsgRecordRetweetForm();
	~MsgRecordRetweetForm();
	
	//覆盖虚函数
	virtual std::wstring GetSkinFolder() override;
	virtual std::wstring GetSkinFile() override;
	virtual std::wstring GetWindowClassName() const override;
	virtual std::wstring GetWindowId() const override;
	virtual UINT GetClassStyle() const override;
	
	/**
	* 窗口初始化函数
	* @return void	无返回值
	*/
	virtual void InitWindow() override;

	/**
	* 拦截并处理底层窗体消息
	* @param[in] uMsg 消息类型
	* @param[in] wParam 附加参数
	* @param[in] lParam 附加参数
	* @return LRESULT 处理结果
	*/
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	/**
	* 处理窗口被销毁的消息
	* @param[in] hWnd 窗口句柄
	* @return void	无返回值
	*/
	virtual void OnFinalMessage(HWND hWnd) override;
	
	/**
	* 处理所有控件的所有消息
	* @param[in] msg 消息的相关信息
	* @return bool true 继续传递控件消息，false 停止传递控件消息
	*/
	bool Notify(ui::EventArgs* msg);

	/**
	* 处理所有控件单击消息
	* @param[in] msg 消息的相关信息
	* @return bool true 继续传递控件消息，false 停止传递控件消息
	*/
	bool OnClicked(ui::EventArgs* msg);

	//////////////////////////////////////////////////////////////////////////
	//与消息相关的操作	
	/**
	* 加载消息记录，把一组历史消息添加到聊天框的开头位置
	* @param[in] msg	历史消息数据
	* @return void 无返回值
	*/
	void ShowMsgs(const std::list<nim::IMMessage> &msg);

	/**
	* 对方发送文件过来，获取是否成功下载文件的回调函数
	* @param[in] code	错误码(200代表无错误)
	* @param[in] file_path 下载资源文件本地绝对路径
	* @param[in] sid	消息所属的会话id
	* @param[in] cid	消息id
	* @return void 无返回值
	*/
	void OnDownloadCallback(nim::NIMResCode code, const std::string& file_path, const std::string& sid, const std::string& cid);

	//////////////////////////////////////////////////////////////////////////
	//音频相关的操作

	/**
	* 单击了语音消息后，开始播放语音的回调函数
	* @param[in] cid	消息id
	* @param[in] code	错误码(200代表无错误)
	* @return void 无返回值
	*/
	void OnPlayAudioCallback(const std::string &cid, int code);

	/**
	* 第二次单击了语音消息后，停止播放语音的回调函数
	* @param[in] cid	消息id
	* @param[in] code	错误码(200代表无错误)
	* @return void 无返回值
	*/
	void OnStopAudioCallback(const std::string &cid, int code);
	
	void SetInfo(const std::shared_ptr<MultiMessageRetweetData>& data, const std::string& session_id,nim::NIMSessionType session_type);

	void ShowMessageLogs();
private:
	/**
	* 响应窗体最大化消息
	* @param[in] max 是否最大化显示
	* @return void	无返回值
	*/
	void OnWndSizeMax(bool max);

	/**
	* 获取消息记录展示的发送者名字
	* @param[in] msg	消息数据
	* @param[out] show_name	展示的名字
	* @return bool 是否存在可展示的名字
	*/
	bool GetUserShowName(const nim::IMMessage &msg, std::string& show_name);

	void ShowMsg(const nim::IMMessage &msg, bool first, bool show_time);
public:
	static const LPCTSTR kClassName;
private:
	ui::Label*		label_title_;
	ui::Button*		btn_max_restore_;
	
	ui::ListBox*	msg_list_;
private:
	std::string		session_id_;
	nim::NIMSessionType		session_type_;	
	ui::Label* progress_;
	ui::Box* progress_box_;
	typedef std::map<std::string,MsgBubbleItem*> IdBubblePair;
	IdBubblePair	id_bubble_pair_;
	std::shared_ptr<MultiMessageRetweetData> data_;
	std::map<std::string, nim::TeamMemberProperty> team_member_info_list_; //（会话类型是群聊时有效）保存用户和群相关的信息
};
}