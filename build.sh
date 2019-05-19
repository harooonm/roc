BUILD_TYPE=debug

rm -rf libsighandler
rm -rf lib

if [ $# -ne 0 ]
then
	BUILD_TYPE=$1
fi

git clone https://github.com/harooonm/libsighandler.git

CUR_DIR=$(pwd)
cd libsighandler &&  ./build.sh $BUILD_TYPE && cd $CUR_DIR

cp libsighandler/include/sighandler.h libsighandler/include/libbtree.h include
mkdir lib && cp libsighandler/lib/*.so lib/  && cp libsighandler/libsighandler.so lib/

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$CUR_DIR"
make $BUILD_TYPE
rm -rf libsighandler/
