/*
 *  swig/charmap.i: ObexFTP client library SWIG interface
 *
 *  Copyright (c) 2006 Christian W. Zuckschwerdt <zany@triq.net>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *     
 */

/* handle strings that maybe NULL */
#if defined SWIGPERL
/* perl5.swg uses PL_sv_undef. */
#elif defined SWIGPYTHON
/* python.swg uses "". Change to Py_None maybe? */
#elif defined SWIGRUBY
%typemap(in) char *	"$1 = ($input != Qnil) ? StringValuePtr($input) : NULL;";
%typemap(freearg) char * ""; /* Fix for >=swig-1.3.28 */
%typemap(out) char *	"$result = $1 ? rb_str_new2($1) : Qnil;";
#elif defined SWIGTCL
/* tcl8.swg is naive about this. Does it work? */
#else
#warning "no char * in-typemap for this language"
#endif

/* handle non-sz strings */
%typemap(out) char * {
#if defined SWIGPERL
	$result = newSVpvn(arg1->buf_data, arg1->buf_size);
	argvi++;
#elif defined SWIGPYTHON
	$result = PyString_FromStringAndSize(arg1->buf_data, arg1->buf_size);
#elif defined SWIGRUBY
	$result = arg1->buf_data ? rb_str_new(arg1->buf_data, arg1->buf_size) : Qnil;
#elif defined SWIGTCL
	Tcl_SetObjResult(interp,Tcl_NewStringObj((char *)arg1->buf_data,arg1->buf_size));
#else
#warning "no char * out-typemap for this language"
#endif
};

/* handle arrays of strings */
%typemap(out) char ** {
#if defined SWIGPERL
  char **p;
  AV *myav = newAV();
  for (p = $1; p && *p; p++)
    av_push(myav, newSVpv(*p, 0));
  $result = newRV_noinc((SV*)myav);
  sv_2mortal($result);
  argvi++;
#elif defined SWIGPYTHON
  char **p;
  $result = PyList_New(0);
  for (p = $1; p && *p; p++)
    PyList_Append($result, PyString_FromString(*p));
#elif defined SWIGRUBY
  char **p;
  $result = rb_ary_new();
  for (p = $1; p && *p; p++)
    rb_ary_push($result, rb_str_new2(*p));
#elif defined SWIGTCL
  char **p;
/* $result = Tcl_NewListObj(0, NULL); // not needed? */
  for (p = $1; p && *p; p++)
    Tcl_ListObjAppendElement(interp, $result, Tcl_NewStringObj(*p, strlen(*p)));
#else
#warning "no char ** out-typemap for this language"
#endif
}

%typemap(in) (char *data, size_t size) {
	/* Danger Wil Robinson */
#if defined SWIGPERL
	$1 = SvPV($input,$2);
#elif defined SWIGPYTHON
	$1 = PyString_AsString($input);
	$2 = PyString_Size($input);
#elif defined SWIGRUBY
/* VALUE str = StringValue($input); // perhaps better? */
	$1 = STR2CSTR($input);
	$2 = (int) RSTRING($input)->len;
#elif defined SWIGTCL
	$1 = Tcl_GetStringFromObj($input,&$2);
#else
#warning "no char *, size_t in-typemap for this language"
#endif
};
