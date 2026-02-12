set -e

if [ -d /usr/include/mymuduo ];then
    rm -rf /usr/include/mymuduo
fi

echo "header removed"

if [ -e /usr/lib/libmymuduo.so ];then
    rm /usr/lib/libmymuduo.so
fi

echo "library removed"