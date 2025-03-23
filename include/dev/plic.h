#ifndef __PLIC_H__
#define __PLIC_H__

void plic_init(void);
int plic_claim(void);
void plic_inithart(void);
void plic_complete(int irq);
void intr_on_init();
#endif