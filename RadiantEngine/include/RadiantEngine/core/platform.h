//  Filename: platform 
//	Author:	Daniel										
//	Date: 11/06/2025 20:56:15		
//  Sqwack-Studios													



#ifndef RE_PLATFORM_H
#define	RE_PLATFORM_H

#define persistent static //use this when declaring a variable with persistent memory locally in a function 
#define internal static //use this when declaring a function to be internally linked to the scope of the translation unit

#if defined(__clang__) || defined(__gcc__)
#define RE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define RE_INLINE __forceinline
#else
#define RE_INLINE inline
#endif



#endif // !RE_PLATFORM_H
