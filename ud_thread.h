/* 
 * thread library function prototypes
 */
void sig_handler(int sig_no);
void t_create(void(*function)(int), int thread_id, int priority);
void t_yield(void);
void t_init(void);
void t_shutdown(void);
void t_terminate(void);
