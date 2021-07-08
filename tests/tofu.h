
typedef struct
{
	int shoreditch;
} tofu_t;

extern tofu_t *tofu_new();
extern void tofu_delete(tofu_t *);
extern int tofu_munch(tofu_t *);

