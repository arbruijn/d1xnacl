#ifdef __cplusplus
extern "C" {
#endif
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler) { return 0; }
int getuid(void) { return 0; }
int geteuid(void) { return 0; }
int setuid() { return 0; }
int getgid(void) { return 0; } 
struct passwd;
struct passwd *getpwnam(const char *name) { return 0; }
int link(const char *oldpath, const char *newpath) { return -1; }
int unlink(const char *path) { return -1; }
#ifdef __cplusplus
}
#endif

