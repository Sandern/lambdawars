///
/// @file ResourceInterceptor.h
///
/// @brief The header for the ResourceInterceptor events.
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
#ifndef AWESOMIUM_RESOURCE_INTERCEPTOR_H_
#define AWESOMIUM_RESOURCE_INTERCEPTOR_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

class WebURL;
class WebString;
class WebView;

class ResourceResponse;
class ResourceRequest;
class UploadElement;

///
/// @brief The ResourceInterceptor class is used to intercept requests
/// and responses for resources via WebView::set_resource_interceptor
///
class OSM_EXPORT ResourceInterceptor {
 public:
  ///
  /// Override this method to intercept requests for resources. You can use
  /// this to modify requests before they are sent, respond to requests using
  /// your own custom resource-loading back-end, or to monitor requests for
  /// tracking purposes.
  ///
  /// @param  request  The resource request.
  ///
  /// @return  Return a new ResourceResponse (see ResourceResponse::Create)
  ///          to override the response, otherwise, return NULL to allow
  ///          normal behavior.
  ///
  /// @note WARNING: This method is called on the IO Thread, you should not
  ///       make any calls to WebView or WebCore (they are not threadsafe).
  ///
  virtual Awesomium::ResourceResponse* OnRequest(
                                          Awesomium::ResourceRequest* request) {
    return 0;
  }

  virtual ~ResourceInterceptor() {}
};

///
/// The ResourceRequest class represents a request for a URL resource. You can
/// get information about the request or modify it (change GET to POST, modify
/// headers, etc.).
///
class OSM_EXPORT ResourceRequest {
 public:
  /// Cancel this request.
  virtual void Cancel() = 0;

  /// The process ID where this request originated from. This corresponds
  /// to WebView::process_id().
  virtual int origin_process_id() = 0;

  /// Get the URL associated with this request.
  virtual WebURL url() = 0;

  /// Get the HTTP method (usually "GET" or "POST")
  virtual WebString method() = 0;

  /// Set the HTTP method
  virtual void set_method(const WebString& method) = 0;

  /// Get the referrer
  virtual WebString referrer() = 0;

  /// Set the referrer
  virtual void set_referrer(const WebString& referrer) = 0;

  /// Get extra headers for the request
  virtual WebString extra_headers() = 0;

  virtual void set_extra_headers(const WebString& headers) = 0;

  virtual void AppendExtraHeader(const WebString& name,
                                 const WebString& value) = 0;

  /// Get the number of upload elements (essentially, batches of POST data).
  virtual unsigned int num_upload_elements() = 0;

  /// Get a certain upload element (returned instance is owned by this class)
  virtual const UploadElement* GetUploadElement(unsigned int idx) = 0;

  /// Clear all upload elements
  virtual void ClearUploadElements() = 0;

  /// Append a file for POST data (adds a new UploadElement)
  virtual void AppendUploadFilePath(const WebString& path) = 0;

  /// Append a string of bytes for POST data (adds a new UploadElement)
  virtual void AppendUploadBytes(const char* bytes,
                                 unsigned int num_bytes) = 0;

 protected:
  virtual ~ResourceRequest() {}
};

///
/// @brief The ResourceResponse class is simply a wrapper around a raw block
///        of data and a specified mime-type. It can be used with
///        ResourceInterceptor::onRequest to return a custom resource for a
///        certain resource request.
///
class OSM_EXPORT ResourceResponse {
 public:
  ///
  /// Create a ResourceResponse from a raw block of data. (Data is not owned,
  /// a copy is made of the supplied buffer.)
  ///
  /// @param  num_bytes  Size (in bytes) of the memory buffer.
  ///
  /// @param  buffer     Raw memory buffer to be copied.
  ///
  /// @param  mime_type  The mime-type of the data.
  ///                    See <http://en.wikipedia.org/wiki/Internet_media_type>
  ///
  static ResourceResponse* Create(unsigned int num_bytes,
                                  unsigned char* buffer,
                                  const WebString& mime_type);

  ///
  /// Create a ResourceResponse from a file on disk.
  ///
  /// @param  file_path  The path to the file.
  ///
  static ResourceResponse* Create(const WebString& file_path);

 protected:
  ResourceResponse(unsigned int num_bytes,
                   unsigned char* buffer,
                   const WebString& mime_type);
  ResourceResponse(const WebString& file_path);

  ~ResourceResponse();

  unsigned int num_bytes_;
  unsigned char* buffer_;
  WebString mime_type_;
  WebString file_path_;

  friend class WebCoreImpl;
};

class OSM_EXPORT UploadElement {
 public:
  /// Whether or not this UploadElement is a file
  virtual bool IsFilePath() const = 0;

  /// Whether or not this UploadElement is a string of bytes
  virtual bool IsBytes() const = 0;

  virtual unsigned int num_bytes() const = 0;

  /// Get the string of bytes associated with this UploadElement
  virtual const unsigned char* bytes() const = 0;

  /// Get the file path associated with this UploadElement
  virtual WebString file_path() const = 0;

 protected:
  virtual ~UploadElement() {}
};

}  // namespace Awesomium

#endif  // AWESOMIUM_RESOURCE_INTERCEPTOR_H_
