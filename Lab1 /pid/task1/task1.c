#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

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

static int hello_init(void)
{
        struct task_struct *p;
        printk(KERN_INFO"\n==== Process ====\n");
        printk(KERN_INFO"comm\t\t\tpid\tstate\t\tprio\tppid\n");
        for_each_process(p){
                if (p->flags & PF_KTHREAD){    //or (task_struct -> mm == NULL){
                        int ppid = p->parent ? p->parent->pid : 0;
                        printk(KERN_INFO "%-16s\t%d\t%-12s\t%d\t%d\n",
                                p->comm, 
                                p->pid, 
                                task_state_to_str(p),
                                p->prio, 
                                p->parent ? p->parent->pid : 0);                
                        }
                else{

                        continue;
                }
        }
        return 0;
}
static void hello_exit(void)
{
        printk(KERN_ALERT "task1 exit\n");
}
module_init(hello_init);//加载函数
module_exit(hello_exit);//卸载函数
MODULE_LICENSE("GPL");  //许可证申明
MODULE_DESCRIPTION("hello module");
