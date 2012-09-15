# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
code repository for Indexing Suite V2 - std containers wrappers
"""

all = []

import slice_header
all.append( slice_header )

import set_header
all.append( set_header )

import element_proxy_traits_header
all.append( element_proxy_traits_header )

import python_iterator_header
all.append( python_iterator_header )

import proxy_iterator_header
all.append( proxy_iterator_header )

import element_proxy_header
all.append( element_proxy_header )

import container_suite_header
all.append( container_suite_header )

import slice_handler_header
all.append( slice_handler_header )

import workaround_header
all.append( workaround_header )

import value_traits_header
all.append( value_traits_header )

import visitor_header
all.append( visitor_header )

import algorithms_header
all.append( algorithms_header )

import vector_header
all.append( vector_header )

import methods_header
all.append( methods_header )

import deque_header
all.append( deque_header )

import shared_proxy_impl_header
all.append( shared_proxy_impl_header )

import iterator_range_header
all.append( iterator_range_header )

import int_slice_helper_header
all.append( int_slice_helper_header )

import container_traits_header
all.append( container_traits_header )

import suite_utils_header
all.append( suite_utils_header )

import list_header
all.append( list_header )

import map_header
all.append( map_header )

import iterator_traits_header
all.append( iterator_traits_header )

import container_proxy_header
all.append( container_proxy_header )

import multimap_header
all.append( multimap_header )

import pair_header
all.append( pair_header )

import registry_utils_header
all.append( registry_utils_header )

headers = map( lambda f: f.file_name, all )
