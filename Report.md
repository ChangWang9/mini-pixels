# 实验报告

## 实验完成情况

完成了对 long， timestamp，decimal，date四个类型的测试，测试结果如下

![1b67a7f25da3b9730f57d4d3ce03a2f](.\figure\P1.png)

![d084d635f45d8836c73b62a6bf58ead](.\figure\P2.png)

![857a74006d3a37ddac661a66fa3a55e](.\figure\P3.png)

![2df9719075ca442737a1ac1e71f8219](.\figure\P4.png)

其中，date遇到了Segmentation fault的问题，迫于时间问题，截至目前还未找到问题来源（可能是vector free的问题），bug主要是当文件行数设置过大时会出现segmentation fault，但当n<=10时可以正常读入。



## 实验完成细节

### Date

​	考虑到Date的底层存储为int，我们可以参考int部分的代码，首先在构造函数中为整型指针分配内存，以存储日期的天数。然后在add(std::string &val)函数中，通过解析字符串形式的“yyyy-mm-dd”，计算与1970-01-01的天数差并存储到dates数组。add(bool)和add(int)则用于根据不同类型将数据写入数组。ensureSize会在需要时成倍扩容并复制已存在的数据。close和析构函数中释放内存。最后在set函数中根据索引更新days值并维护isNull信息 。

### Timestamp

​	考虑到Date的底层存储为long,同理date，我们可以参考long部分的代码，首先在构造函数中根据编码级别决定是否使用运行长度编码。在write函数中，将数据按像素步长写入，若超过步长则调用newPixel切换到下一个像素。writeCurPartTimestamp函数根据isNull判断值是否为0。newPixel函数在开启运行长度编码时使用encoder对数据进行编码，否则直接写入。最后通过close释放资源，getColumnChunkEncoding返回相应的编码类型。

### Decimal

​	如果完整实现需要考虑正负数和四舍五入，需要在vector文件中手动实现，实现思路是：add(std::string &val)函数先解析字符形式的整数部分和小数部分，再根据scale进行放大或截断，并在需要时执行正负方向的四舍五入。确保数组长度不足时会自动扩容并复制已有数据。

​	DecimalColumnWriter 在将 long 值写入输出流时，没有对正负值的特殊区分，直接从 DecimalColumnVector 读取 long 型整数并根据字节序及 nullsPadding 设置将数据写出。

## 实验总结

​	从补全代码角度来看，由于有已经实现好的代码可以参考，所以难度并不大，主要花费时间在理解lab框架，知道每一步在干什么。

​	总的来说还是非常不错的lab，特别是锻炼了debug（尤其是gdb能力），在此特别感谢白锦峄同学和沈海涛同学对我的帮助，提供了很多debug方向的建议。



[仓库连接](https://github.com/ChangWang9/mini-pixels)

