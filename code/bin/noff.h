/* noff.h 
 *     Data structures defining the Nachos Object Code Format
 *
 *     Basically, we only know about three types of segments:
 *	code (read-only), initialized data, and unitialized data
 */

#define NOFFMAGIC	0xbadfad 	/* magic number 标识该文件为 Nachos 对象文件*/

typedef struct segment {
  int virtualAddr;		/* 该段在虚拟地址空间中的起始地址 */
  int inFileAddr;		/* 该段在文件中的起始偏移 */
  int size;			/* size of segment */
} Segment;

typedef struct noffHeader {
   int noffMagic;		/* should be NOFFMAGIC */
   Segment code;		/* 可执行代码段 */ 
   Segment initData;		/* 已初始化数据段 */
   Segment uninitData;		/* 未初始化数据段（运行前需清零）*/
} NoffHeader;
