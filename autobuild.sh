#/bin/bash
set -e

echo "请确保用户有sudo权限"


if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
else
    rm -r `pwd`/build/*
fi


cd build && cmake .. && make

# 回到根目录
cd ../ && pwd

if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
    echo "创建/usr/include/mymuduo目录"
fi

for file in `ls ./include/*.h`; do
    cp ${file} /usr/include/mymuduo/
    echo "安装头文件: ${file}"
done

pwd

cp ./lib/libmymuduo.so /usr/lib/


# 刷新动态链接库缓存
ldconfig

echo "安装成功"
echo "头文件被安装在/usr/include/mymuduo里"
echo "动态链接库被安装在/usr/lib里"
