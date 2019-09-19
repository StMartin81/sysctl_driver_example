#include <linux/completion.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sysctl.h>

#define MAX_LENGTH 1
#define START_MEASUREMENT "start_measurement"
#define MEASUREMENT_FINISHED "measurement_finished"
#define START_MEASUREMENT_IDX 0
#define MEASUREMENT_FINISHED_IDX 1
static const char DEVICE_NAME[] = "xdma";

struct sysctl_data
{
  struct ctl_table ctl_table[3];
  struct ctl_path ctl_path[3];
  struct completion measurement_data_ready;
  struct ctl_table_header* ctl_table_header;
  void* data;
};

static struct sysctl_data* sysctl_data;

int
start_measurement(struct ctl_table* ctl,
                  int write,
                  void __user* buffer,
                  size_t* lenp,
                  loff_t* ppos)
{
  long ret;
  struct sysctl_data* data = (struct sysctl_data*)ctl->data;

  ret = wait_for_completion_interruptible_timeout(&data->measurement_data_ready,
                                                  msecs_to_jiffies(60000));
  if (ret < 0)
    return ret;
  else if (ret == 0)
    return -ETIME;

  return 0;
}

int
measurement_finished(struct ctl_table* ctl,
                     int write,
                     void __user* buffer,
                     size_t* lenp,
                     loff_t* ppos)
{
  struct sysctl_data* data = (struct sysctl_data*)ctl->data;
  complete(&(data->measurement_data_ready));
  return 0;
}

int
init_sysctl(struct sysctl_data** sysctl_data, const char device_name[])
{
  struct ctl_path* ctl_path;
  struct ctl_table* ctl_table;
  int retval;

  retval = 0;

  if (!(*sysctl_data = kzalloc(sizeof(struct sysctl_data), GFP_KERNEL))) {
    pr_err("Can not allocate memory for sysctl_data structure\n");
    retval = -ENOMEM;
    goto sysctl_data_alloc;
  }

  ctl_path = (*sysctl_data)->ctl_path;
  ctl_path[0].procname = "dev";
  ctl_path[1].procname = device_name;

  ctl_table = (*sysctl_data)->ctl_table;
  ctl_table[START_MEASUREMENT_IDX].procname = START_MEASUREMENT;
  ctl_table[START_MEASUREMENT_IDX].mode = 0222;
  ctl_table[START_MEASUREMENT_IDX].data = *sysctl_data;
  ctl_table[START_MEASUREMENT_IDX].proc_handler = start_measurement;

  ctl_table[MEASUREMENT_FINISHED_IDX].procname = MEASUREMENT_FINISHED;
  ctl_table[MEASUREMENT_FINISHED_IDX].mode = 0222;
  ctl_table[MEASUREMENT_FINISHED_IDX].data = *sysctl_data;
  ctl_table[MEASUREMENT_FINISHED_IDX].proc_handler = measurement_finished;

  init_completion(&((*sysctl_data)->measurement_data_ready));

  (*sysctl_data)->ctl_table_header = register_sysctl_paths(ctl_path, ctl_table);
  return 0;

sysctl_data_alloc:
  return retval;
}

void
free_sysctl(struct sysctl_data* sysctl_data)
{
  if (sysctl_data) {
    unregister_sysctl_table(sysctl_data->ctl_table_header);
  }
  kfree(sysctl_data);
}

static int __init
xdma_init(void)
{
  return init_sysctl(&sysctl_data, DEVICE_NAME);
}

void __exit
xdma_exit(void)
{
  free_sysctl(sysctl_data);
}

module_init(xdma_init);
module_exit(xdma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Stolpe <martin.stolpe@iaf.fraunhofer.de>");
MODULE_DESCRIPTION("A simple sysfs example driver");
MODULE_VERSION("0.1");
