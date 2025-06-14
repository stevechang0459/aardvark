#ifndef TYPES_H
#define TYPES_H

#ifndef __GNUC__
#define __asm__         asm
#define __typeof__      typeof
#define __inline__      inline
#endif

#ifndef _MSC_VER
/* C99-compliant compilers (GCC) */
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint64_t __le64;
typedef uint32_t __le32;
typedef uint16_t __le16;
typedef uint8_t __u8;

typedef uint64_t __u64;
typedef uint32_t __u32;
typedef uint16_t __u16;

#else
/* Microsoft compilers (Visual C++) */
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;
typedef signed __int8 s8;
typedef signed __int16 s16;
typedef signed __int32 s32;
typedef signed __int64 s64;
#endif /* __MSC_VER */

#define u08 u8
#define s08 s8

// typedef unsigned char bool;

typedef unsigned char       byte;
typedef unsigned short      word;
typedef unsigned int        dword;
typedef unsigned long long  qword;

typedef int ret_code;

/**
 *linux/include/linux/types.h
 */
// struct list_head {
//     struct list_head *next, *prev;
// };

// struct hlist_head {
//     struct hlist_node *first;
// };

// struct hlist_node {
//     struct hlist_node *next, **pprev;
// };

/**
 *linux/include/linux/stddef.h
 */
#define NULL    ((void *)0)

// enum {
//      false = 0,
//      true  = 1
// };

#if 0
#define _offsetof(type, member)  ((size_t)&((type *)0)->member)

/**
 * offsetofend(TYPE, MEMBER)
 *
 * @TYPE: The type of the structure
 * @MEMBER: The member within the structure to get the end offset of
 */
#define _offsetend(type, member) \
    (_offsetof(type, member) + sizeof(((type *)0)->member))

/**
 * linux/include/linux/kernel.h
 *
 * _container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define _container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - _offsetof(type, member) );})
#endif
#define U64_MASK                                                (0xFFFFFFFFFFFFFFFF)

#define U32_SHIFT                       (32)
#define U32_MASK                        ((1ULL << U32_SHIFT) - 1)

#define U16_SHIFT                       (16)
#define U16_MASK                        ((1 << U16_SHIFT) - 1)

#define U8_SHIFT                        (8)
#define U8_MASK                         ((1 << U8_SHIFT) - 1)

#define DWORD_SHIFT                     (32)
#define DWORD_MASK                      ((1ULL << DWORD_SHIFT) - 1)

#define WORD_SHIFT                      (16)
#define WORD_MASK                       ((1 << WORD_SHIFT) - 1)

#define QW_ALIGNED(addr)                (((addr) + 7) & (0xfffffff8))
#define DW_ALIGNED(addr)                (((addr) + 3) & (0xfffffffc))
#define WD_ALIGNED(addr)                (((addr) + 1) & (0xfffffffe))
#define FW_GDMA_ALIGNED(addr)           (QW_ALIGNED(addr))

#define FW_MEM_ADDR_ALIGN               (64)
#define FW_MEM_ADDR_ALIGN_MASK          (FW_MEM_ADDR_ALIGN - 1)
#define FW_MEM_ADDR_ALIGNED(addr)       (((addr) + FW_MEM_ADDR_ALIGN_MASK) & (0xffffffc0))

#define ATA_MEM_ADDR_ALIGN              (512)
#define ATA_MEM_ADDR_ALIGN_MASK         (ATA_MEM_ADDR_ALIGN - 1)
#define ATA_MEM_ADDR_ALIGNED(addr)      (((addr) + ATA_MEM_ADDR_ALIGN_MASK) & (0xfffffe00))

#define MEM_ALIGN(addr, align)          ((addr + align - 1) & ~(dword)(align - 1))

#define KB (1024)
#define MB (KB * KB)
#define GB (MB * KB)

#define BITLSHIFT(v, n)                 ((u32)(v) << (n))
#define BITRSHIFT(v, n)                 ((u32)(v) >> (n))

// #define likely(x)                       __builtin_expect((x), 1)
// #define unlikely(x)                     __builtin_expect((x), 0)
#define likely(x)                       __builtin_expect(!!(x), 1)
#define unlikely(x)                     __builtin_expect(!!(x), 0)

enum {
	FALSE = 0,
	TRUE = 1
};

enum trace_type {
	ERROR = 0,
	WARN = 1,
	DEBUG,
	INFO,
	INIT,

	TRACE_TYPE_MAX
};

struct aa_args {
	int handle;
	int verbose;
	uint32_t nsid;
	int timeout;
	uint8_t slv_addr;
	uint8_t dst_eid;
	bool csi;
	bool pec;
	bool ic;
};

#endif  // TYPES_H
