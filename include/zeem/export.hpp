// Copyright Maarten L. Hekkelman 2026 
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#ifndef ZEEM_EXPORT
# define ZEEM_EXPORT
#endif

#ifndef ZEEM_INLINE
# define ZEEM_INLINE inline
#endif

#ifndef ZEEM_API
# if defined(_WIN32) && defined(ZEEM_SHARED_BUILD)
#  define ZEEM_API __declspec(dllexport)
# else
#  define ZEEM_API
# endif
#endif
