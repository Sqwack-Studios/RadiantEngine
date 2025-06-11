//  Filename: platform 
//	Author:	Daniel														
//	Date: 11/06/2025 20:56:15		
//  Sqwack-Studios													



#ifndef RE_PLATFORM_H
#define	RE_PLATFORM_H


#if defined(__clang__) || defined(__gcc__)
#define RE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define RE_INLINE __forceinline
#else
#define RE_INLINE inline
#endif


#endif // !RE_PLATFORM_H
