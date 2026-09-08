#ifndef TASK_CLI_H
#define TASK_CLI_H

void InitCLITask(void);
void StartCLITask(void *argument);


/* Domain command handlers (분리: cli-bsa.c / cli-plc.c) */
void CLI_BSA_Cmd(int argc, char **argv);
void CLI_PLC_Cmd(int argc, char **argv);

#endif /* TASK_CLI_H */
