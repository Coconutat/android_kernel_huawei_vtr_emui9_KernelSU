#ifndef __PAGE_EXT_H
#define __PAGE_EXT_H

#ifdef CONFIG_HISI_PAGE_EXT
extern unsigned long  _free_unused_page_ext(void);
#else
unsigned long  _free_unused_page_ext(void) {return 0;};
#endif

#endif
