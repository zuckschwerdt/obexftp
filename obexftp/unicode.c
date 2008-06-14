/**
	\file obexftp/unicode.c
	Unicode charset and encoding conversions.
	ObexFTP library - language bindings for OBEX file transfer.

	Copyright (c) 2007 Christian W. Zuckschwerdt <zany@triq.net>

	ObexFTP is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with ObexFTP. If not, see <http://www.gnu.org/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32 /* no need for iconv */
#include <Windows.h> /* pulls in Winnls.h */
#else
#ifdef HAVE_ICONV
#include <iconv.h>
#include <locale.h>
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#define locale_charset nl_langinfo(CODESET)
#else
#define locale_charset ""
#endif /* HAVE_LANGINFO_H */
#endif /* HAVE_ICONV */
#endif /* _WIN32 */

#include "unicode.h"

#include <common.h>


/**
	Convert a string to UTF-16BE, tries to guess charset and encoding.

	As a lib we can't be sure what the input charset and encoding is.
	Try to read the input as UTF-8, this will also work for plain ASCII (7bit).
	On errors fall back to the environment locale, which again could be UTF-8.
	As last resort try to copy verbatim, i.e. as ISO-8859-1.

	\note This is a quick hack until OpenOBEX is iconv-ready.
 */
int CharToUnicode(uint8_t *uc, const uint8_t *c, int size)
{
#ifdef _WIN32 /* no need for iconv */
	int ret, i;
	char tmp;

        return_val_if_fail(uc != NULL, -1);
        return_val_if_fail(c != NULL, -1);

	/* ANSI to UTF-16LE */
	ret = MultiByteToWideChar(CP_ACP, 0, c, -1, (LPWSTR)uc, size);
	/* turn the eggs the right way around now */
	for (i=0; i < ret; i++) {
		tmp = uc[2*i];
		uc[2*i] = uc[2*i+1];
		uc[2*i+1] = tmp;
	}
	return ret * 2; /* 0 on error */
#else /* _WIN32 */

#ifdef HAVE_ICONV
	iconv_t utf16;
	size_t ni, no, nrc;
	int ret;
	/* avoid type-punned dereferecing (breaks strict aliasing) */
       	char *cc = c;
	char *ucc = uc;

        return_val_if_fail(uc != NULL, -1);
        return_val_if_fail(c != NULL, -1);

	/* try UTF-8 to UTF-16BE */
	ni = strlen(cc) + 1;
	no = size;
	utf16 = iconv_open("UTF-16BE", "UTF-8");
       	nrc = iconv(utf16, &cc, &ni, &ucc, &no);
       	ret = iconv_close(utf16);
       	if (nrc == (size_t)(-1)) {
       		DEBUG(3, "Iconv from UTF-8 conversion error: '%s'\n", cc);
       	} else {
		return size-no;
	}

	/* try current locale charset to UTF-16BE */
	setlocale(LC_CTYPE, "");
	DEBUG(2, "Iconv from locale \"%s\"\n", locale_charset);
       	cc = c;
	ucc = uc;
	ni = strlen(cc) + 1;
	no = size;
	utf16 = iconv_open("UTF-16BE", locale_charset);
       	nrc = iconv(utf16, &cc, &ni, &ucc, &no);
       	ret = iconv_close(utf16);
       	if (nrc == (size_t)(-1)) {
       		DEBUG(3, "Iconv from locale conversion error: '%s'\n", cc);
       	} else {
		return size-no;
	}

	/* fallback to ISO-8859-1 to UTF-16BE (every byte is valid here) */
       	cc = c;
	ucc = uc;
	ni = strlen(cc) + 1;
	no = size;
	utf16 = iconv_open("UTF-16BE", "ISO-8859-1");
       	nrc = iconv(utf16, &cc, &ni, &ucc, &no);
       	ret = iconv_close(utf16);
       	if (nrc == (size_t)(-1)) {
       		DEBUG(2, "Iconv internal conversion error: '%s'\n", cc);
		return -1;
	}

	return size-no;
#else /* HAVE_ICONV */
	return OBEX_CharToUnicode(uc, c, size);
#endif /* HAVE_ICONV */

#endif /* _WIN32 */
}


/**
	Convert a string from UTF-16BE to locale charset.

	Plain ASCII (7bit) and basic ISO-8859-1 will always work.
	This conversion supports UTF-8 and single byte locales.

	\note This is a quick hack until OpenOBEX is iconv-ready.
 */
int UnicodeToChar(uint8_t *c, const uint8_t *uc, int size)
{
#ifdef _WIN32 /* no need for iconv */
	int ret, n, i;
	uint8_t *le;

        return_val_if_fail(uc != NULL, -1);
        return_val_if_fail(c != NULL, -1);

	/* turn the eggs around, pointy side up */
	for (n=0; uc[2*n] != 0 || uc[2*n+1] != 0; n++);
	le = malloc(2*n+2);
	for (i=0; i <= n; i++) {
		le[2*i] = uc[2*i+1];
		le[2*i+1] = uc[2*i];
	}
	/* UTF-16LE to ANSI */
	ret = WideCharToMultiByte(CP_ACP, 0, le, -1, c, size, NULL, NULL);
	free(le);
	return ret; /* 0 on error */
#else /* _WIN32 */

#ifdef HAVE_ICONV
	iconv_t utf16;
	size_t ni, no, nrc;
	int ret;
	/* avoid type-punned dereferecing (breaks strict aliasing) */
       	char *cc = c;
	char *ucc = uc;

        return_val_if_fail(uc != NULL, -1);
        return_val_if_fail(c != NULL, -1);

	/* UTF-16BE to current locale charset */
	setlocale(LC_CTYPE, "");
	DEBUG(3, "Iconv to locale \"%s\"\n", locale_charset);
	for (ni=0; ucc[2*ni] != 0 || ucc[2*ni+1] != 0; ni++);
	ni = 2*ni+2;
	no = size;
	utf16 = iconv_open(locale_charset, "UTF-16BE");
       	nrc = iconv(utf16, &ucc, &ni, &cc, &no);
       	ret = iconv_close(utf16);
       	if (nrc == (size_t)(-1)) {
       		DEBUG(2, "Iconv from locale conversion error: '%s'\n", cc);
	}
	return size-no;
#else /* HAVE_ICONV */
	return OBEX_UnicodeToChar(c, uc, size);
#endif /* HAVE_ICONV */

#endif /* _WIN32 */
}


/**
	Convert a (xml) string from UTF-8 to locale charset.

	Plain ASCII (7bit) and basic ISO-8859-1 will always work.
	This conversion supports UTF-8 and single byte locales.

	\note This is a quick hack until OpenOBEX is iconv-ready.
 */
int Utf8ToChar(uint8_t *c, const uint8_t *uc, int size)
{
#ifdef _WIN32 /* no need for iconv */
	int ret, n, i;
	uint8_t *le;

        return_val_if_fail(uc != NULL, -1);
        return_val_if_fail(c != NULL, -1);

	n = strlen(uc)*2+2;
	le = malloc(n);
	/* UTF-8 to UTF-16LE */
	ret = MultiByteToWideChar(CP_UTF8, 0, uc, -1, (LPWSTR)le, n);

	/* UTF-16LE to ANSI */
	ret = WideCharToMultiByte(CP_ACP, 0, le, -1, c, size, NULL, NULL);
	free(le);
	return ret; /* 0 on error */
#else /* _WIN32 */

#ifdef HAVE_ICONV
	iconv_t utf8;
	size_t ni, no, nrc;
	int ret;
	/* avoid type-punned dereferecing (breaks strict aliasing) */
       	char *cc = c;
	char *ucc = uc;

        return_val_if_fail(uc != NULL, -1);
        return_val_if_fail(c != NULL, -1);

	setlocale(LC_CTYPE, "");
	DEBUG(2, "Iconv to \"%s\"\n", locale_charset);
	ni = strlen(ucc);
	no = size;
	utf8 = iconv_open(locale_charset, "UTF-8");
       	nrc = iconv(utf8, &ucc, &ni, &cc, &no);
       	ret = iconv_close(utf8);
       	if (nrc != (size_t)(-1)) {
       		DEBUG(2, "Iconv from locale conversion error: '%s'\n", cc);
       	}
	return size-no;
#else /* HAVE_ICONV */
	int n, i;
	n = strlen(uc);
	strncpy(c, uc, size);
	c[size] = '\0';
	return n;
#endif /* HAVE_ICONV */

#endif /* _WIN32 */
}
