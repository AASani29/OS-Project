#ifndef _KERN_VMM_MPTINTRO_H_
#define _KERN_VMM_MPTINTRO_H_

#ifdef _KERN_

void set_cr3(unsigned int **pdir);  // sets the CR3 register
unsigned int container_alloc(unsigned int id);

#endif  /* _KERN_ */

#endif  /* !_KERN_VMM_MPTINTRO_H_ */
