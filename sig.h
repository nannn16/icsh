#define MAXJOBS      100

typedef struct job {
    pid_t pid;
    int job_id;
    char *command;
    char *state;
} job;

struct job jobs[MAXJOBS];
pid_t fgpid;
int fgjob_id;
pid_t curbgpid;
pid_t prevbgpid;
int fgrun;
int exitCode;

void enableDefaultHandlers();
void enableSignalHandlers();
void printCmd();
void printJobDone(pid_t pid);
void addJob(pid_t pid, int job_id, char *cmd, char *state);
void deleteJob(pid_t pid, int job_id);
int findFreeJobID();