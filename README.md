# 标准C语言实现简单Web服务器

完成基础的tcp连接，支持基础的client与其连接 使用fork()来支持并发访问服务器 简单的http访问，支持静态页面访问 支持静态页面访问，需要一定的报错机制，如404页面的建立。
1.	socket实现简单Http服务器，完成html的解析；
2.	运行该服务器可以通过浏览器访问服务器目录下的  Html文件、jpg图片、css文件的载入。 完成初步的Http服务器功能。

# Server端:
1.完成socket(),bind(),listen()这些初始化工作后，调用accept()方法阻塞等待(其实就是进入一个死循环),等待CLient的connect()方法连接 Client端: 
2.先调用socket(),然后调用connect()想要与Server端进行连接，这个时候就会进行传说中的TCP三次握手，也就是在Client 发起connect()，并且Server进入accept()阻塞等待时发生三次握手。

# 效果请访问
https://yc13797.github.io/Hi/
