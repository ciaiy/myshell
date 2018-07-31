**myshell v2.0**

作者: ciaiy

时间: 2018-07-31

---

``程序目标:``
 - 单个命令:   ls    
 - 带-l到多个参数的命令:   ls -l /temp
 - 带输出重定向的命令: ls -l > a
 - 带输入重定向的命令: wc -c < a
 - 带管道命令: ls -l | wc -c
 - 后台运行符可以添加到命令的最后面:   ls &

---

**``注意``**: 

 - 需要用`root`权限编译
 - ``本程序依赖GNU的readline``如果没有安装的话, 请先安装, 安装命令: sudo apt-get install readline
 - 本程序会在`实际用户的家目录`下新建一个.history文件

---

更新日志:

- sudo 和 su 命令需要验证
