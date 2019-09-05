#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/slab.h>
#include <linux/completion.h>

#define MAX_LENGTH 1024
#define START_MEASUREMENT "start_measurement"
#define MEASUREMENT_FINISHED "measurement_finished"
#define START_MEASUREMENT_IDX 0
#define MEASUREMENT_FINISHED_IDX 1
static const char DEVICE_NAME[] = "xdma";

struct sysctl_data {
    struct ctl_table ctl_table[3];
    struct ctl_path ctl_path[3];
    struct completion measurement_data_ready;
    struct ctl_table_header *ctl_table_header;
};

static struct sysctl_data *sysctl_data;

int start_measurement (struct ctl_table *ctl, int write,
                       void __user *buffer, size_t *lenp, loff_t *ppos)
{
    struct sysctl_data *data = container_of((void *)(ctl - START_MEASUREMENT_IDX), struct sysctl_data, ctl_table);
    pr_info("Data address (start_measurement): 0x%p\n", data);
    if (!*lenp || (*ppos && !write)) {
        *lenp = 0;
        return 0;
    }
    wait_for_completion(&(data->measurement_data_ready));
    return proc_dostring(ctl, write, buffer, lenp, ppos);
}

int measurement_finished (struct ctl_table *ctl, int write,
                          void __user *buffer, size_t *lenp, loff_t *ppos)
{
    struct sysctl_data *data = container_of((void *)(ctl - MEASUREMENT_FINISHED_IDX), struct sysctl_data, ctl_table);
    pr_info("Data address (measurement_finished): 0x%p\n", data);
    if (!*lenp || (*ppos && !write)) {
        *lenp = 0;
        return 0;
    }
    complete(&(data->measurement_data_ready));
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
    pr_info("Data address (init_sysctl): 0x%p\n", *sysctl_data);

    ctl_path = (*sysctl_data)->ctl_path;
    ctl_path[0].procname = "dev";
    ctl_path[1].procname = device_name;

    ctl_table = (*sysctl_data)->ctl_table;
    ctl_table[START_MEASUREMENT_IDX].procname = START_MEASUREMENT;
    ctl_table[START_MEASUREMENT_IDX].mode = 0666;
    if (!(ctl_table[0].data = kmalloc(MAX_LENGTH, GFP_KERNEL)))
    {
        pr_err("Can not allocate memory for data\n");
        retval = -ENOMEM;
        goto data_alloc_start_measurement;
    }
    ctl_table[START_MEASUREMENT_IDX].maxlen = MAX_LENGTH;
    ctl_table[START_MEASUREMENT_IDX].proc_handler = start_measurement;

    ctl_table[MEASUREMENT_FINISHED_IDX].procname = MEASUREMENT_FINISHED;
    ctl_table[MEASUREMENT_FINISHED_IDX].mode = 0666;
    if (!(ctl_table[MEASUREMENT_FINISHED_IDX].data = kmalloc(MAX_LENGTH, GFP_KERNEL)))
    {
        pr_err("Can not allocate memory for data\n");
        retval = -ENOMEM;
        goto data_alloc_measurment_finished;
    }
    ctl_table[MEASUREMENT_FINISHED_IDX].maxlen = MAX_LENGTH;
    ctl_table[MEASUREMENT_FINISHED_IDX].proc_handler = measurement_finished;

    init_completion(&((*sysctl_data)->measurement_data_ready));

    (*sysctl_data)->ctl_table_header = register_sysctl_paths(ctl_path, ctl_table);
    return 0;

data_alloc_measurment_finished:
    kfree(ctl_table[START_MEASUREMENT_IDX].data);
data_alloc_start_measurement:
    kfree(*sysctl_data);
sysctl_data_alloc:
    return retval;
}

void free_sysctl(struct sysctl_data *sysctl_data)
{
    if (sysctl_data)
    {
        unregister_sysctl_table(sysctl_data->ctl_table_header);
        kfree(sysctl_data->ctl_table[MEASUREMENT_FINISHED_IDX].data);
        kfree(sysctl_data->ctl_table[START_MEASUREMENT_IDX].data);
    }
    kfree(sysctl_data);
}

static int __init xdma_init(void)
{
    return init_sysctl(&sysctl_data, DEVICE_NAME);
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
