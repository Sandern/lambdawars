// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef REFCOUNT_DWA2002615_HPP
# define REFCOUNT_DWA2002615_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/cast.hpp>

namespace boost { namespace python { 

template <class T>
inline T* incref(T* p)
{
    Py_INCREF(python::upcast<PyObject>(p));
    return p;
}

template <class T>
inline T* xincref(T* p)
{
    Py_XINCREF(python::upcast<PyObject>(p));
    return p;
}

template <class T>
inline void decref(T* p)
{
    Py_DECREF(python::upcast<PyObject>(p));
}

template <class T>
inline void xdecref(T* p)
{
#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1700)
	// Workaround VS2012 & VS2013 internal compiler error...
	PyObject *_py_tmp = (PyObject *)(python::upcast<PyObject>(p));
	if (_py_tmp != NULL) 
	{    
		_Py_DEC_REFTOTAL;
		--(_py_tmp->ob_refcnt);
	}				

	if (_py_tmp != NULL && _py_tmp->ob_refcnt != 0)
	{
		_Py_CHECK_REFCNT(_py_tmp);
	}
	else if (_py_tmp != NULL && _py_tmp->ob_refcnt == 0)
	{
		_Py_INC_TPFREES(_py_tmp) _Py_COUNT_ALLOCS_COMMA
		(*_py_tmp->ob_type->tp_dealloc)((PyObject *)(_py_tmp));
	}
#else
	Py_XDECREF(python::upcast<PyObject>(p));
#endif
}

}} // namespace boost::python

#endif // REFCOUNT_DWA2002615_HPP
