#define MAXJOBS      100

typedef struct job {
    pid_t pid;
    int job_id;
    char *command;
    char *state;
} job;

struct job bg_jobs[MAXJOBS];
struct job foreground;
pid_t fgpid;
pid_t curbgpid;
pid_t prevbgpid;
int fgrun;
int exitCode;

void enableDefaultHandlers();
void enableSignalHandlers();
void printCmd();
void printJobDone(pid_t pid);
void addBgJob(pid_t pid, int job_id, char *cmd, char *state);
void deleteBgJob(pid_t pid, int job_id);
int findFreeJobID();