BUILD_TYPE=Debug

rm -rf libsighandler
rm -rf lib
if [ $# -ne 0 ]
then
	BUILD_TYPE=$1
fi

git clone https://github.com/harooonm/libsighandler.git

CUR_DIR=$(pwd)
cd libsighandler && ./build.sh "$BUILD_TYPE" && cd "$CUR_DIR"

mkdir lib
cp  libsighandler/lib/libbtree.so  lib/libbtree.so
cp libsighandler/include/libbtree.h  include/libbtree.h
cp  libsighandler/include/sighandler.h include/sig_handler.h
cp  libsighandler/libsighandler.so lib/libsighandler.so

cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" . && make
