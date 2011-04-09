#ifndef __U4C_H__
#define __U4C_H__ 1

typedef struct u4c_globalstate u4c_globalstate_t;

extern u4c_globalstate_t *u4c_init(void);
extern void u4c_list_tests(u4c_globalstate_t *);
extern int u4c_run_tests(u4c_globalstate_t *);
extern void u4c_done(u4c_globalstate_t *);

#endif /* __U4C_H__ */
