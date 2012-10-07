///
/// @file WebViewListener.h
///
/// @brief The header for the WebViewListener events.
///
/// @author
///
/// This file is a part of Awesomium, a Web UI bridge for native apps.
///
/// Website: <http://www.awesomium.com>
///
/// Copyright (C) 2012 Khrona. All rights reserved. Awesomium is a
/// trademark of Khrona.
///
#ifndef AWESOMIUM_WEB_VIEW_LISTENER_H_
#define AWESOMIUM_WEB_VIEW_LISTENER_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebMenuItem.h>
#include <Awesomium/WebStringArray.h>

namespace Awesomium {

class WebURL;
class WebString;
class WebView;

///
/// An enumeration of all the possible web cursors.
///
/// @see WebViewListener::View::OnChangeCursor
///
enum Cursor {
  kCursor_Pointer,
  kCursor_Cross,
  kCursor_Hand,
  kCursor_IBeam,
  kCursor_Wait,
  kCursor_Help,
  kCursor_EastResize,
  kCursor_NorthResize,
  kCursor_NorthEastResize,
  kCursor_NorthWestResize,
  kCursor_SouthResize,
  kCursor_SouthEastResize,
  kCursor_SouthWestResize,
  kCursor_WestResize,
  kCursor_NorthSouthResize,
  kCursor_EastWestResize,
  kCursor_NorthEastSouthWestResize,
  kCursor_NorthWestSouthEastResize,
  kCursor_ColumnResize,
  kCursor_RowResize,
  kCursor_MiddlePanning,
  kCursor_EastPanning,
  kCursor_NorthPanning,
  kCursor_NorthEastPanning,
  kCursor_NorthWestPanning,
  kCursor_SouthPanning,
  kCursor_SouthEastPanning,
  kCursor_SouthWestPanning,
  kCursor_WestPanning,
  kCursor_Move,
  kCursor_VerticalText,
  kCursor_Cell,
  kCursor_ContextMenu,
  kCursor_Alias,
  kCursor_Progress,
  kCursor_NoDrop,
  kCursor_Copy,
  kCursor_None,
  kCursor_NotAllowed,
  kCursor_ZoomIn,
  kCursor_ZoomOut,
  kCursor_Grab,
  kCursor_Grabbing,
  kCursor_Custom
};

///
/// Used with WebViewListener::View::OnChangeFocus
///
/// @note  You should generally forward keyboard events to the active WebView
/// whenever one of the following element types are focused: input, text-input,
/// editable-content, or plugin
///
enum FocusedElementType {
  kFocusedElementType_None = 0,         ///< Nothing is focused
  kFocusedElementType_Text,             ///< A text-node is focused
  kFocusedElementType_Link,             ///< A link is focused
  kFocusedElementType_Input,            ///< An input element is focused
  kFocusedElementType_TextInput,        ///< A text-input element is focused
  kFocusedElementType_EditableContent,  ///< Some editable content is focused
  kFocusedElementType_Plugin,           ///< A plugin (eg, Flash) is focused
  kFocusedElementType_Other,            ///< Some other element is focused
};

/// Used with WebViewListener::Process::OnCrashed
enum TerminationStatus {
  kTerminationStatus_Normal,       ///< Zero Exit Status
  kTerminationStatus_Abnormal,     ///< Non-Zero exit status
  kTerminationStatus_Killed,       ///< e.g. SIGKILL or Task Manager kill
  kTerminationStatus_Crashed,      ///< e.g. Segmentation Fault
  kTerminationStatus_StillRunning  ///< Process hasn't exited yet
};

/// Used with WebViewListener::InputMethodEditor::OnUpdate
enum TextInputType {
  kTextInputType_None,      ///< Input is not editable, no IME should be displayed.
  kTextInputType_Text,      ///< Input is editable, IME should be displayed.
  kTextInputType_Password,  ///< Input is a password box, IME should only be displayed if suitable.
  kTextInputType_Search,    ///< Input is a search box, IME should only be displayed if suitable.
  kTextInputType_Email,     ///< Input is an email input, IME should only be displayed if suitable.
  kTextInputType_Number,    ///< Input is a number input, IME should only be displayed if suitable.
  kTextInputType_Telephone, ///< Input is a telephone input, IME should only be displayed if suitable.
  kTextInputType_URL,       ///< Input is a URL input, IME should only be displayed if suitable.
};

/// Used with WebFileChooserInfo
enum WebFileChooserMode {
  kWebFileChooserMode_Open,         ///< Select a file (file should exist)
  kWebFileChooserMode_OpenMultiple, ///< Select multiple files
  kWebFileChooserMode_OpenFolder,   ///< Select a folder (folder should exist)
  kWebFileChooserMode_Save,         ///< Select a file to save to
};

/// Used with WebViewListener::Dialog::OnShowFileChooser
#pragma pack(push)
#pragma pack(1)
struct OSM_EXPORT WebFileChooserInfo {
  WebFileChooserMode mode;       ///< The type of dialog to display
  WebString title;               ///< Title of the dialog
  WebString default_file_name;   ///< Suggested file name for the dialog
  WebStringArray accept_types;   ///< Valid mime types
};
#pragma pack(pop)

/// Used with WebViewListener::Menu::OnShowPopupMenu
#pragma pack(push)
#pragma pack(1)
struct OSM_EXPORT WebPopupMenuInfo {
  Awesomium::Rect bounds;  ///< The location to display the menu
  int item_height;         ///< The height of each menu item
  double item_font_size;   ///< The font-size of each menu item
  int selected_item;       ///< The index of the currently-selected item
  WebMenuItemArray items;  ///< The actual menu items
  bool right_aligned;      ///< Whether or not the menu is right-aligned
};
#pragma pack(pop)

/// Used with WebContextMenuInfo
enum MediaType {
  kMediaType_None,
  kMediaType_Image,
  kMediaType_Video,
  kMediaType_Audio,
  kMediaType_File,
  kMediaType_Plugin
};

/// Used with WebContextMenuInfo
enum MediaState {
  kMediaState_None = 0x0,
  kMediaState_Error = 0x1,
  kMediaState_Paused = 0x2,
  kMediaState_Muted = 0x4,
  kMediaState_Loop = 0x8,
  kMediaState_CanSave = 0x10,
  kMediaState_HasAudio = 0x20,
  kMediaState_HasVideo = 0x40
};

/// Used with WebContextMenuInfo
enum CanEditFlags {
  kCan_EditNothing = 0x0,
  kCan_Undo = 0x1,
  kCan_Redo = 0x2,
  kCan_Cut = 0x4,
  kCan_Copy = 0x8,
  kCan_Paste = 0x10,
  kCan_Delete = 0x20,
  kCan_SelectAll = 0x40,
};

/// Used with WebViewListener::Menu::OnShowContextMenu
#pragma pack(push)
#pragma pack(1)
struct OSM_EXPORT WebContextMenuInfo {
  int pos_x;                 ///< The x-coordinate of the menu's position.
  int pos_y;                 ///< The y-coordinate of the menu's position.
  MediaType media_type;      ///< The type of media (if any) that was clicked.
  int media_state;           ///< The state of the media (if any).
  WebURL link_url;           ///< The URL of the link (if any).
  WebURL src_url;            ///< The URL of the media (if any).
  WebURL page_url;           ///< The URL of the web-page.
  WebURL frame_url;          ///< The URL of the frame.
  int64 frame_id;            ///< The ID of the frame.
  WebString selection_text;  ///< The selected text (if any).
  bool is_editable;          ///< Whether or not this node is editable.
  int edit_flags;            ///< Which edit actions can be performed.
};
#pragma pack(pop)

/// Used with WebViewListener::Dialog::OnShowLoginDialog
#pragma pack(push)
#pragma pack(1)
struct OSM_EXPORT WebLoginDialogInfo {
  int request_id;         ///< The unique ID of the request.
  WebString request_url;  ///< The URL of the web-page requesting login.
  bool is_proxy;          ///< Whether or not this is a proxy auth.
  WebString host;         ///< The hostname of the server.
  unsigned short port;    ///< The port of the server.
  WebString scheme;       ///< The scheme of the server.
  WebString realm;        ///< The realm of the server.
};
#pragma pack(pop)

/// Namespace containing all the WebView event-listener interfaces.
namespace WebViewListener {

///
/// @brief  An interface that you can use to handle all View-related events
///         for a certain WebView.
///
/// @note  See WebView::set_view_listener
///
class OSM_EXPORT View {
 public:
  /// This event occurs when the page title has changed.
  virtual void OnChangeTitle(Awesomium::WebView* caller,
                             const Awesomium::WebString& title) = 0;

  /// This event occurs when the page URL has changed.
  virtual void OnChangeAddressBar(Awesomium::WebView* caller,
                                  const Awesomium::WebURL& url) = 0;

  /// This event occurs when the tooltip text has changed. You
  /// should hide the tooltip when the text is empty.
  virtual void OnChangeTooltip(Awesomium::WebView* caller,
                               const Awesomium::WebString& tooltip) = 0;

  /// This event occurs when the target URL has changed. This
  /// is usually the result of hovering over a link on a page.
  virtual void OnChangeTargetURL(Awesomium::WebView* caller,
                                 const Awesomium::WebURL& url) = 0;

  /// This event occurs when the cursor has changed. This is
  /// is usually the result of hovering over different content.
  virtual void OnChangeCursor(Awesomium::WebView* caller,
                              Awesomium::Cursor cursor) = 0;

  /// This event occurs when the focused element changes on the page.
  /// This is usually the result of textbox being focused or some other
  /// user-interaction event.
  virtual void OnChangeFocus(Awesomium::WebView* caller,
                             Awesomium::FocusedElementType focused_type) = 0;

  /// This event occurs when a WebView creates a new child WebView
  /// (usually the result of window.open or an external link). It
  /// is your responsibility to display this child WebView in your
  /// application. You should call Resize on the child WebView
  /// immediately after this event to make it match your container
  /// size.
  ///
  /// If this is a child of a Windowed WebView, you should call
  /// WebView::set_parent_window on the new view immediately within
  /// this event.
  ///
  virtual void OnShowCreatedWebView(Awesomium::WebView* caller,
                                    Awesomium::WebView* new_view,
                                    const Awesomium::WebURL& opener_url,
                                    const Awesomium::WebURL& target_url,
                                    const Awesomium::Rect& initial_pos,
                                    bool is_popup) = 0;

 protected:
  virtual ~View() {}
};

///
/// @brief  An interface that you can use to handle all page-loading
///         events for a certain WebView.
///
/// @note: See WebView::SetLoadListener
///
class OSM_EXPORT Load {
 public:
  /// This event occurs when the page begins loading a frame.
  virtual void OnBeginLoadingFrame(Awesomium::WebView* caller,
                                   int64 frame_id,
                                   bool is_main_frame,
                                   const Awesomium::WebURL& url,
                                   bool is_error_page) = 0;

  /// This event occurs when a frame fails to load. See error_desc
  /// for additional information.
  virtual void OnFailLoadingFrame(Awesomium::WebView* caller,
                                  int64 frame_id,
                                  bool is_main_frame,
                                  const Awesomium::WebURL& url,
                                  int error_code,
                                  const Awesomium::WebString& error_desc) = 0;

  /// This event occurs when the page finishes loading a frame.
  /// The main frame always finishes loading last for a given page load.
  virtual void OnFinishLoadingFrame(Awesomium::WebView* caller,
                                    int64 frame_id,
                                    bool is_main_frame,
                                    const Awesomium::WebURL& url) = 0;

  /// This event occurs when the DOM has finished parsing and the
  /// window object is available for JavaScript execution.
  virtual void OnDocumentReady(Awesomium::WebView* caller,
                               const Awesomium::WebURL& url) = 0;

 protected:
  virtual ~Load() {}
};

///
/// @brief  An interface that you can use to handle all process-related
///         events for a certain WebView.
///
/// Each WebView has an associated "render" process, you can use these events
/// to recover from crashes or hangs).
///
/// @note  See WebView::set_process_listener
///
class OSM_EXPORT Process {
 public:
  /// This event occurs when the process hangs.
  virtual void OnUnresponsive(Awesomium::WebView* caller) = 0;

  /// This event occurs when the process becomes responsive after
  /// a hang.
  virtual void OnResponsive(Awesomium::WebView* caller) = 0;

  /// This event occurs when the process crashes.
  virtual void OnCrashed(Awesomium::WebView* caller,
                         Awesomium::TerminationStatus status) = 0;
 protected:
  virtual ~Process() {}
};

///
/// @brief  An interface that you can use to handle all menu-related events
///         for a certain WebView.
///
/// @note  See WebView::set_menu_listener
///
class OSM_EXPORT Menu {
 public:
  ///
  /// This event occurs when the page requests to display a dropdown
  /// (popup) menu. This is usually the result of a user clicking on
  /// a "select" HTML input element. It is your responsibility to
  /// display this menu in your application. This event is not modal.
  ///
  /// @see WebView::DidSelectPopupMenuItem
  /// @see WebView::DidCancelPopupMenu
  ///
  virtual void OnShowPopupMenu(Awesomium::WebView* caller,
                               const WebPopupMenuInfo& menu_info) = 0;

  ///
  /// This event occurs when the page requests to display a context menu.
  /// This is usually the result of a user right-clicking somewhere on the
  /// page. It is your responsibility to display this menu in your
  /// application and perform the selected actions. This event is not modal.
  ///
  virtual void OnShowContextMenu(Awesomium::WebView* caller,
                                 const WebContextMenuInfo& menu_info) = 0;
 protected:
  virtual ~Menu() {}
};

///
/// @brief  An interface that you can use to handle all dialog-related events
///         for a certain WebView.
///
/// @see  WebView::set_dialog_listener
///
class OSM_EXPORT Dialog {
 public:
  ///
  /// This event occurs when the page requests to display a file chooser
  /// dialog. This is usually the result of a user clicking on an HTML
  /// input element with `type='file`. It is your responsibility to display
  /// this menu in your application. This event is not modal.
  ///
  /// @see WebView::DidChooseFiles
  ///
  virtual void OnShowFileChooser(Awesomium::WebView* caller,
                                 const WebFileChooserInfo& chooser_info) = 0;

  ///
  /// This event occurs when the page needs authentication from the user (for
  /// example, Basic HTTP Auth, NTLM Auth, etc). It is your responsibility to
  /// display a dialog so that users can input their username and password.
  /// This event is not modal.
  ///
  /// @see WebView::DidLogin
  /// @see WebView::DidCancelLogin
  ///
  virtual void OnShowLoginDialog(Awesomium::WebView* caller,
                                 const WebLoginDialogInfo& dialog_info) = 0;

 protected:
  virtual ~Dialog() {}
};

///
/// @brief  An interface that you can use to handle all print-related events
///         for a certain WebView.
///
/// @see  WebView::set_print_listener
///
class OSM_EXPORT Print {
 public:
  ///
  /// This event occurs when the page requests to print itself. (Usually
  /// the result of `window.print()` being called from JavaScript.) It is
  /// your responsiblity to print the WebView to a file and handle the
  /// actual device printing.
  ///
  /// @see  WebView::PrintToFile
  ///
  virtual void OnRequestPrint(Awesomium::WebView* caller) = 0;

  ///
  /// This event occurs when WebView::PrintToFile fails. Typically because of
  /// bad printer configuration or invalid output path (it must be writable).
  ///
  /// @param  request_id  The unique request ID (returned from
  ///                     WebView::PrintToFile earlier).
  ///
  virtual void OnFailPrint(Awesomium::WebView* caller,
                           int request_id) = 0;

  ///
  /// This event occurs when WebView::PrintToFile succeeds.
  ///
  /// @param  request_id  The unique request ID (returned from
  ///                     WebView::PrintToFile earlier).
  ///
  /// @param  file_list  The list of file-paths written. There may be multiple
  ///                    files written if split_pages_into_multiple_files was
  ///                    set to true in PrintConfig.
  ///
  virtual void OnFinishPrint(Awesomium::WebView* caller,
                             int request_id,
                             const WebStringArray& file_list) = 0;

 protected:
  virtual ~Print() {}
};

///
/// @brief  An interface that you can use to handle all download-related events
///         for a certain WebView.
///
/// @see  WebView::set_download_listener
///
class OSM_EXPORT Download {
 public:
  ///
  /// This event occurs when the page requests to begin downloading a certain
  /// file. It is your responsiblity to call WebView::DidChooseDownloadPath or
  /// WebView::DidCancelDownload as a result of this event.
  ///
  /// @param  download_id  The unique ID of the download.
  ///
  /// @param  url  The URL that initiated the download.
  ///
  /// @param  suggested_filename  The suggested name for the file.
  ///
  /// @param  mime_type  The mime type of the file.
  ///
  virtual void OnRequestDownload(Awesomium::WebView* caller,
                                 int download_id,
                                 const Awesomium::WebURL& url,
                                 const Awesomium::WebString& suggested_filename,
                                 const Awesomium::WebString& mime_type) = 0;

  ///
  /// This event occurs when the progress of the download is updated.
  ///
  /// @param  download_id  The unique ID of the download.
  ///
  /// @param  total_bytes  The total number of bytes (may be 0 if unknown).
  ///
  /// @param  received_bytes  The number of bytes received so far.
  ///
  /// @param  current_speed  The current speed in bytes per second.
  ///
  virtual void OnUpdateDownload(Awesomium::WebView* caller,
                                int download_id,
                                int64 total_bytes,
                                int64 received_bytes,
                                int64 current_speed) = 0;

  ///
  /// This event occurs when the download is finished.
  ///
  /// @param  download_id  The unique ID of the download.
  ///
  /// @param  url  The URL that initiated the download.
  ///
  /// @param  saved_path  The path that the download was saved to.
  ///
  virtual void OnFinishDownload(Awesomium::WebView* caller,
                                int download_id,
                                const Awesomium::WebURL& url,
                                const Awesomium::WebString& saved_path) = 0;

 protected:
  virtual ~Download() {}
};

///
/// @brief  An interface that you can use to handle all IME-related events
///         for a certain WebView.
///
/// @see  WebView::set_input_method_editor_listener
///
class OSM_EXPORT InputMethodEditor {
 public:
  ///
  /// You should handle this message if you are displaying your
  /// own IME (input method editor) widget.
  ///
  /// This event is fired whenever the user does something that may change
  /// the position, visiblity, or type of the IME Widget. This event is only
  /// active when IME is active (please see WebView::ActivateIME).
  ///
  /// @param  caller  The WebView that fired this event.
  ///
  /// @param  type  The type of IME widget that should be displayed (if any).
  ///
  /// @param  caret_x  The x-position of the caret (relative to the View).
  /// @param  caret_y  The y-position of the caret (relative to the View).
  ///
  virtual void OnUpdateIME(Awesomium::WebView* caller,
                           Awesomium::TextInputType type,
                           int caret_x,
                           int caret_y) = 0;

  ///
  /// This event is fired when the page cancels the IME composition.
  ///
  virtual void OnCancelIME(Awesomium::WebView* caller) = 0;

  ///
  /// This event is fired when the page changes the displayed range
  /// of the IME composition.
  ///
  virtual void OnChangeIMERange(Awesomium::WebView* caller,
                                unsigned int start,
                                unsigned int end) = 0;

 protected:
  virtual ~InputMethodEditor() {}
};

}  // namespace WebViewListener

}  // namespace Awesomium

#endif  // AWESOMIUM_WEB_VIEW_LISTENER_H_
