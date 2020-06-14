# imu_pool
`imu_pool`是一个imu数据的`无锁`环形缓冲池，支持单写多读。生产者以固定的速率(例如，100Hz)填充数据，消费者读取指定时间戳范围的数据。环形缓冲中的数据是均匀分布的，每一帧imu数据都带有一个timestamp。

关于无锁，这里假定了生产者以固定的速率产生数据，消费端通过抛弃最旧的几帧数据，来避免加锁。为了防止写端数据短暂积压，速率不恒定，增加了读保护，正在读取数据的时候，写端将会进入Retry-Loop循环等待。严格来说，要实现真正安全的“无锁”，有如下两种方案：
-  CAS(compare_and_swap)原子操作 + Retry-Loop
-  使用Memory barrier（内存屏障）技术

参考资料：

[无锁队列的实现-coolshell](https://coolshell.cn/articles/8239.html)

[无锁队列的分析与设计](https://www.ibm.com/developerworks/cn/aix/library/au-multithreaded_structures2/index.html)

[理解 Memory barrier (内存屏障)](https://zhuanlan.zhihu.com/p/102307258)

[Linux内核kfifo的实现](https://www.linuxidc.com/Linux/2016-12/137936.htm)