///
/// @file PrintConfig.h
///
/// @brief The header for the PrintConfig class.
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
#ifndef AWESOMIUM_PRINT_CONFIG_H_
#define AWESOMIUM_PRINT_CONFIG_H_

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

///
/// @brief Use this class to specify print-to-file settings.
///
/// @see WebView::PrintToFile
///
class OSM_EXPORT PrintConfig {
 public:
  ///
  /// Create the default Printing Configuration
  ///
  PrintConfig();

  /// A writable directory where all files will be written to.
  WebString output_directory;

  /// The dimensions (width/height) of the page, in points.
  Awesomium::Rect page_size;

  /// Number of dots per inch.
  double dpi;

  /// Whether or not we should make a new file for each page.
  bool split_pages_into_multiple_files;

  /// Whether or not we should only print the selection.
  bool print_selection_only;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_PRINT_CONFIG_H_
