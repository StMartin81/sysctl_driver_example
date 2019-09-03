#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/slab.h>

#define MAX_LENGTH 1024
#define START_MEASUREMENT "start_measurement"

struct sysctl_data {
    struct ctl_table ctl_table[2];
    struct ctl_path ctl_path[3];
    struct ctl_table_header *ctl_table_header;
};

static struct sysctl_data *sysctl_data;

int start_measurement (struct ctl_table *ctl, int write,
                       void __user *buffer, size_t *lenp, loff_t *ppos)
{
    if (!*lenp || (*ppos && !write)) {
        *lenp = 0;
        return 0;
    }
    return proc_dostring(ctl, write, buffer, lenp, ppos);
}

int init_sysctl(struct sysctl_data **sysctl_data, const char device_name[])
{
    struct ctl_path *ctl_path;
    struct ctl_table *ctl_table;
    int retval;

    retval = 0;

    if (!(*sysctl_data = kzalloc(sizeof(struct sysctl_data), GFP_KERNEL)))
    {
        pr_err("Can not allocate memory for sysctl_data structure\n");
        retval = -ENOMEM;
        goto sysctl_data_alloc;
    }

    ctl_path = (*sysctl_data)->ctl_path;
    ctl_path[0].procname = "dev";
    ctl_path[1].procname = "xdma";

    ctl_table = (*sysctl_data)->ctl_table;
    ctl_table[0].procname = "start_measurement";
    ctl_table[0].mode = 0666;
    if (!(ctl_table[0].data = kmalloc(MAX_LENGTH, GFP_KERNEL)))
    {
        pr_err("Can not allocate memory for data\n");
        retval = -ENOMEM;
        goto data_alloc;
    }
    ctl_table[0].maxlen = MAX_LENGTH;
    ctl_table[0].proc_handler = start_measurement;

    (*sysctl_data)->ctl_table_header = register_sysctl_paths(ctl_path, ctl_table);
    return 0;

data_alloc:
    kfree(*sysctl_data);
sysctl_data_alloc:
    return retval;
}

void free_sysctl(struct sysctl_data *sysctl_data)
{
    if (sysctl_data)
    {
        unregister_sysctl_table(sysctl_data->ctl_table_header);
        kfree(sysctl_data->ctl_table[0].data);
    }
    kfree(sysctl_data);
}

static int __init xdma_init(void)
{
    return init_sysctl(&sysctl_data, "");
}

void __exit xdma_exit(void)
{
    free_sysctl(sysctl_data);
}

module_init(xdma_init);
module_exit(xdma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Stolpe <martin.stolpe@iaf.fraunhofer.de>");
MODULE_DESCRIPTION("A simple sysfs example driver");
MODULE_VERSION("0.1");
