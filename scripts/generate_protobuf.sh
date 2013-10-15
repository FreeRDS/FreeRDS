#!/bin/bash

PROTOC=protoc
PROTOCC=protoc-c

PBRPC_GEN_C="freerds/icp/pbrpc/gen-c"
ICP_GEN_C="freerds/icp/gen-c"

GEN_CPP="session-manager/gen-cpp"
PBDIR="session-manager/protobuf"
PBFILES="ICP.proto  pbRPC.proto"


which $PROTOC > /dev/zero
if [ $? -ne 0 ];then
	echo "protoc not found in path"
	exit 1
fi

which $PROTOCC  > /dev/zero
if [ $? -ne 0 ];then
	echo "protocc not found in path"
	exit 1
fi

#generate c++
for i in $PBFILES;do
	$PROTOC -I $PBDIR --cpp_out $GEN_CPP ${PBDIR}/$i
done

$PROTOCC -I $PBDIR --c_out $PBRPC_GEN_C ${PBDIR}/pbRPC.proto
$PROTOCC -I $PBDIR --c_out $ICP_GEN_C ${PBDIR}/ICP.proto

