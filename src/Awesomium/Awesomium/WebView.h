///
/// @file WebView.h
///
/// @brief The header for the WebView class.
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
#ifndef AWESOMIUM_WEB_VIEW_H_
#define AWESOMIUM_WEB_VIEW_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>
#include <Awesomium/WebStringArray.h>
#include <Awesomium/WebURL.h>
#include <Awesomium/Surface.h>
#include <Awesomium/WebKeyboardEvent.h>
#include <Awesomium/WebTouchEvent.h>
#include <Awesomium/WebViewListener.h>
#include <Awesomium/PrintConfig.h>
#include <Awesomium/JSValue.h>

namespace Awesomium {

class WebSession;

///
/// The three different mouse-button types.
///
/// @see WebView::InjectMouseDown
///
enum MouseButton {
  kMouseButton_Left = 0,
  kMouseButton_Middle,
  kMouseButton_Right
};

///
/// @brief A WebView is similar to a tab in a browser. You load pages into a
/// WebView, interact with it, and display it however you want.
/// 
/// Unless otherwise specified, all methods are asynchronous and must
/// be called from the main thread. Each WebView is rendered in its own
/// child-process and most API calls must be proxied through IPC messages.
/// 
/// @see WebCore::CreateWebView
///
class OSM_EXPORT WebView {
 public:
  ///
  /// Destroy this WebView immediately. If you never call this, the WebView
  /// will be destroyed upon calling WebCore::Shutdown.
  ///
  virtual void Destroy() = 0;

  ///
  /// Register a listener to handle view-related events.
  ///
  /// @param  listener  The instance to register (you retain ownership).
  ///
  virtual void set_view_listener(WebViewListener::View* listener) = 0;

  ///
  /// Register a listener to handle page-load events.
  ///
  /// @param  listener  The instance to register (you retain ownership).
  ///
  virtual void set_load_listener(WebViewListener::Load* listener) = 0;

  ///
  /// Register a listener to handle process-related events (crash, hangs, etc).
  ///
  /// @param  listener  The instance to register (you retain ownership).
  ///
  virtual void set_process_listener(WebViewListener::Process* listener) = 0;

  ///
  /// Register a listener to handle the display of various menus.
  ///
  /// @param  listener  The instance to register (you retain ownership).
  ///
  virtual void set_menu_listener(WebViewListener::Menu* listener) = 0;

  ///
  /// Register a listener to handle the display of various dialogs.
  ///
  /// @param  listener  The instance to register (you retain ownership).
  ///
  virtual void set_dialog_listener(WebViewListener::Dialog* listener) = 0;

  ///
  /// Register a listener to handle printing-related events.
  ///
  /// @param  listener  The instance to register (you retain ownership).
  ///
  virtual void set_print_listener(WebViewListener::Print* listener) = 0;

  /// Get the current view-event listener (may be NULL).
  virtual WebViewListener::View* view_listener() = 0;

  /// Get the current load-event listener (may be NULL).
  virtual WebViewListener::Load* load_listener() = 0;

  /// Get the current process-event listener (may be NULL).
  virtual WebViewListener::Process* process_listener() = 0;

  /// Get the current menu-event listener (may be NULL).
  virtual WebViewListener::Menu* menu_listener() = 0;

  /// Get the current dialog-event listener (may be NULL).
  virtual WebViewListener::Dialog* dialog_listener() = 0;

  /// Get the current print-event listener (may be NULL).
  virtual WebViewListener::Print* print_listener() = 0;

  ///
  /// Begin loading a certain URL asynchronously.
  ///
  /// @param  url  The URL to begin loading.
  ///
  /// @note  The page is not guaranteed to be loaded after this method returns.
  ///        You should use WebViewListener::Load::OnFinishLoadingFrame to do
  ///        something after the page is loaded.
  ///
  virtual void LoadURL(const WebURL& url) = 0;

  ///
  /// Go back one page in history.
  ///
  virtual void GoBack() = 0;

  ///
  /// Go forward one page in history.
  ///
  virtual void GoForward() = 0;

  ///
  /// Go to a specific offset in history (for example, -1 would
  /// go back one page).
  ///
  /// @param  The offset to go to.
  ///
  virtual void GoToHistoryOffset(int offset) = 0;

  ///
  /// Stop all page loads.
  ///
  virtual void Stop() = 0;

  ///
  /// Reload the current page.
  ///
  /// @param  ignore_cache  Whether or not we force cached resources to
  ///                       to be reloaded as well.
  ///
  virtual void Reload(bool ignore_cache) = 0;

  ///
  /// Check whether or not we can go back in history.
  ///
  virtual bool CanGoBack() = 0;

  ///
  /// Check whether or not we can go forward in history.
  ///
  virtual bool CanGoForward() = 0;

  /// 
  /// Get the current rendering Surface. May be NULL.
  /// 
  /// @note: If you never call WebCore::SetSurfaceFactory, the returned
  ///        Surface will always be of type BitmapSurface.
  ///
  /// @see WebCore::set_surface_factory
  /// 
  virtual Surface* surface() = 0;

  /// Get the current page URL.
  virtual WebURL url() = 0;

  /// Get the current page title.
  virtual WebString title() = 0;

  /// Get the session associated with this WebView.
  virtual WebSession* session() = 0;

  /// Check whether or not any page resources are loading.
  virtual bool IsLoading() = 0;

  /// Check whether or not the WebView process has crashed.
  virtual bool IsCrashed() = 0;

  ///
  /// Resize to certain pixel dimensions (will trigger a new Surface to
  /// be created).
  ///
  /// @param  width  The width in pixels.
  ///
  /// @param  height  The height in pixels.
  ///
  virtual void Resize(int width, int height) = 0;

  ///
  /// Set the background of the view to be transparent. You must call this
  /// if you intend to preserve the transparency of a page (eg, your
  /// body element has "background-color: transparent;" or some other
  /// semi-translucent background). Please note that the alpha channel is
  /// premultiplied.
  ///
  /// If you never call this, the view will have an opaque, white background
  /// by default.
  ///
  /// @param  is_transparent  Whether or not the view should support
  ///                         transparency.
  ///
  virtual void SetTransparent(bool is_transparent) = 0;

  ///
  /// Whether or not the view supports transparency.
  ///
  virtual bool IsTransparent() = 0;

  ///
  /// Pause the renderer. All rendering is done asynchronously in a separate
  /// process so you should call this when your view is hidden to save
  /// some CPU cycles.
  ///
  virtual void PauseRendering() = 0;

  ///
  /// Resume the renderer (will force a full repaint).
  ///
  /// @see WebView::PauseRendering
  ///
  virtual void ResumeRendering() = 0;

  ///
  /// Give the appearance of input focus. You should call this whenever the
  /// view gains focus.
  ///
  /// @note If you fail to see a blinking caret when you select a textbox,
  ///       it's usually because you forgot to call this method.
  ///
  virtual void Focus() = 0;

  ///
  /// Remove the appearance of input focus. You should call this whenever the
  /// view loses focus.
  ///
  virtual void Unfocus() = 0;

  ///
  /// Get the type of the currently-focused element. This is useful for determining if
  /// the WebView should capture keyboard events. If no element is focused, this will
  /// return kFocusedElementType_None.
  ///
  /// @see FocusedElementType
  ///
  virtual FocusedElementType focused_element_type() = 0;

  ///
  /// Passes a mouse-move event to the view.
  ///
  /// @param  x  The x-coordinate of the current mouse position
  ///
  /// @param  y  The y-coordinate of the current mouse position
  ///
  /// @note All coordinates should be localized to the view. All values are in
  ///       pixels, the origin (0,0) begins at the top-left corner of the view,
  ///       positive-y values are "down" and positive-x values are "right".
  ///
  virtual void InjectMouseMove(int x, int y) = 0;

  ///
  /// Passes a mouse-down event using the last mouse-move coordinates.
  ///
  /// @param  button  The button that was pressed.
  ///
  virtual void InjectMouseDown(MouseButton button) = 0;

  ///
  /// Passes a mouse-up event using the last mouse-move coordinates.
  ///
  /// @param  button  The button that was released.
  ///
  virtual void InjectMouseUp(MouseButton button) = 0;

  ///
  /// Passes a mouse-wheel event.
  ///
  /// @param  scroll_vert  The amount of pixels to scroll vertically by.
  ///
  /// @param  scroll_horz  The amount of pixels to scroll horizontally by.
  ///
  virtual void InjectMouseWheel(int scroll_vert, int scroll_horz) = 0;

  ///
  /// Passes a keyboard event.
  ///
  /// @param  key_event  The keyboard event.
  ///
  virtual void InjectKeyboardEvent(const WebKeyboardEvent& key_event) = 0;

  ///
  /// Passes a multi-touch event.
  ///
  /// @param  touch_event  The multi-touch event.
  ///
  virtual void InjectTouchEvent(const WebTouchEvent& touch_event) = 0;

  ///
  /// Undo the last 'edit' operation. (Similar to CTRL+Z).
  ///
  virtual void Undo() = 0;

  ///
  /// Redo the last 'edit' operation. (Similar to CTRL+Y).
  ///
  virtual void Redo() = 0;

  ///
  /// Performs a 'cut' operation using the system clipboard.
  ///
  virtual void Cut() = 0;

  ///
  /// Performs a 'copy' operation using the system clipboard.
  ///
  virtual void Copy() = 0;

  ///
  /// Attempt to copy an image on the page to the system clipboard.
  /// This is meant to be used with Menu::OnShowContextMenu.
  ///
  /// @param  x  The x-coordinate.
  ///
  /// @param  y  The y-coordinate.
  ///
  /// @note All coordinates should be localized to the view. All values are in
  ///       pixels, the origin (0,0) begins at the top-left corner of the view,
  ///       positive-y values are "down" and positive-x values are "right".
  ///
  virtual void CopyImageAt(int x, int y) = 0;

  ///
  /// Performs a 'paste' operation using the system clipboard.
  ///
  virtual void Paste() = 0;

  ///
  /// Performs a 'paste' operation using the system clipboard while attempting
  /// to preserve any styles of the original text.
  ///
  virtual void PasteAndMatchStyle() = 0;

  ///
  /// Performs a 'select all' operation.
  ///
  virtual void SelectAll() = 0;

  ///
  /// Prints this WebView to a PDF file asynchronously.
  ///
  /// @param  config  The configuration settings to use (you must specify
  ///                 a writable output_path or this operation will fail).
  ///
  /// @see WebView::set_print_listener
  ///
  /// @return  Returns a unique request ID that you can use later to identify
  ///          this specific request (see WebViewListener::Print). May return 0
  ///          if this method fails prematurely (eg, if the view has crashed).
  ///
  virtual int PrintToFile(const PrintConfig& config) = 0;

  ///
  /// Check if an error occurred during the last synchronous API call.
  ///
  /// @see WebView::CreateGlobalJavascriptObject
  /// @see WebView::ExecuteJavascriptWithResult
  ///
  virtual Error last_error() const = 0;

  ///
  /// Create a JavaScript Object that will persist between all loaded pages.
  ///
  /// @note
  ///
  /// Global Objects can only contain the following JavaScript types as 
  /// properties:
  ///
  /// - Number
  /// - String
  /// - Array
  /// - Null
  /// - Undefined
  ///
  /// Global Objects will retain any custom methods that are registered.
  ///
  /// You can only create objects on pages with an active DOM. (You should
  /// wait until the first DOMReady event before creating your objects).
  ///
  /// @param  name  The name of the object as it will appear in JavaScript.
  ///
  /// @return The returned JSValue will be of 'Object' type if this call
  /// succeeds. You can check the reason why the call failed by calling
  /// WebView::last_error() after this method.
  ///
  virtual JSValue CreateGlobalJavascriptObject(const WebString& name) = 0;

  ///
  /// Executes some JavaScript asynchronously on the page.
  ///
  /// @param  script       The string of JavaScript to execute.
  ///
  /// @param  frame_xpath  The xpath of the frame to execute within; leave
  ///                      this blank to execute in the main frame.
  ///
  virtual void ExecuteJavascript(const WebString& script,
                                 const WebString& frame_xpath) = 0;

  ///
  /// Executes some JavaScript synchronously on the page and returns a result.
  ///
  /// @param  script       The string of JavaScript to execute.
  ///
  /// @param  frame_xpath  The xpath of the frame to execute within; leave
  ///                      this blank to execute in the main frame.
  ///
  /// @return  Returns the result (if any). Any JSObject returned from this
  ///          method will be a remote proxy for an object contained within
  ///          the WebView process. If this call fails, JSValue will have an
  ///          Undefined type. You can check WebView::last_error() for more
  ///          information about the failure.
  ///
  /// @note  You should never call this from within any of the following
  ///        callbacks:
  ///
  ///        - JSMethodHandler::OnMethodCall
  ///        - JSMethodHandler::OnMethodCallWithReturnValue
  ///        - DataSource::OnRequest
  ///
  virtual JSValue ExecuteJavascriptWithResult(const WebString& script,
                                              const WebString& frame_xpath) = 0;

  ///
  /// Register a handler for custom JSObject methods.
  ///
  /// @param  handler  The handler to register (you retain ownership).
  ///
  /// @see JSObject::SetCustomMethod
  ///
  virtual void set_js_method_handler(JSMethodHandler* handler) = 0;

  /// Get the handler for custom JSObject methods.
  virtual JSMethodHandler* js_method_handler() = 0;

  ///
  /// This method should be called as the result of a user selecting an item
  /// in a popup (dropdown) menu. 
  ///
  /// @param  item_index  The index of the item selected. Item index starts
  ///                     at 0. You can pass -1 as a shortcut for
  ///                     WebView::DidCancelPopupMenu.
  ///
  /// @see  WebViewListener::Menu::OnShowPopupMenu
  ///
  virtual void DidSelectPopupMenuItem(int item_index) = 0;

  ///
  /// This method should be called as the result of a user cancelling a popup
  /// menu.
  ///
  /// @see  WebViewListener::Menu::OnShowPopupMenu
  ///
  virtual void DidCancelPopupMenu() = 0;

  ///
  /// This method should be called as the result of a user selecting files in
  /// a file-chooser dialog.
  ///
  /// @param  files  An array of file-paths that the user selected. If the
  ///                user cancelled the dialog, you should pass an empty array.
  ///
  /// @param  should_write_files  Whether or not this was a Save File dialog.
  ///
  /// @see  WebViewListener::Dialog::OnShowFileChooser
  ///
  virtual void DidChooseFiles(const WebStringArray& files,
                              bool should_write_files) = 0;

  ///
  /// This method should be called as the result of a user supplying
  /// their credentials in a login dialog.
  ///
  /// @param  request_id  The id of the request that was handled (see 
  ///                     WebLoginDialogInfo::request_id).
  ///
  /// @param  username  The username supplied.
  ///
  /// @param  password  The password supplied.
  ///
  /// @see  WebViewListener::Dialog::OnShowLoginDialog
  ///
  virtual void DidLogin(int request_id,
                        const WebString& username,
                        const WebString& password) = 0;

  ///
  /// This method should be called as the result of a user cancelling
  /// a login dialog.
  ///
  /// @param  request_id  The id of the request that was handled (see 
  ///                     WebLoginDialogInfo::request_id).
  ///
  /// @see  WebViewListener::Dialog::OnShowLoginDialog
  ///
  virtual void DidCancelLogin(int request_id) = 0;

 protected:
    virtual ~WebView() {}
};

}  // namespace Awesomium

#endif  // AWESOMIUM_WEB_VIEW_H_
