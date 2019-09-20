#include <linux/completion.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sysctl.h>

#define DATA_MAX_SIZE 1024
#define START_MEASUREMENT "start_measurement"
#define MEASUREMENT_FINISHED "measurement_finished"
#define START_MEASUREMENT_IDX 0
#define MEASUREMENT_FINISHED_IDX 1
static const char g_device_name[] = "sysctl_dev";

struct sysctl_data
{
  struct ctl_table ctl_table[3];
  struct ctl_path ctl_path[3];
  struct completion measurement_data_ready;
  struct ctl_table_header* ctl_table_header;
  char data[DATA_MAX_SIZE];
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
  struct ctl_table tmp_table = {
    .data = data->data,
    .maxlen = DATA_MAX_SIZE,
  };

  if (!*lenp || (*ppos && !write)) {
    *lenp = 0;
    return 0;
  }

  ret = wait_for_completion_interruptible_timeout(&data->measurement_data_ready,
                                                  msecs_to_jiffies(60000));
  if (ret < 0)
    return ret;
  else if (ret == 0)
    return -ETIME;

  ret = proc_dostring(&tmp_table, write, buffer, lenp, ppos);

  return ret;
}

int
measurement_finished(struct ctl_table* ctl,
                     int write,
                     void __user* buffer,
                     size_t* lenp,
                     loff_t* ppos)
{
  int ret = 0;
  struct sysctl_data* data = (struct sysctl_data*)ctl->data;
  struct ctl_table tmp_table = {
    .data = data->data,
    .maxlen = DATA_MAX_SIZE,
  };

  if (!*lenp || (*ppos && !write)) {
    *lenp = 0;
    return 0;
  }

  ret = proc_dostring(&tmp_table, write, buffer, lenp, ppos);
  complete(&(data->measurement_data_ready));

  return ret;
}

void
free_sysctl(struct sysctl_data* sysctl_data)
{
  if (sysctl_data) {
    unregister_sysctl_table(sysctl_data->ctl_table_header);
    kfree(sysctl_data->data);
  }
  kfree(sysctl_data);
  sysctl_data = NULL;
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
  ctl_table[START_MEASUREMENT_IDX].mode = 0444;
  ctl_table[START_MEASUREMENT_IDX].data = (*sysctl_data);
  ctl_table[START_MEASUREMENT_IDX].maxlen = DATA_MAX_SIZE;
  ctl_table[START_MEASUREMENT_IDX].proc_handler = start_measurement;

  ctl_table[MEASUREMENT_FINISHED_IDX].procname = MEASUREMENT_FINISHED;
  ctl_table[MEASUREMENT_FINISHED_IDX].mode = 0222;
  ctl_table[MEASUREMENT_FINISHED_IDX].data = (*sysctl_data);
  ctl_table[MEASUREMENT_FINISHED_IDX].maxlen = DATA_MAX_SIZE;
  ctl_table[MEASUREMENT_FINISHED_IDX].proc_handler = measurement_finished;

  init_completion(&((*sysctl_data)->measurement_data_ready));

  (*sysctl_data)->ctl_table_header = register_sysctl_paths(ctl_path, ctl_table);
  return 0;

sysctl_data_alloc:
  free_sysctl(*sysctl_data);
  return retval;
}

static int __init
sysctl_test_init(void)
{
  return init_sysctl(&sysctl_data, g_device_name);
}

void __exit
sysctl_test_exit(void)
{
  free_sysctl(sysctl_data);
}

module_init(sysctl_test_init);
module_exit(sysctl_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Stolpe <martin.stolpe@iaf.fraunhofer.de>");
MODULE_DESCRIPTION("A simple sysfs example driver");
MODULE_VERSION("0.1");
