#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/sched.h>
#include<linux/moduleparam.h>

static pid_t pid = 1;
module_param(pid,int,0644);

static const char *task_state_to_str(struct task_struct *task) {
    unsigned long state = task->__state;

    if (task->exit_state & EXIT_ZOMBIE)
        return "ZOMBIE";
    if (task->exit_state & EXIT_DEAD)
        return "DEAD";
    if (state & __TASK_STOPPED)
        return "STOPPED";
    if (state & __TASK_TRACED)
        return "TRACED";
    if (state & TASK_INTERRUPTIBLE)
        return "INTERRUPTIBLE";
    if (state & TASK_UNINTERRUPTIBLE)
        return "UNINTERRUPTIBLE";
    if (state == TASK_RUNNING)
        return "RUNNING";

    return "UNKNOWN";
}

static int listinfo_init(void){
    struct pid *pid_struct;
    struct task_struct *target_task;
    struct task_struct *parent_task;
    struct task_struct *sibling_task;
    struct task_struct *child_task;
    
    pid_struct = find_get_pid(pid);
    if (!pid_struct){
        printk(KERN_ALERT "pid not found\n");
        return -1;
    }

    target_task = pid_task(pid_struct,PIDTYPE_PID);
    if (!target_task){
        printk(KERN_ALERT "task not found\n");
        return -1;
    }
    printk(KERN_INFO "\n====Target Process====\n");
    printk(KERN_INFO "comm\t\t\tpid\tstate\n");
    printk(KERN_INFO "%-16s\t%d\t%-12s\n",target_task->comm,target_task->pid,task_state_to_str(target_task));


    printk(KERN_INFO "\n====Parent Process====\n");
    parent_task = target_task->parent;
    if (parent_task){
        printk(KERN_INFO "%-16s\t%d\t%-12s\n",parent_task->comm,parent_task->pid,task_state_to_str(parent_task));
    }
    else{
        printk(KERN_INFO "no parent\n");
    }

    printk(KERN_INFO "\n====Sibling Process====\n");
    if (parent_task){
        sibling_task = NULL;
        list_for_each_entry(sibling_task,&parent_task->children,sibling){
            if (sibling_task->pid != target_task->pid){
                printk(KERN_INFO "%-16s\t%d\t%-12s\n",sibling_task->comm,sibling_task->pid,task_state_to_str(parent_task));
            }
        }
    }
    else{
        printk(KERN_INFO "no sibling\n");
    }

    printk(KERN_INFO "\n====Children Process====\n");
    if(list_empty(&target_task->children)){
        printk(KERN_INFO "no children\n");
    }
    else{
        list_for_each_entry(child_task,&target_task->children,sibling){
            printk(KERN_INFO "%-16s\t%d\t%-12s\n",child_task->comm,child_task->pid,task_state_to_str(child_task));
        }
    }
    return 0;
    
}
static void listinfo_exit(void)
{
printk(KERN_ALERT"bye\n");
}
module_init(listinfo_init);
module_exit(listinfo_exit);
MODULE_LICENSE("GPL");