# 安装以及第一次编译

1. 下载项目源代码并进入项目根目录

   `git clone https://github.com/Yuan-Yuhan/postgresql-json-cache.git`

2. `adduser postgres`	# 添加postgres用户, 若已添加可以忽略

   `mkdir /var/postgresql/data` # 创建data文件夹

   `chown postgres /var/postgresql/data` # 授予postgres权限

   `chmod -R 775 ./*` # 赋予所有读写权限

3. `sh configure --enable-depend --enable-cassert --enable-debug CFLAGS="-ggdb -O0"` # 初始化config

   `su` # 以超级管理员权限进行编译，如果未开启可能出现chmod不允许

   `make && make install` # 编译，如果在这一步出现了组件缺少报错，则安装组件，并重新初始化Config

4. `su - postgres` # 登录postgres用户

   `/usr/local/pgsql/bin/initdb -D /var/postgresql/data` # 初始化数据库文件夹
   
   `/usr/local/pgsql/bin/pg_ctl start -D  /var/postgresql/data` # # 启动服务器
   
   `/usr/local/pgsql/bin/createdb postgres`	# 创建数据库
   
    `/usr/local/pgsql/bin/psql postgres`	# 连接数据库


# 开发更新和编译

1. `sudo make distclean`	# 清空所有上次configure产生的文件(重新编译时需执行)

2. `sh configure --enable-depend --enable-cassert --enable-debug CFLAGS="-ggdb -O0"` # 初始化config

   `su` # 以超级管理员权限进行编译，如果未开启可能出现chmod不允许
   
   `make && make install` # 编译，如果在这一步出现了组件缺少报错，则安装组件，并重新初始化Config

# 文件索引
- `src/include/utils` 头文件
    - `arc_json.h` JSON结果缓存设计
    - `json_cache_utils.h` JSON解析信息缓存、工具方法
    - `json_update_sync.h` 跨进程同步消息机制
    - `jsonb_cache` 测试JSONB优化可行性的数据结构
    - `json_cache` Deprecated, 较旧版本的JSON结果缓存设计
- `src/backend/utils/adt` 实现
    - 以上所有.h文件的实现均在此目录下
- 被修改的主要PG文件
    - `src/backend/utils/adt/jsonfuncs.c`
    - `src/common/jsonapi.c`
    - `src/backend/executor/execExprInterp.c`
- 根目录下的tests文件夹
    - `rows.json` JSON测试数据
    - `rows.sql` JSON测试SQL语句
    - `rows.sh` JSON测试脚本
    - `rows_jsonb.json` JSONB测试数据
    - `rows_jsonb.sql` JSONB测试SQL语句
    - `rows_jsonb.sh` JSONB测试脚本