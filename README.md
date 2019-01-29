# 基于libevent的HttpServer

## 简介
Libevent是一个用于处理和调度事件的网络库，以及执行非阻塞I/O，正是因为Libevent是一个基于单线程的库，如果你有多个CPU或具有多线程的CPU，会导致CPU资源闲置。对于这一问题，解决方案是为每个活动连接创建一个Libevent事件队列(AKA event_base），每个队列都有自己的事件未决线程。这个项目正是基于这一点，为您提供编写高性能，多线程，基于Libevent的套接字HTTP服务器所需的一切。
## 功能

1. 提供对简单的负载均衡和反向代理的支持
2. 提供了可供选择的线程池功能以及灵活的Json配置文件
3. 对JavaWeb动态页面的转发
4. 对于服务器内部错误和请求异常提供了5种不同的日志记录等级以及内置错误页面
5. 内置markdown解析模块，可以直接在页面中显示markdown效果。


</table>

## 编译以及使用

### 编译流程
```shell
    mkdir build
    cmake ..
    make 
    sudo make  install
```

### 样例配置文件
在安装完成后在`properties.json`中修改自己需要的配置
```JSON
{
  "server": {
    "port": 9999, 
    "dynamic_file": [
      "/examples/servlets/" 
    ],
    "redirect_path": ["/xyd/anon","/api/carousel"],
    "redirect_site": ["https://doublehh.cn","http://doublehh.cn:82"],
    "map_path": "/Users/fss/CLionProjects/httpServ/",
    "index": "hello.html",
    "log_path": "/Users/fss/servlog.log",
    "alternate_port": 9998,
    "static_page": "./www/http/pages/static/%s",
    "load_balancing": 1,
    "load_servers": [
      8080,
      8081,
      8082
    ]
  }
}




```

### 启动服务器
终端使用命令`nohup httpServer &`启动服务器

## 致谢
感谢我的好朋友[胡昊](https://github.com/1120023921)为这个项目的开发出谋划策，提供各种各样的帮助。
