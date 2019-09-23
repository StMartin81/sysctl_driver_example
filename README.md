Example for implementing a Linux driver which uses the sysctl interface.

The driver will create two files:
* /proc/sys/dev/sysctl_dev/start_measurement
* /proc/sys/dev/sysctl_dev/measurement_finished

The start_measurement interface waits until data was written to measurement_finished or a timeout occured.

The messaging interface  can be tried out with the following commands:
```
$ cat /proc/sys/dev/sysctl_dev/start_measurement
$ echo test > /proc/sys/dev/sysctl_dev/measurement_finished (has to done in another terminal)
```
